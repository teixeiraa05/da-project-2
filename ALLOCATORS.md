# Register Allocator Architectures

This document details the internal logic and heuristics used by the four allocation algorithms implemented in this project.

## 1. Basic Allocator
The **Basic Allocator** is a strictly greedy approach.

*   **Logic:** It attempts to remove nodes from the interference graph that have a degree less than the number of available registers ($N$). These nodes are pushed onto a stack.
*   **Failure Mode:** If it reaches a state where every remaining node has a degree $\geq N$, the algorithm fails immediately and returns an unfeasible status.
*   **Coloring:** If simplification succeeds, it pops nodes from the stack and assigns the smallest available register index that doesn't conflict with its neighbors.

## 2. Spilling Allocator
The **Spilling Allocator** extends the Basic logic with a recovery mechanism to handle complex graphs.

*   **Logic:** When simplification gets "stuck" (all nodes have degree $\geq N$), it doesn't fail. Instead, it selects a victim node to "spill" to memory.
*   **Heuristic:** It picks the node with the **Highest Degree** (most interferences) to provide the maximum relief to the graph. 
*   **Tie-breaker:** If degrees are equal, it spills the node with the **Smallest Footprint** (least number of definitions and last uses) to minimize total memory traffic.
*   **Outcome:** The spilled node is assigned to Memory (M), and the algorithm continues simplifying the remaining graph.

## 3. Splitting Allocator
The **Splitting Allocator** attempts to keep variables in registers by breaking their live ranges into smaller, more manageable fragments.

*   **Exhaustive Search:** Unlike traditional allocators that split at arbitrary midpoints, this implementation uses a **Brute-Force Simulation**.
*   **Logic:** It identifies the highest-degree node (with at least 2 lines) and simulates a split at **every possible program point** within its range.
*   **Evaluation:** For every potential split, it rebuilds a temporary interference graph and runs a simplification pass to count "stuck" nodes (those that would still force a spill). 
*   **Selection:** It chooses the exact split point that results in the **best colorability** (minimum number of stuck nodes). This surgically breaks cycles and high-pressure cliques without immediately resorting to memory.

## 4. Free (Hybrid) Allocator
The **Free Allocator** is the most advanced tier, combining global program analysis with high-end heuristics. It follows a prioritized decision tree:

1.  **Clique Detection (Fail-Fast):** Before coloring, it runs a greedy clique finder. If it finds a group of $N+1$ variables all alive at the same line, it knows coloring is mathematically impossible. It spills the node with the **highest degree** within that clique instantly to save time.
2.  **DSATUR Coloring:** Instead of standard greedy ordering, it uses the **Degree of Saturation (DSATUR)** algorithm. It prioritizes nodes that have the most "saturated" neighbors (neighbors already using different colors), which is significantly more effective at finding optimal colorings for dense graphs.
3.  **Advanced Splitting Fallback:** If DSATUR fails to color all nodes, it triggers the **Exhaustive Brute-Force Split** logic (from the Splitting Allocator) for the uncolored node with the highest degree.
4.  **Surgical Spill:** If the split budget (limit of 3) is exhausted or splitting doesn't improve colorability, it performs a targeted spill of the hardest node and restarts the loop.

### Strong & Optimal Graph Topologies

The Free Allocator is highly efficient—and often optimal—for several critical graph classes:

1.  **Interval Graphs:** Most linear code (without complex jumps) generates interference graphs that are *Interval Graphs*. DSATUR is near-optimal for this class, consistently finding the minimum chromatic number for standard program variable flows.
2.  **Bipartite Graphs:** The algorithm is guaranteed to find an optimal 2-coloring for any bipartite structure, making it perfect for simple conditional flows.
3.  **Cycle & Wheel Graphs:** DSATUR handles odd/even cycles and wheel topologies optimally. Combined with **Surgical Splitting**, the allocator can break these structures to fit into even smaller register sets than mathematically required for the original graph.
4.  **Star Graphs:** In topologies where a central node interferes with many independent leaves, standard greedy algorithms often fail by coloring leaves first. The **Saturation Heuristic** ensures the central node is prioritized, leading to an optimal coloring instantly.
5.  **Chordal Graphs:** Many register allocation problems result in chordal graphs. The combination of clique detection and DSATUR is exceptionally strong for these structures, as they are "Perfect Graphs" where the clique number equals the chromatic number.

> **Note on Time Complexity vs. Generality:** 
> While the Free Allocator achieves **optimal/close-to-optimal results (minimum colors)** for Interval, Bipartite, and Chordal graphs, its **time complexity is sub-optimal** compared to specialized solvers. For example, Interval graphs can be colored in $O(W \log W)$, whereas DSATUR requires $O(W^3)$. The implementation intentionally prioritizes **algorithmic robustness** and **universality** over theoretical performance for niche cases, ensuring a single resilient solver can handle hybrid graph topologies that "clean" specialized solvers cannot.

### Heuristic Limitations (Free Algorithm)

While highly effective, each stage of the Free Allocator involves trade-offs and heuristic limitations:

*   **Greedy Clique Detection:** Finding the *Maximum Clique* is an NP-Hard problem. The implemented greedy finder may identify a local bottleneck ($N+1$) while missing a larger, more problematic clique ($N+k$). This can lead to sub-optimal spill choices early in the process.
*   **DSATUR Convergence:** DSATUR does not guarantee an optimal coloring for all graph topologies. In rare "worst-case" graphs, it may fail to find a valid coloring even if one exists, potentially triggering expensive splitting simulations prematurely.
*   **Localized Splitting Logic:** The exhaustive simulation evaluates split points based on a simplified "colorability test" rather than a full DSATUR run to maintain performance. Consequently, it may select a split point that simplifies the graph for basic greedy coloring but remains difficult for the actual DSATUR pass.
*   **Splitting Budget ($S_{limit}$):** The allocator only permits up to 3 splits per variable to prevent exponential growth of the interference graph. In deeply nested or highly congested code, this limit may be reached before a valid coloring is found, forcing a spill where more aggressive splitting might have succeeded.
*   **Spill Weight Ignorance:** The "Highest Degree" spill heuristic focuses on graph simplification (removing the most edges). It does not account for the variable's access frequency; a high-degree variable that is used inside a deep loop might be spilled instead of a lower-degree variable that is rarely used, potentially increasing memory traffic.

#### Vulnerable Graph Topologies

The Free Allocator algorithm designed falters to certain types of graphs:

1.  **Mycielski Graphs (Mycielskians):** These graphs are triangle-free (no $K_3$ cliques) but can have arbitrarily high chromatic numbers. Because the Free allocator relies on **Clique Detection** for fast-spilling and **Saturation** for coloring, it often struggles with Mycielskians, which lack local density but possess complex global interferences.
2.  **Smallest Hard-to-Color Graph ($G_1$):** There exists a specific 3-colorable graph with 8 vertices and 12 edges where DSATUR is mathematically forced to use 4 colors regardless of tie-breaking. In such cases, the Free allocator will incorrectly trigger splitting or spilling.
3.  **Highly Symmetric Graphs:** In graphs with high symmetry, many nodes have identical degrees and saturation levels. This neutralizes the "most constrained" heuristic, forcing the algorithm to make arbitrary choices that can lead to sub-optimal colorings.
4.  **Non-Perfect Graphs:** While DSATUR is near-optimal for *Perfect Graphs* (like Interval Graphs found in simple linear code), it struggles with the odd cycles and holes found in complex control-flow graphs (CFGs) with multiple overlapping loops.


---


## Complexity Analysis Summary

| Algorithm | Complexity | Strategy | Best Used For |
| :--- | :--- | :--- | :--- |
| **Basic** | $O(W^2 \cdot L \log L + E)$ | Simple Greedy | Low-pressure, linear code |
| **Spilling** | $O(W^2 \cdot L \log L + W^2 + E)$ | Degree-based Spill | Reliable general-purpose allocation |
| **Splitting** | $O(S \cdot L^2 W^2 \log L)$ | Brute-Force Simulation | Breaking complex interference cycles |
| **Free** | $O(C \cdot W^3 + S_{limit} \cdot L^2 W^2 \log L)$ | Hybrid Decision Tree | Maximum register utilization |

### Phase-by-Phase Detail (Free Allocator)
The Free Allocator operates in a loop that restarts upon any structural change (spill or split). Each iteration consists of:

*   **Graph Reconstruction ($O(W^2 \cdot L \log L)$):** Rebuilds the interference graph. Each pair of Webs ($W^2$) is checked for interference by comparing line sets ($L$) using `std::set` lookups ($O(L \log L)$).
*   **Greedy Clique Detection ($O(W^3)$):** Attempts to build a clique starting from every node by checking mutual interference between neighbors.
*   **DSATUR Coloring ($O(W^3)$):** Iteratively selects the most saturated node and assigns a color. Recomputing saturation after each assignment takes $O(W^2)$.
*   **Exhaustive Splitting Simulation ($O(L^2 W^2 \log L)$):** For each potential split point ($L$), the algorithm rebuilds the graph ($O(W^2 \cdot L \log L)$) and simplifies it ($O(W^2)$).
*   **Total Iteration Complexity:** Dominated by the exhaustive split search: $O(L^2 W^2 \log L)$.

---

*Variables:* 
*   $W$: Number of Webs (nodes)
*   $E$: Number of Interferences (edges)
*   $L$: Maximum length of a Live Range (lines)
*   $S$: Split Limit (constant $K\_param$)
*   $S_{limit}$: Fixed Split Limit for Free Allocator (constant = 3)
*   $C$: Total number of Spills/Splits performed
