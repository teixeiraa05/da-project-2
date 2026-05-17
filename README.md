# Register Allocation via Graph Coloring

This project implements a high-performance **Global Register Allocator** using graph coloring heuristics. It transforms variable live ranges into an interference graph and provides four distinct allocation strategies to optimize register utilization and minimize memory traffic.

## Allocation Architectures

### 1. Basic Allocator
A strictly greedy approach. It removes nodes with degree $< N$ from the graph. If it encounters a state where all remaining nodes have degree $\geq N$, it terminates as unfeasible.

### 2. Spilling Allocator
A reliable allocator that resolves conflicts by moving variables to memory.
*   **Victim Selection:** Chooses the node with the **Highest Degree** to maximize graph simplification.
*   **Tie-breaker:** Prefers nodes with the **Smallest Footprint** (least definitions and uses) to minimize memory access overhead.

### 3. Splitting Allocator (Intelligent Structural Optimization)
An advanced alternative to spilling that fragments live ranges to break interferences.
*   **Exhaustive Search:** Unlike standard mid-point splitters, this implementation uses a **Brute-Force Simulation** to evaluate every possible split point for the most problematic variable.
*   **Colorability Heuristic:** For each candidate split, it runs a simplification pass to count how many nodes would remain "stuck." It selects the exact program point that resulting in the **best colorability**.
*   **Cycle Breaking:** This strategy is specifically designed to resolve complex interference **Cycles** (e.g., the 5-cycle case) without resorting to memory.

### 4. Free (Hybrid) Allocator
The most sophisticated tier, combining global analysis with high-end heuristics. It follows a prioritized decision tree:
1.  **Clique Detection (Fail-Fast):** Detects $N+1$ variable cliques immediately. If found, it skips useless splits and performs an immediate spill.
2.  **DSATUR Coloring:** Uses the **Degree of Saturation** algorithm. It prioritizes nodes based on neighbor constraints, finding optimal colorings for dense graphs where greedy ordering fails.
3.  **Advanced Splitting Fallback:** Triggers the exhaustive simulation splitter if coloring fails.
4.  **Surgical Spill:** Performs a targeted spill if the split budget is exhausted.

---

## Technical Details

For detailed internal logic, time complexity analysis, and heuristic justifications, please refer to:
*   [ALLOCATORS.md](./ALLOCATORS.md) - Deep dive into algorithm internals.
*   [Doxygen Documentation](./docs/html/index.html) - Full API reference (run `doxygen docs/Doxyfile` to generate).

## Usage

### Batch Mode
```bash
./myProg -b <ranges_file> <config_file> <output_file>
```

### Interactive Mode
```bash
./myProg
```
Follow the on-screen menu to load data, choose algorithms, and run the allocation.

---
**FEUP - DA Project 2**
Group T09_G09
