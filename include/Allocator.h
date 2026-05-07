#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "Graph.h"
#include "Parser.h"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Class responsible for register allocation based on Graph Coloring.
 * @details Attempts to assign each Web (node) a "color" (physical register) ensuring
 * no interfering Web shares the same register. Implements Spilling and Splitting tactics when resources are insufficient.
 */
class Allocator {
private:
    Graph graph;                         /**< @brief Instance of the original graph containing Webs and edges. */
    ConfigRegisters config;              /**< @brief Configuration for register limits and algorithm parameters. */
    
    std::map<int, int> allocation;       /**< @brief Final mapping from Web ID to register color (-1 means Memory/M). */
    bool isFeasible;                     /**< @brief Boolean flag indicating whether the algorithm completed the allocation. */
    
    std::vector<Web> finalWebs;          /**< @brief Mutable copy of the Webs, required to allow splitting. */

    /**
     * @brief Attempts to color the graph by repeatedly removing nodes with degree less than N (Chaitin's algorithm).
     * @return true If it was possible to color the graph without resorting to Spilling/Splitting.
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
     * @note Time complexity: O(S * (W^3 + E)), as it continuously evaluates the graph state after splits and hybrid removals.
     */
    bool runFreeAllocation(); 

public:
    /**
     * @brief Allocator class constructor. Prepares internal structures based on the generated graph.
     * @param g Constant reference to the Interference Graph.
     * @param c Definitions read from the registers/configuration file.
     */
    Allocator(const Graph& g, const ConfigRegisters& c);

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
};

#endif