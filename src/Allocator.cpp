#include "../include/Allocator.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <stack>

using namespace std;

Allocator::Allocator(const Graph& g, const ConfigRegisters& c) : graph(g), config(c) {
    isFeasible = true;
    finalWebs =
        graph.getWebs();  // Copy the original webs so they can be modified during splitting.

    for (const auto& web : finalWebs) {
        allocation[web.id] = -1;
    }
}

void Allocator::run() {
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

bool Allocator::spillNode(const std::map<int, std::set<int>>& originalAdjList,
                          std::map<int, bool>& inGraph, int& nodesRemaining, int& spilledNodeId) {
    spilledNodeId = -1;
    int maxDegree = -1;

    for (const auto& pair : originalAdjList) {
        if (!inGraph[pair.first]) continue;

        int degree = 0;
        for (int neighbor : pair.second)
            if (inGraph[neighbor]) degree++;

        if (degree > maxDegree) {
            maxDegree = degree;
            spilledNodeId = pair.first;
        }
    }

    if (spilledNodeId == -1) return false;

    inGraph[spilledNodeId] = false;
    allocation[spilledNodeId] = -1;
    nodesRemaining--;
    return true;
}

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

bool Allocator::runBasicAllocation() {
    int N = config.numRegisters;
    const auto& originalAdjList = graph.getAdjList();
    map<int, bool> inGraph;
    int nodesRemaining = finalWebs.size();

    for (const auto& web : finalWebs) inGraph[web.id] = true;
    stack<int> S;

    while (nodesRemaining > 0) {
        bool found = false;
        found = simplifyFindAndRemove(originalAdjList, inGraph, N, S, nodesRemaining); // Try to find a node with degree < N and remove it.

        // If no node has degree < N, choose one node to spill and keep simplifying.
        if (!found) {
            int spilledNodeId = -1;
            if (!spillNode(originalAdjList, inGraph, nodesRemaining, spilledNodeId)) return false;
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
            cout << "-> Spilling applied to Web " << nodeToSpill << endl;
        }
    }

    return colorNodes(S, originalAdjList, N);
}

bool Allocator::runSplittingAllocation() {
    int maxSplits = config.k_param;
    int splitsUsed = 0;
    int N = config.numRegisters;

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
            return colorNodes(S, currentAdjList, N);
        }

        // 4. If we got stuck, we must split.
        if (splitsUsed >= maxSplits) return false;  // We have exhausted the splits.

        // Find the web with the highest degree that has at least 2 lines.
        int nodeToSplitId = -1, maxDegree = -1, splitIdx = -1;
        for (size_t i = 0; i < finalWebs.size(); ++i) {
            int id = finalWebs[i].id;
            int degree = currentAdjList[id].size();
            if (degree > maxDegree && finalWebs[i].lines.size() >= 2) {
                maxDegree = degree;
                nodeToSplitId = id;
                splitIdx = i;
            }
        }

        if (nodeToSplitId == -1) return false;  // No splittable web found.

        // Perform the split.
        Web original = finalWebs[splitIdx];
        finalWebs.erase(finalWebs.begin() + splitIdx);  // Remove the original web.

        Web w1, w2;
        // Find the next available ID.
        int nextId = 0;
        for (const auto& w : finalWebs)
            if (w.id >= nextId) nextId = w.id + 1;

        w1.id = nextId++;
        w2.id = nextId++;
        w1.variableName = original.variableName;
        w2.variableName = original.variableName;

        // Split the lines in half.
        int half = original.lines.size() / 2;
        int count = 0;
        for (int line : original.lines) {
            if (count < half) {
                w1.lines.insert(line);
                if (original.defLines.find(line) != original.defLines.end())
                    w1.defLines.insert(line);
                if (original.useEndLines.find(line) != original.useEndLines.end())
                    w1.useEndLines.insert(line);
            } else {
                w2.lines.insert(line);
                if (original.defLines.find(line) != original.defLines.end())
                    w2.defLines.insert(line);
                if (original.useEndLines.find(line) != original.useEndLines.end())
                    w2.useEndLines.insert(line);
            }
            count++;
        }

        finalWebs.push_back(w1);
        finalWebs.push_back(w2);
        splitsUsed++;

        cout << "-> Splitting applied to Web " << original.id << " (split into Web " << w1.id
             << " and Web " << w2.id << ")" << endl;

        // The while(true) loop repeats and tries to allocate the new graph again.
    }
}

bool Allocator::runFreeAllocation() {
    int N = config.numRegisters;
    int maxSplits = 3;  // Our internal split budget.
    int splitsUsed = 0;

    set<int> spilledWebs;  // Webs that we were forced to move to memory.

    // Main loop that guarantees the compiler keeps going.
    while (true) {
        // 1. Rebuild the adjacency list, ignoring the webs already moved to memory.
        map<int, set<int>> currentAdjList;
        rebuildAdjacency(currentAdjList, &spilledWebs);

        // 2. Simplification phase.
        map<int, bool> inGraph;
        int nodesRemaining = 0;
        for (const auto& web : finalWebs) {
            if (!spilledWebs.count(web.id)) {
                inGraph[web.id] = true;
                nodesRemaining++;
            }
        }

        stack<int> S;
        bool stuck = false;

        while (nodesRemaining > 0) {
            bool found = simplifyFindAndRemove(currentAdjList, inGraph, N, S, nodesRemaining);
            if (!found) {
                stuck = true;
                break;
            }  // Stuck!
        }

        // 3. If we did not get stuck, perform the final coloring.
        if (!stuck) {
            allocation.clear();
            for (const auto& web : finalWebs) allocation[web.id] = -1;  // Initial reset.
            return colorNodes(S, currentAdjList, N);  // Absolute success!
        }

        // 4. If we got stuck -> PLAN A: try splitting.
        int nodeToSplitId = -1, maxDegreeSplit = -1, splitIdx = -1;
        if (splitsUsed < maxSplits) {
            for (size_t i = 0; i < finalWebs.size(); ++i) {
                int id = finalWebs[i].id;
                if (spilledWebs.count(id) || !inGraph[id]) continue;

                int degree = 0;
                for (int neighbor : currentAdjList[id])
                    if (inGraph[neighbor]) degree++;

                // It must have the highest interference and at least 2 code lines.
                if (degree > maxDegreeSplit && finalWebs[i].lines.size() >= 2) {
                    maxDegreeSplit = degree;
                    nodeToSplitId = id;
                    splitIdx = i;
                }
            }
        }

        if (nodeToSplitId != -1) {
            // Execute the split.
            Web original = finalWebs[splitIdx];
            finalWebs.erase(finalWebs.begin() + splitIdx);

            Web w1, w2;
            int nextId = 0;
            for (const auto& w : finalWebs)
                if (w.id >= nextId) nextId = w.id + 1;

            w1.id = nextId++;
            w2.id = nextId++;
            w1.variableName = original.variableName;
            w2.variableName = original.variableName;

            int half = original.lines.size() / 2;
            int count = 0;
            for (int line : original.lines) {
                if (count < half) {
                    w1.lines.insert(line);
                    if (original.defLines.find(line) != original.defLines.end())
                        w1.defLines.insert(line);
                    if (original.useEndLines.find(line) != original.useEndLines.end())
                        w1.useEndLines.insert(line);
                } else {
                    w2.lines.insert(line);
                    if (original.defLines.find(line) != original.defLines.end())
                        w2.defLines.insert(line);
                    if (original.useEndLines.find(line) != original.useEndLines.end())
                        w2.useEndLines.insert(line);
                }
                count++;
            }

            finalWebs.push_back(w1);
            finalWebs.push_back(w2);
            splitsUsed++;
            cout << "-> [Free Mode] Splitting applied on the Web " << original.id << endl;
            continue;  // Restart the loop with the new webs.
        }

        // 5. If splitting fails or runs out -> PLAN B: spilling.
        int nodeToSpillId = -1, maxDegreeSpill = -1;
        for (size_t i = 0; i < finalWebs.size(); ++i) {
            int id = finalWebs[i].id;
            if (spilledWebs.count(id) || !inGraph[id]) continue;

            int degree = 0;
            for (int neighbor : currentAdjList[id])
                if (inGraph[neighbor]) degree++;

            if (degree > maxDegreeSpill) {
                maxDegreeSpill = degree;
                nodeToSpillId = id;
            }
        }

        if (nodeToSpillId != -1) {
            spilledWebs.insert(nodeToSpillId);  // Move it to memory.
            cout << "-> [Free Mode] Spilling applied on the Web " << nodeToSpillId << endl;
            continue;  // Restart the loop.
        }

        return false;  // This should never be reached, but it is kept for safety.
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
    cout << "Results exported to: " << filename << endl;
}
