/**
 * @file InterferenceGraph.h
 * @brief Interference graph built from webs for register allocation.
 */
#ifndef INTERFERENCEGRAPH_H
#define INTERFERENCEGRAPH_H

#include "DataTypes.h"
#include "Graph.h"
#include <vector>
#include <map>
#include <set>

/**
 * @brief Interference graph built specifically for register allocation (Webs as nodes).
 *
 * This class builds Webs from raw LiveRanges and computes interferences between them.
 * It also populates the generic base Graph<int> (vertices are web ids) to reuse
 * the helper graph operations defined in the templated Graph class.
 */
class InterferenceGraph : public Graph<int> {
private:
    std::vector<Web> webs;                           /**< @brief List of all Webs created (graph nodes). */
    std::map<int, std::set<int>> adjList;            /**< @brief Adjacency list: maps a Web ID to a set of neighboring Web IDs (interfering webs). */

    /** @brief Checks whether two live ranges overlap at any program line. 
     * @note Time complexity: O(L log L) where L is the max lines.
     */
    static bool rangesIntersect(const LiveRange& a, const LiveRange& b);
    /** @brief Groups continuous LiveRanges into Web structures. 
     * @note Time complexity: O(R^2 * L log L) where R is the number of LiveRanges.
     */
    void buildWebs(const std::vector<LiveRange>& liveRanges);
    /** @brief Builds the interference adjacency list from the constructed webs. 
     * @note Time complexity: O(W^2 * L log L) where W is the number of Webs.
     */
    void buildInterferences();

public:
    /**
     * @brief Construct an InterferenceGraph and build its webs/interferences.
     * @param liveRanges Vector of LiveRanges parsed from the input.
     * @note Total constructor complexity: O(R^2 * L log L + W^2 * L log L).
     */
    InterferenceGraph(const std::vector<LiveRange>& liveRanges);

    /** @return Const reference to the list of webs. */
    const std::vector<Web>& getWebs() const { return webs; }
    /** @return Const reference to the interference adjacency list. */
    const std::map<int, std::set<int>>& getAdjList() const { return adjList; }

    /** @brief Prints the interference graph to standard output. */
    void printGraph() const;
};

#endif // INTERFERENCEGRAPH_H
