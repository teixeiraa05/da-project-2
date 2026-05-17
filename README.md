# DA Project 2

This project implements global register allocation using graph coloring over interference graphs.

## Spilling Strategy

The spilling algorithm is intentionally structural-first. When the allocator gets stuck, it selects the active web with the highest interference degree, because that node is blocking the largest number of neighbors from being simplified or colored.

If two candidate webs have the same degree, the allocator uses a smaller `defLines + useEndLines` footprint as a tie-breaker. This keeps the main decision focused on graph structure, while still preferring the web that is cheaper to move to memory when the structural impact is the same.

In practice, this means the spill policy tries to remove the node that helps colorability the most, and only then considers spill cost as a secondary criterion.

## Splitting Strategy Analysis

The splitting algorithm implemented in this project serves as a sophisticated alternative to spilling, aiming to keep variables in registers by fragmenting their live ranges.

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
