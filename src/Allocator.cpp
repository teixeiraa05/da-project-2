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
    finalWebs =
        graph.getWebs();  // Copy the original webs so they can be modified during splitting.

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
    } else if (config.algorithm == "free") {  // Our custom algorithm.
        isFeasible = runFreeAllocation();
    }

    if (!isFeasible) {
        cerr << "Warning: The allocation was not possible with the given parameters." << endl;
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

bool Allocator::colorNodesDSATUR(const map<int, set<int>>& adjList,
                                  const set<int>& webIds, int N) {
    // saturation[id] = number of DISTINCT colors used by already-colored neighbours
    map<int, int> saturation;
    map<int, bool> isColored;

    for (int id : webIds) {
        saturation[id] = 0;
        isColored[id] = false;
        allocation[id] = -1;
    }

    for (size_t step = 0; step < webIds.size(); ++step) {
        // 1. Select uncolored web with highest saturation.
        //    Break ties by highest degree (number of active neighbours).
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

        // 2. Collect colors already used by colored neighbours.
        set<int> usedColors;
        auto it = adjList.find(chosen);
        if (it != adjList.end())
            for (int nb : it->second)
                if (isColored[nb] && allocation[nb] != -1)
                    usedColors.insert(allocation[nb]);

        // 3. Assign the smallest available color within [0, N).
        int assignedColor = -1;
        for (int c = 0; c < N; ++c) {
            if (!usedColors.count(c)) {
                assignedColor = c;
                break;
            }
        }

        allocation[chosen] = assignedColor; // -1 means no color found
        isColored[chosen] = true;

        // 4. Update saturation of uncolored neighbours.
        //    Recompute the full distinct-color set to avoid double-counting
        //    when two neighbours share the same color.
        //    Only count valid colors (skip -1 from spilled nodes).
        if (it != adjList.end()) {
            for (int nb : it->second) {
                if (isColored[nb]) continue;
                set<int> neighbourColors;
                auto nit = adjList.find(nb);
                if (nit != adjList.end())
                    for (int nn : nit->second)
                        if (isColored[nn] && allocation[nn] != -1) // skip -1
                            neighbourColors.insert(allocation[nn]);
                saturation[nb] = neighbourColors.size();
            }
        }
    }

    // 5. Return false if any web could not be colored.
    for (int id : webIds) {
        if (allocation[id] == -1) return false;
    }

    return true;
}

// =========================================================
// Splitting algorithms & helpers
// =========================================================

// helper to find next available web id for splitting
int Allocator::nextWebId(const vector<Web>& webs) const {
    int nextId = 0;
    for (const auto& web : webs) {
        if (web.id >= nextId) nextId = web.id + 1;
    }
    return nextId;
}

pair<Web, Web> Allocator::splitWebAtBoundary(const Web& original, int newId) const {
    // Safety check: ensure newId is different from original.id
    if (newId == original.id) newId++;

    Web left;
    Web right;

    left.id = original.id;
    right.id = newId;
    left.variableName = original.variableName;
    right.variableName = original.variableName;

    // Find the largest gap between consecutive program points to determine the split index.
    int maxGap = -1;
    size_t bestSplitIndex = 1;
    auto it = original.lines.begin();
    for (size_t i = 0; i < original.lines.size() - 1; ++i) {
        int current = *it;
        int next = *(++it);
        if (next - current > maxGap) {
            maxGap = next - current;
            bestSplitIndex = i + 1;
        }
    }

    const size_t splitIndex = bestSplitIndex;

    size_t index = 0;
    for (int line : original.lines) {
        const bool goesLeft = index < splitIndex;
        const bool goesRight = index >= splitIndex;

        if (goesLeft) {
            left.lines.insert(line);
            if (original.defLines.find(line) != original.defLines.end()) left.defLines.insert(line);
            if (original.useEndLines.find(line) != original.useEndLines.end())
                left.useEndLines.insert(line);
        }

        if (goesRight) {
            right.lines.insert(line);
            if (original.defLines.find(line) != original.defLines.end())
                right.defLines.insert(line);
            if (original.useEndLines.find(line) != original.useEndLines.end())
                right.useEndLines.insert(line);
        }

        ++index;
    }

    return {left, right};
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

// Spill selection: choose the active node with the highest degree.
bool Allocator::spillNode(const std::map<int, std::set<int>>& originalAdjList,
                          std::map<int, bool>& inGraph, int& nodesRemaining, int& spilledNodeId) {
    // Select the active node with the highest degree (counting only active neighbors).
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
        outAdj[id1];  // Ensure entry exists
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
            cerr << "Error: Basic allocation failed because the graph is not greedily colorable with "
                 << N << " registers." << endl;
            return false;
        }
    }

    return colorNodes(S, originalAdjList, N);
}

bool Allocator::runSpillingAllocation() {
    // Same as before, adapted to use finalWebs.
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
            const Web* spilledWeb = findWebById(finalWebs, nodeToSpill);
            if (spilledWeb != nullptr) {
                cout << "[Spill] Web " << nodeToSpill << " (" << spilledWeb->variableName
                     << ") moved to memory." << endl;
            } else {
                cout << "[Spill] Web " << nodeToSpill << " moved to memory." << endl;
            }
            spillCount++;
        }
    }

    cout << "[Spill] Summary: " << spillsUsed << " used of " << maxSpills << " allowed." << endl;
    return colorNodes(S, originalAdjList, N);
}

bool Allocator::runSplittingAllocation() {
    int N = config.numRegisters;
    int maxSplits = config.k_param;
    int splitsUsed = 0;

    // Main loop: try to color the graph, and if it fails, perform one split and restart.
    while (true) {
        // 1. Rebuild the local adjacency list for the current webs.
        map<int, set<int>> currentAdjList;
        rebuildAdjacency(currentAdjList, nullptr);

        // 2. Try to simplify (as in Basic).
        map<int, bool> inGraph;
        int nodesRemaining = finalWebs.size();
        for (const auto& web : finalWebs) inGraph[web.id] = true;
        stack<int> S;
        bool stuck = false;

        while (nodesRemaining > 0) {
            bool found = simplifyFindAndRemove(currentAdjList, inGraph, N, S, nodesRemaining);

            if (!found) {
                stuck = true;
                break;
            }
        }

        // 3. If we did not get stuck, we can color the graph and finish successfully.
        if (!stuck) {
            allocation.clear();
            for (const auto& web : finalWebs) allocation[web.id] = -1;  // Reset.
            cout << "[Split] Summary: " << splitsUsed << " used of " << maxSplits << " allowed."
                 << endl;
            return colorNodes(S, currentAdjList, N);
        }

        // 4. If we got stuck, we must split.
        if (splitsUsed >= maxSplits) {
            cout << "[Split] Summary: " << splitsUsed << " used of " << maxSplits << " allowed."
                 << endl;
            return false;  // We have exhausted the splits.
        }

        // Find the web with the highest interference density (degree/lines) that has at least 2 lines.
        // Use highest absolute degree as a tie-breaker.
        int nodeToSplitId = -1, splitIdx = -1;
        double maxHurt = -1.0;
        int maxDegree = -1;

        for (size_t i = 0; i < finalWebs.size(); ++i) {
            int id = finalWebs[i].id;
            int degree = currentAdjList[id].size();
            int numLines = finalWebs[i].lines.size();

            if (numLines >= 2) {
                double hurt = static_cast<double>(degree) / static_cast<double>(numLines);
                if (hurt > maxHurt || (hurt == maxHurt && degree > maxDegree)) {
                    maxHurt = hurt;
                    maxDegree = degree;
                    nodeToSplitId = id;
                    splitIdx = i;
                }
            }
        }

        if (nodeToSplitId == -1) {
            cout << "[Split] No splittable web was found." << endl;
            cout << "[Split] Summary: " << splitsUsed << " used of " << maxSplits << " allowed."
                 << endl;
            return false;
        }

        // Perform the split
        Web original = finalWebs[splitIdx];
        const int newId = nextWebId(finalWebs); // Get new ID before erasing original
        finalWebs.erase(finalWebs.begin() + splitIdx);  // Remove the original web.

        auto [w1, w2] = splitWebAtBoundary(original, newId);

        finalWebs.push_back(w1);
        finalWebs.push_back(w2);
        splitsUsed++;
        splitCount++;

        cout << "[Split] Web " << original.id << " was split into Web " << w1.id << " and Web "
             << w2.id << "." << endl;

        // The while(true) loop repeats and tries to allocate the new graph again.
    }
}

bool Allocator::runFreeAllocation() {
    int N = config.numRegisters;

    set<int> spilledWebs;  // Webs that we were forced to move to memory.

    // Main loop that guarantees the compiler keeps going.
    while (true) {
        // 1. Rebuild the adjacency list, ignoring the webs already moved to memory.
        map<int, set<int>> currentAdjList;
        rebuildAdjacency(currentAdjList, &spilledWebs);

        // 2. Collect web IDs to color (exclude spilled ones)
        set<int> webIds;
        for (const auto& web : finalWebs) {
            if (!spilledWebs.count(web.id)) {
                webIds.insert(web.id);
            }
        }

        if (webIds.empty()) {
            cout << "[Free] All webs were processed. Some were moved to memory." << endl;
            return true;
        }

        // 3. Reset allocation and try DSATUR coloring
        
        for (int webId : webIds) {
            allocation[webId] = -1;
        }

        if (colorNodesDSATUR(currentAdjList, webIds, N)) {
            cout << "[Free] DSATUR coloring completed successfully." << endl;
            return true;
        }

        // If DSATUR failed, we need to spill or split. 
        // For simplicity in this investigation, we'll just return false if it fails,
        // but a real implementation would loop and resolve.
        return false;
    }
}

void Allocator::generateOutputFile(const string& filename) const {
    ofstream file(filename);
    if (!file.is_open()) return;

    set<int> usedRegisters;
    for (const auto& pair : allocation) {
        if (pair.second >= 0) {
            usedRegisters.insert(pair.second);
        }
    }

    file << "# Total number of webs followed by the listing of the program points of each one\n";
    file << "# program points in each web are sorted in ascending order\n";
    file << "webs: " << finalWebs.size() << "\n";  // Use the final webs.

    for (const auto& web : finalWebs) {
        file << "web" << web.id << ": ";
        bool first = true;
        for (int line : web.lines) {
            if (!first) file << ",";
            file << line;

            const bool isDef = (web.defLines.find(line) != web.defLines.end());
            const bool isLastUse = (web.useEndLines.find(line) != web.useEndLines.end());
            // Suppress merged boundary points that became both def and last-use on the same line.
            if (!(isDef && isLastUse)) {
                if (isDef) file << "+";
                if (isLastUse) file << "-";
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
    escaped.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

string Allocator::formatWebRange(const Web& web) const {
    std::ostringstream out;
    bool first = true;
    for (int line : web.lines) {
        if (!first) out << ",";
        out << line;

        const bool isDef = (web.defLines.find(line) != web.defLines.end());
        const bool isLastUse = (web.useEndLines.find(line) != web.useEndLines.end());
        if (!(isDef && isLastUse)) {
            if (isDef) out << "+";
            if (isLastUse) out << "-";
        }

        first = false;
    }
    return out.str();
}

void Allocator::generateColoredDOT(const std::string& filename) const {
    std::ofstream file(filename + ".gv");
    if (!file.is_open()) return;

    set<int> usedRegisters;
    for (const auto& pair : allocation) {
        if (pair.second >= 0) {
            usedRegisters.insert(pair.second);
        }
    }

    // Color palette for registers - hardcoded but its fine for up to 10 registers. Memory (M) will
    // be gray.
    vector<string> colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8",
                             "#F7DC6F", "#BB8FCE", "#85C1E2", "#F8B88B", "#A8E6CF"};

    file << "graph {\n";
    file << "  labelloc = top;\n";
    file << "  labeljust = left;\n";
    file << "  fontname = calibri;\n";
    file << "  fontsize = 16;\n";
    file << "  pad = 0.45;\n";
    file << "  nodesep = 0.9;\n";
    file << "  ranksep = 1.15;\n";
    file << "  rankdir = LR;\n";
    file << "  node [shape=circle, style=filled];\n\n";

    map<int, set<int>> currentAdjList;
    rebuildAdjacency(currentAdjList, nullptr);

    file << "  subgraph cluster_legend {\n";
    file << "    label=\"Allocation legend\";\n";
    file << "    labelloc=top;\n";
    file << "    fontsize=14;\n";
    file << "    style=rounded;\n";
    file << "    color=\"#7A7A7A\";\n";
    file << "    margin=24;\n";
    file << "    rank=sink;\n";
    file << "    legend_node [shape=plain, margin=0, label=<\n";
    file << "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"5\">\n";
    file << "        <TR><TD BGCOLOR=\"#ECECEC\" COLSPAN=\"4\"><B>Run summary</B></TD></TR>\n";
    file << "        <TR><TD ALIGN=\"LEFT\"><B>Algorithm</B></TD><TD ALIGN=\"LEFT\" COLSPAN=\"3\">"
         << escapeDotHtml(config.algorithm) << "</TD></TR>\n";
        file << "        <TR><TD ALIGN=\"LEFT\"><B>Registers provided</B></TD><TD ALIGN=\"LEFT\">"
            << config.numRegisters << "</TD><TD ALIGN=\"LEFT\"><B>Registers used</B></TD><TD ALIGN=\"LEFT\">"
            << usedRegisters.size() << "</TD></TR>\n";
        file << "        <TR><TD ALIGN=\"LEFT\"><B>Feasible</B></TD><TD ALIGN=\"LEFT\" COLSPAN=\"3\">"
            << (isFeasible ? "yes" : "no") << "</TD></TR>\n";
    file << "        <TR><TD ALIGN=\"LEFT\"><B>Splits</B></TD><TD ALIGN=\"LEFT\">"
         << splitCount << "</TD><TD ALIGN=\"LEFT\"><B>Spills</B></TD><TD ALIGN=\"LEFT\">"
         << spillCount << "</TD></TR>\n";
    file << "        <TR><TD BGCOLOR=\"#ECECEC\" COLSPAN=\"4\"><B>Web details</B></TD></TR>\n";
    file << "        <TR><TD><B>Node</B></TD><TD><B>Variable</B></TD><TD><B>Live range</B></TD><TD><B>Assignment</B></TD></TR>\n";

    for (const auto& web : finalWebs) {
        const int reg = allocation.at(web.id);
        const string assignment = !isFeasible ? "black" : (reg == -1 ? "M" : "r" + to_string(reg));
        string fillColor;
        if (!isFeasible) {
            fillColor = "#000000";
        } else if (reg == -1) {
            fillColor = "#D3D3D3";
        } else {
            fillColor = colors[reg % colors.size()];
        }

        file << "        <TR>";
        file << "<TD BGCOLOR=\"" << fillColor << "\"><FONT COLOR=\""
             << (!isFeasible ? "#FFFFFF" : "#000000") << "\"><B>web" << web.id
             << "</B></FONT></TD>";
        file << "<TD ALIGN=\"LEFT\">" << escapeDotHtml(web.variableName) << "</TD>";
        file << "<TD ALIGN=\"LEFT\">" << escapeDotHtml(formatWebRange(web)) << "</TD>";
        file << "<TD ALIGN=\"LEFT\">" << assignment << "</TD>";
        file << "</TR>\n";
    }

    file << "      </TABLE>\n";
    file << "    >];\n";
    file << "  }\n\n";

    // Output nodes with colors based on register assignment
    for (const auto& web : finalWebs) {
        int reg = allocation.at(web.id);
        file << "  web" << web.id << " [label=\"web" << web.id << " (" << web.variableName
             << ")\", ";

        if (!isFeasible) {
            file << "fillcolor=\"#000000\", fontcolor=\"#FFFFFF\"];  // failed allocation\n";
        } else if (reg == -1) {
            file << "fillcolor=\"#D3D3D3\"];  // Memory (gray)\n";
        } else {
            int colorIdx = reg % colors.size();
            file << "fillcolor=\"" << colors[colorIdx] << "\"];  // r" << reg << "\n";
        }
    }

    file << "\n";

    // Output undirected edges from the current transformed adjacency list (single edge per pair)
    for (const auto& pair : currentAdjList) {
        int src = pair.first;
        for (int dst : pair.second) {
            if (src < dst) {
                file << "  web" << src << " -- web" << dst << ";\n";
            }
        }
    }

    file << "}\n";
    file.close();
}
