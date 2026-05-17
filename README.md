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
### 1. Core Mechanism: Fragmentation
Instead of moving a variable to memory, the allocator identifies a "web" (node) that is difficult to color and splits it into two independent sub-webs at a program point. This effectively breaks one long node into two shorter nodes, potentially reducing the interference degree in critical regions of the code.

### 2. Selection Strategy: Highest Degree
When the graph is stuck (all remaining nodes have degree $\geq N$), the algorithm picks the web with the **highest interference degree** that has at least 2 program lines to split. Removing a high-degree node first eliminates the most blocking constraints from the graph.

### 3. Split Point: Exhaustive Brute-Force Search
To determine *where* to split, the algorithm runs a **full simulation for every possible split point**. For each candidate point $p$, it:
1. Splits the web into two fragments at $p$.
2. Rebuilds the interference graph with the new fragments.
3. Runs a **Simplified Colorability Test** (Chaitin-style simplification) to count how many nodes remain "stuck" (degree $\geq N$).
4. Selects the split point that **minimizes the stuck count**.

If a split point achieves zero stuck nodes, the search stops early.

### 4. Algorithmic Workflow (Iterative Restart)
Splitting is handled as an iterative process:
1. Attempt to color the graph using standard simplification.
2. If stuck (no nodes with degree < N), identify the highest degree web with at least 2 lines.
3. Exhaustively search for the best split point by simulating each candidate.
4. Split the web, update the global list of webs, and **completely rebuild** the interference graph.
5. Restart the allocation process from scratch with the new graph structure.
6. Repeat until the graph is colored or the `K_param` (split limit) is reached.

### 5. Architectural Trade-offs
- **Pros:** Maximizes register usage and minimizes costly memory traffic (loads/stores) compared to spilling.
- **Cons:** Significantly more computationally expensive. Each split requires rebuilding the interference graph and simulating every possible split point, leading to a complexity of $O(S \cdot L \cdot W^3)$, where $S$ is the split budget and $L$ is the maximum live range length. It also increases the number of nodes in the graph, making the coloring problem larger over time.

## Free Mode: The "Smart" Allocator

The `Free` algorithm is the most advanced mode, combining multiple heuristics into a unified decision loop. It aims to be "ambitious" about registers while remaining "realistic" about mathematical constraints.

### 1. Fail-Fast Clique Detection
Before attempting to color, the allocator runs a **Greedy Clique Finder**. 
- If a clique larger than the number of available registers is found, the allocator knows the graph is mathematically impossible to color.
- It immediately triggers a **Spill** on the highest-impact node in that clique. This prevents wasting time on expensive coloring attempts that are guaranteed to fail.

### 2. DSATUR Coloring
Free mode uses the **DSATUR (Degree of Saturation)** algorithm instead of a simple greedy stack. This selects nodes based on how "restricted" they are by their neighbors, leading to higher-quality colorings that often fit in fewer registers.

### 3. Hybrid Resolution with Split-First Preference
If DSATUR fails to color a node, the allocator enters a resolution phase:
- **Phase A (Splitting):** It attempts to split the problematic node. To prevent infinite loops or excessive fragmentation, it is limited to **3 splits** per allocation run.
- **Phase B (Spilling):** If the split budget is exhausted or the node is too small to split (only 1 point), it falls back to a **Spill**, moving the variable to memory.

This hybrid approach ensures that the allocator explores all possibilities to keep variables in registers (via splitting) before finally giving up and using memory.

## Splitting Analysis and Limitations

The splitting algorithm is designed to break large live ranges into smaller pieces to reduce interference. However, certain patterns in the data can lead to infeasibility:

### Range 7: Brute-Force Budget Limitation (The Split Budget Problem)
In `ranges7.txt`, the algorithm may fail due to the limited split budget being exhausted.
- **The Problem:** The data contains 10 webs with large live ranges spanning both a dense interference zone (lines 103–178) and isolated tail lines (1000–1009). Each web's long range requires many split-point simulations, and each split only produces a marginal improvement.
- **The Result:** The limited split budget (K=10) is consumed before the graph becomes fully colorable. A larger budget or a heuristic that preferentially splits within the dense zone would be required.

### Range 8: Structural Bottleneck (Point Cliques)
In `ranges8.txt`, multiple variables (e.g., v0, v1, v2, v5) are all active at the exact same program point (line 6). 
- **The Problem:** Even if these webs are split, any fragment that contains line 6 will still interfere with every other fragment containing line 6. 
- **The Result:** This creates an "instantaneous" K4 clique (a complete graph of 4 nodes) at line 6. Since a graph with a K4 clique requires at least 4 colors, it is impossible to allocate with only 2 registers using splitting alone. Only **Spilling** can resolve this by moving some variables to memory.

## Additional Range Files

The project includes several additional range files for testing specific allocator behaviors:

| File | Purpose |
|------|---------|
| `ranges_cycle_split.txt` | **5-Cycle Split Case:** A pentagon interference graph (odd cycle) that requires 3 colors. With N=2, splitting can break the cycle by splitting one node, reducing the chromatic number to 2. |
| `ranges_pres1_dsatur.txt` | **Star Graph (DSATUR Superiority):** Center node v0 connects to 5 leaf nodes. Greedy coloring may fail if leaves are colored first, but DSATUR colors the center first (highest saturation), demonstrating its advantage over simple greedy ordering. |
| `ranges_pres2_split.txt` | **Surgical Split (Hybrid Resolution):** A `long_var` bridges two independent computation regions. Free allocator splits `long_var` at the natural gap between the two regions, removing the artificial conflict without spilling. |
| `ranges_pres3_clique.txt` | **Point Clique (Fail-Fast Detection):** Four variables share line 5, forming a K4 clique. With 3 registers, Free allocator's greedy clique finder detects the bottleneck instantly and spills directly — bypassing DSATUR entirely. |
| `ranges_split_win.txt` | **Split Win Case:** A bridge variable spans two regions — a high-pressure region (3 variables, 2 registers) and a low-pressure region (2 variables, 2 registers). Splitting the bridge at the boundary resolves both regions independently. |
