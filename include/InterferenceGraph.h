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

    static bool rangesIntersect(const LiveRange& a, const LiveRange& b);
    void buildWebs(const std::vector<LiveRange>& liveRanges);
    void buildInterferences();

public:
    /**
     * @brief Construct an InterferenceGraph and build its webs/interferences.
     * @param liveRanges Vector of LiveRanges parsed from the input.
     */
    InterferenceGraph(const std::vector<LiveRange>& liveRanges);

    const std::vector<Web>& getWebs() const { return webs; }
    const std::map<int, std::set<int>>& getAdjList() const { return adjList; }

    void printGraph() const;
};

#endif // INTERFERENCEGRAPH_H
