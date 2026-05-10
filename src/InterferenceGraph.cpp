#include "../include/InterferenceGraph.h"

#include <iostream>

using namespace std;

InterferenceGraph::InterferenceGraph(const vector<LiveRange>& liveRanges) {
    buildWebs(liveRanges);
    buildInterferences();

    // Populate base Graph<int> with vertices and edges for helper operations
    for (const auto &w : webs) {
        addVertex(w.id);
    }
    for (const auto &pair : adjList) {
        int src = pair.first;
        for (int dst : pair.second) {
            // addEdge will silently fail if vertex missing; vertices were added above
            addEdge(src, dst, 1.0);
        }
    }
}

bool InterferenceGraph::rangesIntersect(const LiveRange& a, const LiveRange& b) {
    for (int line : a.lines) {
        if (b.lines.find(line) != b.lines.end()) {
            return true;
        }
    }
    return false;
}

void InterferenceGraph::buildWebs(const vector<LiveRange>& liveRanges) {
    int webCounter = 0;

    // Group LiveRanges by variable name
    map<string, vector<LiveRange>> varRanges;
    for (const auto& lr : liveRanges) {
        varRanges[lr.variableName].push_back(lr);
    }

    // Merge overlapping ranges per variable
    for (auto& pair : varRanges) {
        string varName = pair.first;
        vector<LiveRange>& ranges = pair.second;

        bool merged;
        do {
            merged = false;
            for (size_t i = 0; i < ranges.size(); ++i) {
                for (size_t j = i + 1; j < ranges.size(); ++j) {
                    if (rangesIntersect(ranges[i], ranges[j])) {
                        ranges[i].lines.insert(ranges[j].lines.begin(), ranges[j].lines.end());
                        ranges[i].defLines.insert(ranges[j].defLines.begin(), ranges[j].defLines.end());
                        ranges[i].useEndLines.insert(ranges[j].useEndLines.begin(), ranges[j].useEndLines.end());
                        ranges.erase(ranges.begin() + j);
                        merged = true;
                        break;
                    }
                }
                if (merged) break;
            }
        } while (merged);

        for (const auto& r : ranges) {
            Web newWeb;
            newWeb.id = webCounter++;
            newWeb.variableName = varName;
            newWeb.lines = r.lines;
            newWeb.defLines = r.defLines;
            newWeb.useEndLines = r.useEndLines;
            webs.push_back(newWeb);
        }
    }
}

void InterferenceGraph::buildInterferences() {
    for (size_t i = 0; i < webs.size(); ++i) {
        adjList[webs[i].id];
        for (size_t j = i + 1; j < webs.size(); ++j) {
            if (webs[i].interferesWith(webs[j])) {
                adjList[webs[i].id].insert(webs[j].id);
                adjList[webs[j].id].insert(webs[i].id);
            }
        }
    }
}

void InterferenceGraph::printGraph() const {
    cout << "\n=== Interference Graph (Webs: " << webs.size() << ") ===" << endl;
    for (const auto& web : webs) {
        cout << "Web " << web.id << " (" << web.variableName << "): [ ";
        for (int line : web.lines) cout << line << " ";
        cout << "] -> Interferes with: { ";
        auto it = adjList.find(web.id);
        if (it != adjList.end()) {
            for (int neighbor : it->second) {
                cout << "Web" << neighbor << " ";
            }
        }
        cout << " }" << endl;
    }
    cout << "======================================\n" << endl;
}
