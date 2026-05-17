#!/usr/bin/env bash
#
# Batch-mode test suite for register allocator (myProg -b)
#
# Usage:
#   ./tests/run_tests.sh              # run all tests
#   ./tests/run_tests.sh basic        # run only basic-algorithm tests (registers1-5)
#   ./tests/run_tests.sh spilling     # run only spilling-algorithm tests
#   ./tests/run_tests.sh splitting    # run only splitting-algorithm tests
#   ./tests/run_tests.sh free         # run only free-algorithm tests (free1-5)
#   ./tests/run_tests.sh ranges       # run cross-product of ranges1-8 + special files
#   ./tests/run_tests.sh smoke        # run only smoke / error-handling tests
#

set -o pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="$PROJECT_DIR/myProg"
TEMP_DIR=$(mktemp -td batch_test_XXXXXX)
PASS=0
FAIL=0
SKIP=0

trap 'rm -rf "$TEMP_DIR"' EXIT

# Colours
green()  { printf '\033[32m%s\033[0m\n' "$*"; }
red()    { printf '\033[31m%s\033[0m\n' "$*"; }
yellow() { printf '\033[33m%s\033[0m\n' "$*"; }

pass() { ((PASS++)); }
fail() { ((FAIL++)); red "  ✗ $*"; }
skip() { ((SKIP++)); yellow "  – $* (skipped)"; }

# Run one batch-mode test case and validate output.
# Allocator warnings on stderr are allowed (eg "allocation not possible").
#   $1  label
#   $2  ranges file
#   $3  registers file
#   $4  output file (will be created)
run_test() {
    local label="$1" ranges="$2" regs="$3" out="$4"
    printf '  [ ] %s ... ' "$label"

    local stderr
    stderr=$(mktemp "$TEMP_DIR/stderr.XXXXXX")

    "$TARGET" -b "$ranges" "$regs" "$out" >/dev/null 2>"$stderr"
    local rc=$?

    # Must exit 0 even when allocation is impossible
    if [[ $rc -ne 0 ]]; then
        local err
        err=$(<"$stderr")
        rm -f "$stderr"
        fail "$label (exit code $rc${err:+: $err})"
        return
    fi
    rm -f "$stderr"

    # --- Validate output files ---

    # Text output
    if [[ ! -f "$out" || ! -s "$out" ]]; then
        fail "$label (output file missing or empty: $out)"
        return
    fi

    # DOT graph (interference)
    local dotfile="${out}.gv"
    if [[ ! -f "$dotfile" || ! -s "$dotfile" ]]; then
        fail "$label (DOT file missing or empty: $dotfile)"
        return
    fi

    # Coloured DOT (allocated)
    local allocated_dot="${out}_allocated.gv"
    if [[ ! -f "$allocated_dot" || ! -s "$allocated_dot" ]]; then
        fail "$label (allocated DOT file missing or empty: $allocated_dot)"
        return
    fi

    # Output format: webs: N, web0: ..., registers: N, r0/M: web...
    if ! grep -q '^webs: [0-9]' "$out" 2>/dev/null; then
        fail "$label (output missing 'webs: N' line)"
        return
    fi
    if ! grep -q '^registers: [0-9]' "$out" 2>/dev/null; then
        fail "$label (output missing 'registers: N' line)"
        return
    fi
    if ! grep -q '^web[0-9]' "$out" 2>/dev/null; then
        fail "$label (output missing web lines)"
        return
    fi
    if ! grep -Eq '^[rM][0-9]*: web[0-9]' "$out" 2>/dev/null; then
        fail "$label (output missing register/memory assignment lines)"
        return
    fi

    # DOT file has graph header
    if ! grep -q '^graph {' "$dotfile" && ! grep -q '^digraph {' "$dotfile"; then
        fail "$label (DOT file missing 'graph {' header)"
        return
    fi

    # Allocated DOT has graph header
    if ! grep -q '^graph {' "$allocated_dot" && ! grep -q '^digraph {' "$allocated_dot"; then
        fail "$label (allocated DOT file missing 'graph {' header)"
        return
    fi

    pass
}

# ---------------------------------------------------------------
#  Error-handling / smoke tests
# ---------------------------------------------------------------
test_smoke() {
    echo ""
    echo "=== Smoke / Error-handling tests ==="

    # -b with no file args at all
    "$TARGET" -b >/dev/null 2>/dev/null
    if [[ $? -eq 0 ]]; then
        fail "batch with no file args should not exit 0"
    else
        pass
    fi

    # -b with only one file arg
    "$TARGET" -b "$PROJECT_DIR/data/ranges/ranges1.txt" >/dev/null 2>/dev/null
    if [[ $? -eq 0 ]]; then
        fail "batch with 1 file arg should not exit 0"
    else
        pass
    fi

    # Non-existent ranges file → output should not be created
    local out
    out="$TEMP_DIR/smoke_bad_ranges.txt"
    "$TARGET" -b "$TEMP_DIR/nonexistent.txt" \
              "$PROJECT_DIR/data/basic/registers2.txt" \
              "$out" >/dev/null 2>/dev/null
    if [[ -f "$out" && -s "$out" ]]; then
        fail "bad ranges file should not produce output"
    else
        pass
    fi

    # Non-existent registers file → falls back to defaults (registers=0)
    # Output should still be produced with registers: 0
    out="$TEMP_DIR/smoke_bad_regs.txt"
    "$TARGET" -b "$PROJECT_DIR/data/ranges/ranges1.txt" \
              "$TEMP_DIR/nonexistent.txt" \
              "$out" >/dev/null 2>/dev/null
    if grep -q '^registers: 0$' "$out" 2>/dev/null; then
        pass
    else
        fail "bad registers file should produce output with registers: 0"
    fi
}

# ---------------------------------------------------------------
#  Algorithm-specific tests (using ranges1.txt)
# ---------------------------------------------------------------
test_basic() {
    echo ""
    echo "=== Basic algorithm (ranges1.txt) ==="
    for r in 1 2 3 4 5; do
        local out="$TEMP_DIR/basic_r${r}.txt"
        run_test "basic  registers=${r}" \
            "$PROJECT_DIR/data/ranges/ranges1.txt" \
            "$PROJECT_DIR/data/basic/registers${r}.txt" \
            "$out"
    done
}

test_free() {
    echo ""
    echo "=== Free algorithm (ranges1.txt) ==="
    for r in 1 2 3 4 5; do
        local out="$TEMP_DIR/free_r${r}.txt"
        run_test "free  registers=${r}" \
            "$PROJECT_DIR/data/ranges/ranges1.txt" \
            "$PROJECT_DIR/data/free/free${r}.txt" \
            "$out"
    done
}

test_spilling() {
    echo ""
    echo "=== Spilling algorithm (ranges1.txt) ==="
    for n in 1 2 3 4; do
        for x in 1 2 3 4; do
            local out="$TEMP_DIR/spilling_N${n}_X${x}.txt"
            run_test "spilling  N=${n} X=${x}" \
                "$PROJECT_DIR/data/ranges/ranges1.txt" \
                "$PROJECT_DIR/data/spilling/${n}/spilling${x}.txt" \
                "$out"
        done
    done
}

test_splitting() {
    echo ""
    echo "=== Splitting algorithm (ranges1.txt) ==="
    for n in 1 2 3 4; do
        for x in 1 2 3 4; do
            local out="$TEMP_DIR/splitting_N${n}_X${x}.txt"
            run_test "splitting  N=${n} X=${x}" \
                "$PROJECT_DIR/data/ranges/ranges1.txt" \
                "$PROJECT_DIR/data/splitting/${n}/splitting${x}.txt" \
                "$out"
        done
    done
}

# ---------------------------------------------------------------
#  Cross-product: all ranges × representative configs
# ---------------------------------------------------------------
test_all_ranges() {
    echo ""
    echo "=== All ranges × all algorithms ==="
    local -a configs=(
        "basic:data/basic/registers2.txt"
        "free:data/free/free2.txt"
        "spilling:data/spilling/2/spilling2.txt"
        "splitting:data/splitting/2/splitting2.txt"
    )

    for ri in 1 2 3 4 5 6 7 8; do
        local ranges="$PROJECT_DIR/data/ranges/ranges${ri}.txt"
        for entry in "${configs[@]}"; do
            local algo="${entry%%:*}"
            local regs="${PROJECT_DIR}/${entry##*:}"
            local out="$TEMP_DIR/r${ri}_${algo}.txt"
            run_test "ranges${ri}  ${algo}" "$ranges" "$regs" "$out"
        done
    done

    # Special range files (tested against splitting and free)
    local special_ranges=(
        "ranges_cycle_split"
        "ranges_pres1_dsatur"
        "ranges_pres2_split"
        "ranges_pres3_clique"
        "ranges_split_win"
    )
    for name in "${special_ranges[@]}"; do
        local ranges="$PROJECT_DIR/data/ranges/${name}.txt"
        local out_splitting="$TEMP_DIR/${name}_splitting.txt"
        local out_free="$TEMP_DIR/${name}_free.txt"
        run_test "${name}  splitting" "$ranges" \
            "$PROJECT_DIR/data/splitting/2/splitting2.txt" "$out_splitting"
        run_test "${name}  free" "$ranges" \
            "$PROJECT_DIR/data/free/free2.txt" "$out_free"
    done
}

# ---------------------------------------------------------------
#  Main
# ---------------------------------------------------------------

cd "$PROJECT_DIR"

echo "========================================="
echo " Batch-mode test suite"
echo " Project: $PROJECT_DIR"
echo " Temp:    $TEMP_DIR"
echo "========================================="

# Build if needed
if [[ ! -x "$TARGET" ]]; then
    echo ""
    echo "Building project..."
    make -C "$PROJECT_DIR" clean 2>/dev/null
    make -C "$PROJECT_DIR" 2>&1 || {
        red "Build failed – aborting tests."
        exit 1
    }
    echo "Build OK."
fi

FILTER="${1:-all}"

case "$FILTER" in
    all)
        test_smoke
        test_basic
        test_free
        test_spilling
        test_splitting
        test_all_ranges
        ;;
    smoke)    test_smoke ;;
    basic)    test_basic ;;
    free)     test_free ;;
    spilling) test_spilling ;;
    splitting) test_splitting ;;
    ranges)   test_all_ranges ;;
    *)
        echo "Unknown filter: $FILTER"
        echo "Usage: $0 [all|smoke|basic|free|spilling|splitting|ranges]"
        exit 1
        ;;
esac

# Summary
echo ""
echo "========================================="
echo " Results"
echo "========================================="
total=$((PASS + FAIL))
if [[ $FAIL -eq 0 ]]; then
    green "  All $total tests passed."
else
    red "  $FAIL / $total tests FAILED."
fi
if [[ $SKIP -gt 0 ]]; then
    yellow "  ($SKIP skipped)"
fi

exit $FAIL
