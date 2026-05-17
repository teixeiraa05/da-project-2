/**
 * @file Allocator.cpp
 * @brief Implementation of graph-coloring register allocation strategies.
 */
#include "../include/Allocator.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stack>
#include <utility>

using namespace std;

Allocator::Allocator(const InterferenceGraph& g, const ConfigRegisters& c) : graph(g), config(c) {
    isFeasible = true;
    splitCount = 0;
    spillCount = 0;
    finalWebs = graph.getWebs();

    for (const auto& web : finalWebs) {
        allocation[web.id] = -1;
    }
}

void Allocator::run() {
    splitCount = 0;
    spillCount = 0;

    if (config.algorithm == "basic") {
        isFeasible = runBasicAllocation();
    } else if (config.algorithm == "spilling") {
        isFeasible = runSpillingAllocation();
    } else if (config.algorithm == "splitting") {
        isFeasible = runSplittingAllocation();
    } else if (config.algorithm == "free") {
        isFeasible = runFreeAllocation();
    }

    if (!isFeasible) {
        cerr << "Warning: Allocation not possible with given parameters." << endl;
        config.numRegisters = 0;
        for (const auto& web : finalWebs) {
            allocation[web.id] = -1;
        }
    }
}

// =========================================================
// Coloring algorithms
// =========================================================

bool Allocator::colorNodes(std::stack<int>& S, const std::map<int, std::set<int>>& adjList, int N) {
    while (!S.empty()) {
        int n = S.top();
        S.pop();

        set<int> usedColors;
        auto nodeIt = adjList.find(n);
        if (nodeIt != adjList.end()) {
            for (int neighbor : nodeIt->second) {
                auto allocIt = allocation.find(neighbor);
                if (allocIt != allocation.end() && allocIt->second != -1) {
                    usedColors.insert(allocIt->second);
                }
            }
        }

        bool assigned = false;
        for (int c = 0; c < N; ++c) {
            if (usedColors.find(c) == usedColors.end()) {
                allocation[n] = c;
                assigned = true;
                break;
            }
        }

        if (!assigned) {
            allocation[n] = -1;
            return false;
        }
    }
    return true;
}

bool Allocator::colorNodesDSATUR(const map<int, set<int>>& adjList, const set<int>& webIds, int N) {
    map<int, int> saturation;
    map<int, bool> isColored;

    for (int id : webIds) {
        saturation[id] = 0;
        isColored[id] = false;
        allocation[id] = -1;
    }

    for (size_t step = 0; step < webIds.size(); ++step) {
        int chosen = -1;
        int maxSat = -1;
        int maxDeg = -1;

        for (int id : webIds) {
            if (isColored[id]) continue;
            int sat = saturation[id];
            int deg = 0;
            auto it = adjList.find(id);
            if (it != adjList.end())
                for (int nb : it->second)
                    if (webIds.count(nb)) deg++;

            if (sat > maxSat || (sat == maxSat && deg > maxDeg)) {
                maxSat = sat;
                maxDeg = deg;
                chosen = id;
            }
        }

        if (chosen == -1) break;

        set<int> usedColors;
        auto it = adjList.find(chosen);
        if (it != adjList.end())
            for (int nb : it->second)
                if (isColored[nb] && allocation[nb] != -1) usedColors.insert(allocation[nb]);

        int assignedColor = -1;
        for (int c = 0; c < N; ++c) {
            if (!usedColors.count(c)) {
                assignedColor = c;
                break;
            }
        }

        allocation[chosen] = assignedColor;
        isColored[chosen] = true;

        if (it != adjList.end()) {
            for (int nb : it->second) {
                if (isColored[nb]) continue;
                set<int> neighbourColors;
                auto nit = adjList.find(nb);
                if (nit != adjList.end())
                    for (int nn : nit->second)
                        if (isColored[nn] && allocation[nn] != -1)
                            neighbourColors.insert(allocation[nn]);
                saturation[nb] = neighbourColors.size();
            }
        }
    }

    for (int id : webIds) {
        if (allocation[id] == -1) return false;
    }
    return true;
}

// =========================================================
// Splitting algorithms & helpers
// =========================================================

int Allocator::nextWebId(const vector<Web>& webs) const {
    int nextId = 0;
    for (const auto& web : webs) {
        if (web.id >= nextId) nextId = web.id + 1;
    }
    return nextId;
}

std::pair<Web, Web> Allocator::splitWebAtPoint(const Web& original, int newId,
                                               size_t splitIndex) const {
    Web left;
    Web right;
    left.id = original.id;
    right.id = newId;
    left.variableName = original.variableName;
    right.variableName = original.variableName;

    size_t index = 0;
    for (int line : original.lines) {
        if (index < splitIndex) {
            left.lines.insert(line);
            if (original.defLines.count(line)) left.defLines.insert(line);
            if (original.useEndLines.count(line)) left.useEndLines.insert(line);
        } else {
            right.lines.insert(line);
            if (original.defLines.count(line)) right.defLines.insert(line);
            if (original.useEndLines.count(line)) right.useEndLines.insert(line);
        }
        index++;
    }
    return {left, right};
}

int Allocator::evaluateColorability(const std::vector<Web>& webs, int N) const {
    std::map<int, std::set<int>> adj;
    for (size_t i = 0; i < webs.size(); ++i) {
        adj[webs[i].id];
        for (size_t j = i + 1; j < webs.size(); ++j) {
            if (webs[i].interferesWith(webs[j])) {
                adj[webs[i].id].insert(webs[j].id);
                adj[webs[j].id].insert(webs[i].id);
            }
        }
    }

    std::map<int, bool> inGraph;
    for (const auto& w : webs) inGraph[w.id] = true;
    std::stack<int> S;
    int nodesRemaining = webs.size();

    while (nodesRemaining > 0) {
        bool found = false;
        for (auto& pair : adj) {
            if (!inGraph[pair.first]) continue;
            int deg = 0;
            for (int nb : pair.second)
                if (inGraph[nb]) deg++;
            if (deg < N) {
                inGraph[pair.first] = false;
                S.push(pair.first);
                nodesRemaining--;
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    return nodesRemaining;
}

const Web* Allocator::findWebById(const vector<Web>& webs, int webId) const {
    for (const auto& web : webs) {
        if (web.id == webId) return &web;
    }
    return nullptr;
}

// =========================================================
// Spilling helpers
// =========================================================

bool Allocator::spillNode(const std::map<int, std::set<int>>& originalAdjList,
                          std::map<int, bool>& inGraph, int& nodesRemaining, int& spilledNodeId) {
    spilledNodeId = -1;
    int highestDegree = -1;
    size_t smallestFootprint = numeric_limits<size_t>::max();

    for (const auto& pair : originalAdjList) {
        int nodeId = pair.first;
        if (!inGraph[nodeId]) continue;

        int degree = 0;
        for (int neighbor : pair.second) {
            if (inGraph[neighbor]) degree++;
        }

        const Web* web = findWebById(finalWebs, nodeId);
        if (web == nullptr) continue;
        const size_t footprint = web->defLines.size() + web->useEndLines.size();

        if (degree > highestDegree || (degree == highestDegree && footprint < smallestFootprint)) {
            highestDegree = degree;
            smallestFootprint = footprint;
            spilledNodeId = nodeId;
        }
    }

    if (spilledNodeId == -1) return false;
    inGraph[spilledNodeId] = false;
    allocation[spilledNodeId] = -1;
    nodesRemaining--;
    return true;
}

bool Allocator::simplifyFindAndRemove(const std::map<int, std::set<int>>& adjList,
                                      std::map<int, bool>& inGraph, int N, std::stack<int>& S,
                                      int& nodesRemaining) {
    for (const auto& pair : adjList) {
        if (!inGraph[pair.first]) continue;
        int degree = 0;
        for (int neighbor : pair.second)
            if (inGraph[neighbor]) degree++;
        if (degree < N) {
            inGraph[pair.first] = false;
            S.push(pair.first);
            nodesRemaining--;
            return true;
        }
    }
    return false;
}

void Allocator::rebuildAdjacency(std::map<int, std::set<int>>& outAdj,
                                 const std::set<int>* skipWebs) const {
    for (size_t i = 0; i < finalWebs.size(); ++i) {
        int id1 = finalWebs[i].id;
        if (skipWebs && skipWebs->count(id1)) continue;
        outAdj[id1];
        for (size_t j = i + 1; j < finalWebs.size(); ++j) {
            int id2 = finalWebs[j].id;
            if (skipWebs && skipWebs->count(id2)) continue;
            if (finalWebs[i].interferesWith(finalWebs[j])) {
                outAdj[id1].insert(id2);
                outAdj[id2].insert(id1);
            }
        }
    }
}

// =========================================================
// Allocator routines
// =========================================================

bool Allocator::runBasicAllocation() {
    int N = config.numRegisters;
    const auto& originalAdjList = graph.getAdjList();
    map<int, bool> inGraph;
    int nodesRemaining = finalWebs.size();
    for (const auto& web : finalWebs) inGraph[web.id] = true;
    stack<int> S;
    while (nodesRemaining > 0) {
        if (!simplifyFindAndRemove(originalAdjList, inGraph, N, S, nodesRemaining)) {
            cerr << "Error: Basic allocation failed." << endl;
            return false;
        }
    }
    return colorNodes(S, originalAdjList, N);
}

bool Allocator::runSpillingAllocation() {
    int N = config.numRegisters;
    int maxSpills = config.k_param;
    int spillsUsed = 0;
    const auto& originalAdjList = graph.getAdjList();
    map<int, bool> inGraph;
    int nodesRemaining = finalWebs.size();
    for (const auto& web : finalWebs) inGraph[web.id] = true;
    stack<int> S;
    while (nodesRemaining > 0) {
        bool found = simplifyFindAndRemove(originalAdjList, inGraph, N, S, nodesRemaining);
        if (!found) {
            if (spillsUsed >= maxSpills) return false;
            int nodeToSpill = -1;
            if (!spillNode(originalAdjList, inGraph, nodesRemaining, nodeToSpill)) return false;
            spillsUsed++;
            spillCount++;
        }
    }
    return colorNodes(S, originalAdjList, N);
}

bool Allocator::runSplittingAllocation() {
    int N = config.numRegisters;
    int maxSplits = config.k_param;
    int splitsUsed = 0;

    while (true) {
        map<int, set<int>> currentAdjList;
        rebuildAdjacency(currentAdjList, nullptr);

        map<int, bool> inGraph;
        int nodesRemaining = finalWebs.size();
        for (const auto& web : finalWebs) inGraph[web.id] = true;
        stack<int> S;
        bool stuck = false;

        while (nodesRemaining > 0) {
            if (!simplifyFindAndRemove(currentAdjList, inGraph, N, S, nodesRemaining)) {
                stuck = true;
                break;
            }
        }

        if (!stuck) {
            allocation.clear();
            for (const auto& web : finalWebs) allocation[web.id] = -1;
            return colorNodes(S, currentAdjList, N);
        }

        if (splitsUsed >= maxSplits) return false;

        int nodeToSplitId = -1, splitIdx = -1;
        int maxDegree = -1;
        for (size_t i = 0; i < finalWebs.size(); ++i) {
            int deg = currentAdjList[finalWebs[i].id].size();
            if (finalWebs[i].lines.size() >= 2 && deg > maxDegree) {
                maxDegree = deg;
                nodeToSplitId = finalWebs[i].id;
                splitIdx = i;
            }
        }

        if (nodeToSplitId == -1) return false;

        Web original = finalWebs[splitIdx];
        const int newId = nextWebId(finalWebs);
        size_t bestPoint = 1;
        int minStuck = evaluateColorability(finalWebs, N);  // Base line

        for (size_t p = 1; p < original.lines.size(); ++p) {
            auto [w1, w2] = splitWebAtPoint(original, newId, p);
            vector<Web> trialWebs;
            for (const auto& fw : finalWebs) {
                if (fw.id == original.id) continue;
                trialWebs.push_back(fw);
            }
            trialWebs.push_back(w1);
            trialWebs.push_back(w2);

            int stuckCount = evaluateColorability(trialWebs, N);
            if (stuckCount < minStuck) {
                minStuck = stuckCount;
                bestPoint = p;
            }
            if (minStuck == 0) break;
        }

        finalWebs.erase(finalWebs.begin() + splitIdx);
        auto [w1, w2] = splitWebAtPoint(original, newId, bestPoint);
        finalWebs.push_back(w1);
        finalWebs.push_back(w2);
        splitsUsed++;
        splitCount++;
        cout << "[Split] Web " << original.id << " split at point " << bestPoint << "." << endl;
    }
}

// =========================================================
// Greedy clique finder Helper (for free allocation)
// =========================================================

set<int> Allocator::findGreedyClique(const map<int, set<int>>& adjList,
                                     const set<int>& webIds) const {
    set<int> maxClique;
    for (int startId : webIds) {
        set<int> currentClique;
        currentClique.insert(startId);
        vector<pair<int, int>> neighbors;
        auto it = adjList.find(startId);
        if (it != adjList.end()) {
            for (int nb : it->second) {
                if (webIds.count(nb)) {
                    int deg = 0;
                    auto nit = adjList.find(nb);
                    if (nit != adjList.end())
                        for (int nnb : nit->second)
                            if (webIds.count(nnb)) deg++;
                    neighbors.push_back({nb, deg});
                }
            }
        }
        sort(neighbors.begin(), neighbors.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& nbPair : neighbors) {
            int nb = nbPair.first;
            bool interferesWithAll = true;
            for (int cliqueNode : currentClique) {
                auto cit = adjList.find(cliqueNode);
                if (cit == adjList.end() || cit->second.find(nb) == cit->second.end()) {
                    interferesWithAll = false;
                    break;
                }
            }
            if (interferesWithAll) currentClique.insert(nb);
        }
        if (currentClique.size() > maxClique.size()) maxClique = currentClique;
    }
    return maxClique;
}

bool Allocator::runFreeAllocation() {
    int N = config.numRegisters;
    set<int> spilledWebs;
    int currentRunSplits = 0;
    const int SPLIT_LIMIT = 3;

    while (true) {
        map<int, set<int>> currentAdjList;
        rebuildAdjacency(currentAdjList, &spilledWebs);

        set<int> webIds;
        for (const auto& web : finalWebs) {
            if (!spilledWebs.count(web.id)) webIds.insert(web.id);
        }

        if (webIds.empty()) return true;

        set<int> clique = findGreedyClique(currentAdjList, webIds);
        if (clique.size() > static_cast<size_t>(N)) {
            int victimId = -1, maxDeg = -1;
            for (int id : clique) {
                int deg = currentAdjList[id].size();
                if (deg > maxDeg) {
                    maxDeg = deg;
                    victimId = id;
                }
            }
            spilledWebs.insert(victimId);
            spillCount++;
            continue;
        }

        for (int webId : webIds) allocation[webId] = -1;
        if (colorNodesDSATUR(currentAdjList, webIds, N)) return true;

        int hardestNodeId = -1, maxDeg = -1;
        for (int id : webIds) {
            if (allocation[id] == -1) {
                int deg = currentAdjList[id].size();
                if (deg > maxDeg) {
                    maxDeg = deg;
                    hardestNodeId = id;
                }
            }
        }

        if (hardestNodeId != -1) {
            const Web* w = findWebById(finalWebs, hardestNodeId);
            if (currentRunSplits < SPLIT_LIMIT && w && w->lines.size() >= 2) {
                int splitIdx = -1;
                for (size_t i = 0; i < finalWebs.size(); ++i) {
                    if (finalWebs[i].id == hardestNodeId) {
                        splitIdx = i;
                        break;
                    }
                }
                Web original = finalWebs[splitIdx];
                const int newId = nextWebId(finalWebs);

                size_t bestPoint = 1;
                int minStuck = numeric_limits<int>::max();
                for (size_t p = 1; p < original.lines.size(); ++p) {
                    auto [tw1, tw2] = splitWebAtPoint(original, newId, p);
                    vector<Web> trialWebs;
                    for (const auto& fw : finalWebs) {
                        if (fw.id == original.id || spilledWebs.count(fw.id)) continue;
                        trialWebs.push_back(fw);
                    }
                    trialWebs.push_back(tw1);
                    trialWebs.push_back(tw2);

                    int stuckCount = evaluateColorability(trialWebs, N);
                    if (stuckCount < minStuck) {
                        minStuck = stuckCount;
                        bestPoint = p;
                    }
                    if (minStuck == 0) break;
                }

                finalWebs.erase(finalWebs.begin() + splitIdx);
                auto [w1, w2] = splitWebAtPoint(original, newId, bestPoint);
                finalWebs.push_back(w1);
                finalWebs.push_back(w2);
                currentRunSplits++;
                splitCount++;
                cout << "[Free] Splitting Web " << hardestNodeId << " at point " << bestPoint << "."
                     << endl;
            } else {
                spilledWebs.insert(hardestNodeId);
                spillCount++;
            }
            continue;
        }
        return false;
    }
}

// =========================================================
// File output and DOT generation
// =========================================================

void Allocator::generateOutputFile(const string& filename) const {
    ofstream file(filename);
    if (!file.is_open()) return;
    set<int> usedRegisters;
    for (const auto& pair : allocation)
        if (pair.second >= 0) usedRegisters.insert(pair.second);

    file << "# Total number of webs followed by the listing of the program points of each one\n";
    file << "webs: " << finalWebs.size() << "\n";

    for (const auto& web : finalWebs) {
        file << "web" << web.id << ": ";
        bool first = true;
        for (int line : web.lines) {
            if (!first) file << ",";
            file << line;
            if (!(web.defLines.count(line) && web.useEndLines.count(line))) {
                if (web.defLines.count(line)) file << "+";
                if (web.useEndLines.count(line)) file << "-";
            }
            first = false;
        }
        file << "\n";
    }

    file << "# Total number of registers used, followed by assignment to webs\n";
    file << "registers: " << usedRegisters.size() << "\n";
    for (const auto& web : finalWebs) {
        int reg = allocation.at(web.id);
        if (reg == -1)
            file << "M: web" << web.id << "\n";
        else
            file << "r" << reg << ": web" << web.id << "\n";
    }
    file.close();
    cout << "Allocation report written to: " << filename << endl;
}

string Allocator::escapeDotHtml(const string& text) const {
    string escaped;
    for (char ch : text) {
        if (ch == '&')
            escaped += "&amp;";
        else if (ch == '<')
            escaped += "&lt;";
        else if (ch == '>')
            escaped += "&gt;";
        else if (ch == '"')
            escaped += "&quot;";
        else
            escaped.push_back(ch);
    }
    return escaped;
}

string Allocator::formatWebRange(const Web& web) const {
    std::ostringstream out;
    bool first = true;
    for (int line : web.lines) {
        if (!first) out << ",";
        out << line;
        if (!(web.defLines.count(line) && web.useEndLines.count(line))) {
            if (web.defLines.count(line)) out << "+";
            if (web.useEndLines.count(line)) out << "-";
        }
        first = false;
    }
    return out.str();
}

void Allocator::generateColoredDOT(const std::string& filename) const {
    std::ofstream file(filename + ".gv");
    if (!file.is_open()) return;

    set<int> usedRegisters;
    for (const auto& pair : allocation)
        if (pair.second >= 0) usedRegisters.insert(pair.second);

    vector<string> colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8",
                             "#F7DC6F", "#BB8FCE", "#85C1E2", "#F8B88B", "#A8E6CF"};

    file << "graph {\n  labelloc = top;\n  labeljust = left;\n  fontname = calibri;\n  fontsize = "
            "16;\n";
    file << "  pad = 0.45;\n  nodesep = 0.9;\n  ranksep = 1.15;\n  rankdir = LR;\n";
    file << "  node [shape=circle, style=filled];\n\n";

    map<int, set<int>> currentAdjList;
    rebuildAdjacency(currentAdjList, nullptr);

    file << "  subgraph cluster_legend {\n    label=\"Allocation legend\";\n    legend_node "
            "[shape=plain, label=<\n";
    file << "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";
    file << "        <TR><TD COLSPAN=\"4\"><B>Run summary</B></TD></TR>\n";
    file << "        <TR><TD>Algorithm</TD><TD COLSPAN=\"3\">" << escapeDotHtml(config.algorithm)
         << "</TD></TR>\n";
    file << "        <TR><TD>Registers used</TD><TD>" << usedRegisters.size()
         << "</TD><TD>Feasible</TD><TD>" << (isFeasible ? "yes" : "no") << "</TD></TR>\n";
    file << "        <TR><TD>Splits</TD><TD>" << splitCount << "</TD><TD>Spills</TD><TD>"
         << spillCount << "</TD></TR>\n";
    file << "        <TR><TD COLSPAN=\"4\"><B>Web details</B></TD></TR>\n";
    for (const auto& web : finalWebs) {
        int reg = allocation.at(web.id);
        string c = (reg == -1) ? "#D3D3D3" : colors[reg % colors.size()];
        file << "        <TR><TD BGCOLOR=\"" << c << "\">web" << web.id << "</TD><TD>"
             << escapeDotHtml(web.variableName) << "</TD><TD>" << escapeDotHtml(formatWebRange(web))
             << "</TD><TD>" << (reg == -1 ? "M" : "r" + to_string(reg)) << "</TD></TR>\n";
    }
    file << "      </TABLE>>];\n  }\n";

    for (const auto& web : finalWebs) {
        int reg = allocation.at(web.id);
        string c = (reg == -1) ? "#D3D3D3" : colors[reg % colors.size()];
        file << "  web" << web.id << " [label=\"web" << web.id << " (" << web.variableName
             << ")\", fillcolor=\"" << c << "\"];\n";
    }

    for (const auto& pair : currentAdjList) {
        for (int dst : pair.second)
            if (pair.first < dst) file << "  web" << pair.first << " -- web" << dst << ";\n";
    }
    file << "}\n";
}
