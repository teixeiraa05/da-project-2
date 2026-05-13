#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "InterferenceGraph.h"
#include "Parser.h"
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

/**
 * @brief Class responsible for register allocation based on Graph Coloring.
 * @details Attempts to assign each Web (node) a "color" (physical register) ensuring
 * no interfering Web shares the same register. Implements Spilling and Splitting tactics when resources are insufficient.
 */
class Allocator {
private:
    InterferenceGraph graph;             /**< @brief Instance of the InterferenceGraph containing Webs and edges. */
    ConfigRegisters config;              /**< @brief Configuration for register limits and algorithm parameters. */
    
    std::map<int, int> allocation;       /**< @brief Final mapping from Web ID to register color (-1 means Memory/M). */
    bool isFeasible;                     /**< @brief Boolean flag indicating whether the algorithm completed the allocation. */
    
    std::vector<Web> finalWebs;          /**< @brief Mutable copy of the Webs, required to allow splitting. */

    /**
        * @brief Attempts to color the graph by repeatedly removing nodes with degree less than N.
        * @details When all remaining nodes have degree >= N, it selects a spill candidate and continues.
        * @return true If allocation terminates successfully (possibly with spilled webs marked as memory).
     * @note Time complexity: O(W^2 + E), where W is the number of Webs and E the number of edges in the graph.
     */
    bool runBasicAllocation();

    /**
     * @brief Simplification step: find a node with degree < N, remove it from the graph,
     * push it onto the stack and decrement the remaining node count.
     * @return true if a node was removed; false otherwise.
     */
    bool simplifyFindAndRemove(const std::map<int, std::set<int>>& adjList,
                               std::map<int, bool>& inGraph,
                               int N,
                               std::stack<int>& S,
                               int& nodesRemaining);

    /**
     * @brief Selects and removes one spill candidate from the current simplification graph.
     * @param originalAdjList Interference adjacency list used for degree computation.
     * @param inGraph Marks whether each node is currently active in simplification.
     * @param nodesRemaining Counter of active nodes; decremented when a spill is applied.
     * @param spilledNodeId Output parameter with the chosen node ID.
     * @return true If a node was selected and removed.
     * @return false If no active node was available.
     */
    bool spillNode(const std::map<int, std::set<int>>& originalAdjList,
                   std::map<int, bool>& inGraph,
                   int& nodesRemaining,
                   int& spilledNodeId);

    /**
     * @brief Assigns register colors by popping nodes from the simplification stack.
     * @param S Stack with node ids in reverse simplification order.
     * @param adjList Adjacency list used to collect already-used neighbor colors.
     * @param N Number of available registers.
     * @return true If every node in the stack received a register color.
     * @return false If at least one node could not be assigned any color.
     */
    bool colorNodes(std::stack<int>& S, const std::map<int, std::set<int>>& adjList, int N);

    /**
     * @brief DSATUR algorithm: colors the graph based on degree of saturation.
     * @details Iteratively selects the uncolored vertex with highest saturation degree
     * (number of different colors used by neighbors). Breaks ties by degree.
     * Typically produces better colorings than greedy approaches.
     * @param adjList Adjacency list representation of the graph.
     * @param webIds Set of Web IDs to color (others are ignored or assumed spilled).
     * @param N Number of available registers.
     * @return true If all webs in webIds received a color.
     * @return false If at least one web could not be colored with N registers.
     * @note Time complexity: O(W^2 * N) where W is the number of webs.
     */
    bool colorNodesDSATUR(const std::map<int, std::set<int>>& adjList,
                          const std::set<int>& webIds, int N);

    /**
     * @brief Rebuilds the adjacency list for the current `finalWebs` set.
     * @param outAdj Output adjacency list mapping web id -> neighbor ids.
     * @param skipWebs Optional pointer to a set of web ids to ignore (e.g., spilled webs). If nullptr, no webs are skipped.
     */
    void rebuildAdjacency(std::map<int, std::set<int>>& outAdj, const std::set<int>* skipWebs = nullptr) const;

    /**
     * @brief Coloring algorithm which, when stuck, sacrifices (spills) the Web with highest interference degree.
     * @return true If the allocation completed within the stipulated limit of K spills.
     * @note Time complexity: O(W^3 + E) in the worst case, since each spill may require recomputing degrees for remaining nodes.
     */
    bool runSpillingAllocation();

    /**
     * @brief Algorithm that splits blocking Webs in half (Splitting) to try to place them in different registers.
     * @return true If the allocation succeeded with the transformed graph.
     * @note Time complexity: O(S * (W^3 + E)), where S is the split limit (K_param), since computation restarts after each split.
     */
    bool runSplittingAllocation();

    /**
     * @brief Custom heuristic that favors Splitting to maximize variables in registers, using Spilling only as a last resort.
     * @return true The algorithm is guaranteed to succeed due to a fallback tactic.
     * @note Time complexity: O(S * (W^3 + E)), as it continuously evaluates the graph state after splits and hybrid removals.
     */
    bool runFreeAllocation(); 

public:
    /**
     * @brief Allocator class constructor. Prepares internal structures based on the generated graph.
     * @param g Constant reference to the Interference Graph.
     * @param c Definitions read from the registers/configuration file.
     */
    Allocator(const InterferenceGraph& g, const ConfigRegisters& c);

    /**
     * @brief Launches execution of the specific algorithm dictated in the configuration file (basic, spilling, splitting or free).
     */
    void run();

    /**
     * @brief Generates the final text file in the strict format required by the project specification.
     * @param filename Path and name of the file (e.g. output.txt) where the result will be written.
     * @note Time complexity: O(W + L), where W is the number of resulting webs and L the total sum of their lines.
     */
    void generateOutputFile(const std::string& filename) const;

    /**
     * @brief Generates a colored DOT file showing register allocation results.
     * @details Nodes are colored by their assigned register: registers r0-r9 get distinct colors,
     * and memory-spilled webs (M) are shown in gray.
     * @param filename Base name for the DOT file (will append .gv automatically).
     */
    void generateColoredDOT(const std::string& filename) const;
};

#endif