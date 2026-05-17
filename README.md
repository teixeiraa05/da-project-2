# DA Project 2

This project implements global register allocation using graph coloring over interference graphs.

## Spilling Strategy

The spilling algorithm is intentionally structural-first. When the allocator gets stuck, it selects the active web with the highest interference degree, because that node is blocking the largest number of neighbors from being simplified or colored.

If two candidate webs have the same degree, the allocator uses a smaller `defLines + useEndLines` footprint as a tie-breaker. This keeps the main decision focused on graph structure, while still preferring the web that is cheaper to move to memory when the structural impact is the same.

In practice, this means the spill policy tries to remove the node that helps colorability the most, and only then considers spill cost as a secondary criterion.

## Splitting Strategy Analysis

The splitting algorithm implemented in this project serves as a sophisticated alternative to spilling, aiming to keep variables in registers by fragmenting their live ranges.

### 1. Core Mechanism: Fragmentation
Instead of moving a variable to memory, the allocator identifies a "web" (node) that is difficult to color and splits it into two independent sub-webs at a program point where it is not being used. This effectively breaks one long node into two shorter nodes, potentially reducing the interference degree in critical regions of the code.

### 2. Selection Strategy: Interference Density
The algorithm doesn't just pick any node to split. It calculates an **Interference Density** for each web:
`Density = Degree / Number of Program Points`
- High density indicates a web that interferes with many neighbors while occupying very few lines.
- Low density indicates a sparse web that is "stretched" over many lines, making it an ideal candidate for splitting as it might only conflict with certain neighbors in specific "bursts."

### 3. Split Point: The "Largest Gap" Heuristic
To determine *where* to split, the algorithm looks for the largest gap between consecutive program points in the web's live range. 
- **Rationale:** A large gap represents a period of inactivity for that variable. Splitting here is "safe" because it doesn't interrupt a tight computation loop and maximizes the chances that the two new fragments can be assigned to different registers.

### 4. Algorithmic Workflow (Iterative Restart)
Splitting is handled as an iterative process:
1. Attempt to color the graph using standard simplification.
2. If stuck (no nodes with degree < N), identify the highest density web.
3. Split the web, update the global list of webs, and **completely rebuild** the interference graph.
4. Restart the allocation process from scratch with the new graph structure.
5. Repeat until the graph is colored or the `K_param` (split limit) is reached.

### 5. Architectural Trade-offs
- **Pros:** Maximizes register usage and minimizes costly memory traffic (loads/stores) compared to spilling.
- **Cons:** Significantly more computationally expensive. Each split requires rebuilding the interference graph, leading to a complexity of $O(S \cdot (W^3 + E))$, where $S$ is the split budget. It also increases the number of nodes in the graph, making the coloring problem larger over time.

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

### Range 7: Heuristic Limitation (The "Distraction" Problem)
In `ranges7.txt`, the algorithm fails due to the "Largest Gap" selection heuristic being misled by specific data structures.
- **The Problem:** The data contains "tail" points (e.g., line 1000+) that create massive gaps compared to the dense interference area (lines 100-150). The algorithm prioritizes splitting at these huge gaps first.
- **The Result:** The limited split budget (K=10) is exhausted on lopping off these irrelevant tails before the allocator ever attempts to split the webs where the actual interference congestion is occurring. A more sophisticated heuristic (e.g., pressure-aware or hole-based splitting) would be required to ignore these distractions.

### Range 8: Structural Bottleneck (Point Cliques)
In `ranges8.txt`, multiple variables (e.g., v0, v1, v2, v5) are all active at the exact same program point (line 6). 
- **The Problem:** Even if these webs are split, any fragment that contains line 6 will still interfere with every other fragment containing line 6. 
- **The Result:** This creates an "instantaneous" K4 clique (a complete graph of 4 nodes) at line 6. Since a graph with a K4 clique requires at least 4 colors, it is impossible to allocate with only 2 registers using splitting alone. Only **Spilling** can resolve this by moving some variables to memory.
