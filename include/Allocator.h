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
    int splitCount;                      /**< @brief Number of splits performed by the last allocation run. */
    int spillCount;                      /**< @brief Number of spills performed by the last allocation run. */
    
    std::vector<Web> finalWebs;          /**< @brief Mutable copy of the Webs, required to allow splitting. */

    /**
     * @brief Returns the next available Web identifier.
     * @param webs Current list of webs.
     * @return The first unused numeric Web id.
     */
    int nextWebId(const std::vector<Web>& webs) const;
    
    /**
     * @brief Finds a web by its identifier inside a list of webs.
     * @param webs Collection to search.
     * @param webId Web identifier to locate.
     * @return Pointer to the matching web, or nullptr if it does not exist.
     */
    const Web* findWebById(const std::vector<Web>& webs, int webId) const;

    /**
     * @brief Formats the live-range contents of a web for display in the DOT legend.
     * @param web Web to format.
     * @return A compact string listing the web's sorted program points with markers.
     */
    std::string formatWebRange(const Web& web) const;

    /**
     * @brief Escapes text so it can be safely embedded in a DOT HTML label.
     * @param text Plain text to escape.
     * @return Escaped HTML-safe text.
     */
    std::string escapeDotHtml(const std::string& text) const;
    
    /**
     * @brief Splits a web into two at a specific point in its program line set.
     * @param original The web to split.
     * @param newId Identifier for the newly created web.
     * @param splitIndex The index in the original lines set where the split occurs.
     * @return A pair containing the left and right split webs.
     */
    std::pair<Web, Web> splitWebAtPoint(const Web& original, int newId, size_t splitIndex) const;

    /**
     * @brief Evaluates the colorability of a hypothetical graph configuration.
     * @param webs The candidate set of webs.
     * @param N Number of available registers.
     * @return The number of nodes that remain "stuck" (degree >= N) after simplification.
     */
    int evaluateColorability(const std::vector<Web>& webs, int N) const;

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
        * @details Chooses the active web with the highest interference degree. If several nodes
        * share the same degree, prefers the one with the smaller live-range footprint.
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
     * @brief Finds a clique (complete subgraph) using a greedy heuristic.
     * @param adjList Current adjacency list.
     * @param webIds Set of active web IDs to consider.
     * @return A set of Web IDs forming the detected clique.
     */
    std::set<int> findGreedyClique(const std::map<int, std::set<int>>& adjList,
        const std::set<int>& webIds) const;
        
    /**
     * @brief Rebuilds the adjacency list for the current `finalWebs` set.
     * @param outAdj Output adjacency list mapping web id -> neighbor ids.
     * @param skipWebs Optional pointer to a set of web ids to ignore (e.g., spilled webs). If nullptr, no webs are skipped.
     */
    void rebuildAdjacency(std::map<int, std::set<int>>& outAdj, const std::set<int>* skipWebs = nullptr) const;
    
    /**
        * @brief Attempts to color the graph by repeatedly removing nodes with degree less than N.
        * @details When all remaining nodes have degree >= N, it selects a spill candidate and continues.
        * @return true If allocation terminates successfully (possibly with spilled webs marked as memory).
        * @note Time complexity: O(W^2 + E), where W is the number of Webs and E the number of edges in the graph.
        */
    bool runBasicAllocation();
    
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
     * @note Time complexity: O(C * W^3 + S_limit * L * (W^2 * L + E)), as it evaluates graph colorability across multiple potential split points.
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