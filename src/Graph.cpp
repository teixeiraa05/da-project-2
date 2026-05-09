#include "../include/Graph.h"
#include <iostream>

using namespace std;

Graph::Graph(const vector<LiveRange>& liveRanges) {
    buildWebs(liveRanges);
    buildInterferences();
}

// Helper method to check if two live ranges intersect (share at least one line)
bool Graph::rangesIntersect(const LiveRange& a, const LiveRange& b) {
    for (int line : a.lines) {
        if (b.lines.find(line) != b.lines.end()) {
            return true;
        }
    }
    return false;
}

void Graph::buildWebs(const vector<LiveRange>& liveRanges) {
    int webCounter = 0;
    
    // Step 1: Group LiveRanges by variable name
    map<string, vector<LiveRange>> varRanges;
    for (const auto& lr : liveRanges) {
        varRanges[lr.variableName].push_back(lr);
    }

    // Step 2: For each variable, merge ranges that intersect
    for (auto& pair : varRanges) {
        string varName = pair.first;
        vector<LiveRange>& ranges = pair.second;

        bool merged;
        do {
            merged = false;
            for (size_t i = 0; i < ranges.size(); ++i) {
                for (size_t j = i + 1; j < ranges.size(); ++j) {

                    if (rangesIntersect(ranges[i], ranges[j])) {
                        // Merge range j into range i
                        ranges[i].lines.insert(ranges[j].lines.begin(), ranges[j].lines.end());
                        ranges[i].defLines.insert(ranges[j].defLines.begin(), ranges[j].defLines.end());
                        ranges[i].useEndLines.insert(ranges[j].useEndLines.begin(), ranges[j].useEndLines.end());
                        // Erase range j (it has been merged)
                        ranges.erase(ranges.begin() + j);
                        merged = true;
                        break; // Break inner loop and restart (indices changed)
                    }
                }
                if (merged) break; // Recomeça o ciclo 'do-while'
            }
        } while (merged);

        // Step 3: Transform the final (merged) ranges into actual Webs
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

void Graph::buildInterferences() {
    for (size_t i = 0; i < webs.size(); ++i) {
        // Ensure the Web exists in the adjacency list, even if it has no edges (degree 0)
        adjList[webs[i].id]; 
        
        for (size_t j = i + 1; j < webs.size(); ++j) {
            // Check interference using the helper method defined in DataTypes.h
            if (webs[i].interferesWith(webs[j])) {
                // Adicionar a aresta em ambas as direções (grafo não direcionado)
                adjList[webs[i].id].insert(webs[j].id);
                adjList[webs[j].id].insert(webs[i].id);
            }
        }
    }
}

void Graph::printGraph() const {
    cout << "\n=== Interference Graph (Webs: " << webs.size() << ") ===" << endl;
    for (const auto& web : webs) {
        cout << "Web " << web.id << " (" << web.variableName << "): [ ";
        for (int line : web.lines) cout << line << " ";
        cout << "] -> Interferes with: { ";
        for (int neighbor : adjList.at(web.id)) {
            cout << "Web" << neighbor << " ";
        }
        cout << "}" << endl;
    }
    cout << "======================================\n" << endl;
}