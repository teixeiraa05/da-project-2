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

    /** @brief Checks whether two live ranges overlap at any program line. */
    static bool rangesIntersect(const LiveRange& a, const LiveRange& b);
    /** @brief Groups continuous LiveRanges into Web structures. */
    void buildWebs(const std::vector<LiveRange>& liveRanges);
    /** @brief Builds the interference adjacency list from the constructed webs. */
    void buildInterferences();

public:
    /**
     * @brief Construct an InterferenceGraph and build its webs/interferences.
     * @param liveRanges Vector of LiveRanges parsed from the input.
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
