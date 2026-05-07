#ifndef GRAPH_H
#define GRAPH_H

#include "DataTypes.h"
#include <vector>
#include <map>
#include <set>

/**
 * @brief Represents the Interference Graph for the register allocation problem.
 * @details Converts isolated variable live ranges into Webs (graph nodes) and
 * builds the adjacency list by creating edges between Webs that share execution points.
 */
class Graph {
private:
    std::vector<Web> webs;                           /**< @brief List of all Webs created (graph nodes). */
    std::map<int, std::set<int>> adjList;            /**< @brief Adjacency list: maps a Web ID to a set of neighboring Web IDs (interfering webs). */

    /**
     * @brief Groups LiveRanges belonging to the same variable, merging those that overlap in time.
     * @param liveRanges Vector containing the raw parsed live-range entries for each variable.
     * @note Time complexity: O(V * R^2), where V is the number of distinct variables and R is the average number of ranges per variable (due to nested merge loops).
     */
    void buildWebs(const std::vector<LiveRange>& liveRanges);

    /**
     * @brief Evaluates interference among all Webs and builds the adjacency list.
     * @details If two Webs coexist on the same source line, a bidirectional edge is created between them.
     * @note Time complexity: O(W^2 * L), where W is the total number of Webs and L is the average number of source lines per Web.
     */
    void buildInterferences();

public:
    /**
     * @brief Graph constructor. Automatically initializes Webs and interferences.
     * @param liveRanges Vector of temporal blocks read by the Parser.
     */
    Graph(const std::vector<LiveRange>& liveRanges);

    /**
     * @brief Returns the list of Webs (nodes) built in the graph.
     * @return const std::vector<Web>& Constant reference to the vector of Webs.
     */
    const std::vector<Web>& getWebs() const { return webs; }

    /**
     * @brief Returns the interference mapping between Webs.
     * @return const std::map<int, std::set<int>>& Constant reference to the adjacency list.
     */
    const std::map<int, std::set<int>>& getAdjList() const { return adjList; }
    
    /**
     * @brief Prints the graph to the console for debugging and presentation purposes (T1.1).
     */
    void printGraph() const;
};

#endif