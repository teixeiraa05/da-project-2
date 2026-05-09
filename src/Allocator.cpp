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
        graph.getWebs();  // Copiamos as webs originais para podermos alterá-las no splitting

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
    } else if (config.algorithm == "free") {  // O NOSSO NOVO ALGORITMO
        isFeasible = runFreeAllocation();
    }

    if (!isFeasible) {
        cerr << "Aviso: A alocacao nao foi possivel com os parametros dados." << endl;
        config.numRegisters = 0;
        for (const auto& web : finalWebs) {
            allocation[web.id] = -1;
        }
    }
}

bool Allocator::spillNode(const std::map<int, std::set<int>>& originalAdjList,
                         std::map<int, bool>& inGraph,
                         int& nodesRemaining,
                         int& spilledNodeId) {
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

bool Allocator::runBasicAllocation() {
    int N = config.numRegisters;
    const auto& originalAdjList = graph.getAdjList();
    map<int, bool> inGraph;
    int nodesRemaining = finalWebs.size();

    for (const auto& web : finalWebs) inGraph[web.id] = true;
    stack<int> S;

    while (nodesRemaining > 0) {
        bool found = false;
        for (const auto& pair : originalAdjList) {
            if (!inGraph[pair.first]) continue;

            int degree = 0;
            for (int neighbor : pair.second)
                if (inGraph[neighbor]) degree++;

            if (degree < N) {
                inGraph[pair.first] = false;
                S.push(pair.first);
                nodesRemaining--;
                found = true;
                break;
            }
        }

        // If no node has degree < N, choose one node to spill and keep simplifying.
        if (!found) {
            int spilledNodeId = -1;
            if (!spillNode(originalAdjList, inGraph, nodesRemaining, spilledNodeId)) return false;
        }
    }

    while (!S.empty()) {
        int n = S.top();
        S.pop();
        set<int> usedColors;
        for (int neighbor : originalAdjList.at(n)) {
            if (allocation[neighbor] != -1) usedColors.insert(allocation[neighbor]);
        }

        for (int c = 0; c < N; ++c) {
            if (usedColors.find(c) == usedColors.end()) {
                allocation[n] = c;
                break;
            }
        }
    }
    return true;
}

bool Allocator::runSpillingAllocation() {
    // Igual ao que já tinhas, adaptado para usar finalWebs
    int N = config.numRegisters;
    int maxSpills = config.k_param;
    int spillsUsed = 0;
    const auto& originalAdjList = graph.getAdjList();
    map<int, bool> inGraph;
    int nodesRemaining = finalWebs.size();

    for (const auto& web : finalWebs) inGraph[web.id] = true;
    stack<int> S;

    while (nodesRemaining > 0) {
        bool found = false;
        for (const auto& pair : originalAdjList) {
            if (!inGraph[pair.first]) continue;
            int degree = 0;
            for (int neighbor : pair.second)
                if (inGraph[neighbor]) degree++;

            if (degree < N) {
                inGraph[pair.first] = false;
                S.push(pair.first);
                nodesRemaining--;
                found = true;
                break;
            }
        }

        if (!found) {
            if (spillsUsed >= maxSpills) return false;

            int nodeToSpill = -1;
            if (!spillNode(originalAdjList, inGraph, nodesRemaining, nodeToSpill)) return false;
            spillsUsed++;
            cout << "-> Spilling aplicado na Web " << nodeToSpill << endl;
        }
    }

    while (!S.empty()) {
        int n = S.top();
        S.pop();
        set<int> usedColors;
        for (int neighbor : originalAdjList.at(n))
            if (allocation[neighbor] != -1) usedColors.insert(allocation[neighbor]);

        for (int c = 0; c < N; ++c) {
            if (usedColors.find(c) == usedColors.end()) {
                allocation[n] = c;
                break;
            }
        }
    }
    return true;
}

bool Allocator::runSplittingAllocation() {
    int maxSplits = config.k_param;
    int splitsUsed = 0;
    int N = config.numRegisters;

    // Ciclo principal: Tenta colorir, se falhar faz 1 split e RECOMEÇA tudo!
    while (true) {
        // 1. Reconstruir a lista de adjacências local para as webs atuais
        map<int, set<int>> currentAdjList;
        for (size_t i = 0; i < finalWebs.size(); ++i) {
            currentAdjList[finalWebs[i].id];  // Garante que a web existe no map
            for (size_t j = i + 1; j < finalWebs.size(); ++j) {
                if (finalWebs[i].interferesWith(finalWebs[j])) {
                    currentAdjList[finalWebs[i].id].insert(finalWebs[j].id);
                    currentAdjList[finalWebs[j].id].insert(finalWebs[i].id);
                }
            }
        }

        // 2. Tentar simplificar (como no Basic)
        map<int, bool> inGraph;
        int nodesRemaining = finalWebs.size();
        for (const auto& web : finalWebs) inGraph[web.id] = true;
        stack<int> S;
        bool stuck = false;

        while (nodesRemaining > 0) {
            bool found = false;
            for (const auto& pair : currentAdjList) {
                if (!inGraph[pair.first]) continue;
                int degree = 0;
                for (int neighbor : pair.second)
                    if (inGraph[neighbor]) degree++;

                if (degree < N) {
                    inGraph[pair.first] = false;
                    S.push(pair.first);
                    nodesRemaining--;
                    found = true;
                    break;
                }
            }

            if (!found) {
                stuck = true;
                break;
            }
        }

        // 3. Se não encravou, podemos colorir e terminar com sucesso!
        if (!stuck) {
            allocation.clear();
            for (const auto& web : finalWebs) allocation[web.id] = -1;  // reset

            while (!S.empty()) {
                int n = S.top();
                S.pop();
                set<int> usedColors;
                for (int neighbor : currentAdjList.at(n))
                    if (allocation[neighbor] != -1) usedColors.insert(allocation[neighbor]);

                for (int c = 0; c < N; ++c) {
                    if (usedColors.find(c) == usedColors.end()) {
                        allocation[n] = c;
                        break;
                    }
                }
            }
            return true;
        }

        // 4. Se encravou, TEMOS DE FAZER SPLIT!
        if (splitsUsed >= maxSplits) return false;  // Esgotámos os splits

        // Procurar a Web com maior grau que tenha pelo menos 2 linhas
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

        if (nodeToSplitId == -1) return false;  // Nenhuma Web divisível

        // Executar o corte
        Web original = finalWebs[splitIdx];
        finalWebs.erase(finalWebs.begin() + splitIdx);  // Remove a original

        Web w1, w2;
        // Descobrir o próximo ID livre
        int nextId = 0;
        for (const auto& w : finalWebs)
            if (w.id >= nextId) nextId = w.id + 1;

        w1.id = nextId++;
        w2.id = nextId++;
        w1.variableName = original.variableName;
        w2.variableName = original.variableName;

        // Dividir as linhas a meio
        int half = original.lines.size() / 2;
        int count = 0;
        for (int line : original.lines) {
            if (count < half)
                w1.lines.insert(line);
            else
                w2.lines.insert(line);
            count++;
        }

        finalWebs.push_back(w1);
        finalWebs.push_back(w2);
        splitsUsed++;

        cout << "-> Splitting aplicado na Web " << original.id << " (Cortada em Web " << w1.id
             << " e Web " << w2.id << ")" << endl;

        // O ciclo while(true) repete e tenta alocar novamente o novo grafo!
    }
}

bool Allocator::runFreeAllocation() {
    int N = config.numRegisters;
    int maxSplits = 3;  // O nosso orçamento interno de cortes
    int splitsUsed = 0;

    set<int> spilledWebs;  // Lista de webs que fomos forçados a atirar para a memória

    // O ciclo principal que garante a sobrevivência do compilador
    while (true) {
        // 1. Reconstruir a lista de adjacências (ignorando as webs que já foram para a memória)
        map<int, set<int>> currentAdjList;
        for (size_t i = 0; i < finalWebs.size(); ++i) {
            int id1 = finalWebs[i].id;
            if (spilledWebs.count(id1)) continue;  // Ignora as já sacrificadas

            currentAdjList[id1];
            for (size_t j = i + 1; j < finalWebs.size(); ++j) {
                int id2 = finalWebs[j].id;
                if (spilledWebs.count(id2)) continue;

                if (finalWebs[i].interferesWith(finalWebs[j])) {
                    currentAdjList[id1].insert(id2);
                    currentAdjList[id2].insert(id1);
                }
            }
        }

        // 2. Fase de Simplificação
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
            bool found = false;
            for (const auto& pair : currentAdjList) {
                if (!inGraph[pair.first]) continue;
                int degree = 0;
                for (int neighbor : pair.second)
                    if (inGraph[neighbor]) degree++;

                if (degree < N) {
                    inGraph[pair.first] = false;
                    S.push(pair.first);
                    nodesRemaining--;
                    found = true;
                    break;
                }
            }
            if (!found) {
                stuck = true;
                break;
            }  // Encravou!
        }

        // 3. Se não encravou, Coloração Final!
        if (!stuck) {
            allocation.clear();
            for (const auto& web : finalWebs) allocation[web.id] = -1;  // Reset inicial

            while (!S.empty()) {
                int n = S.top();
                S.pop();
                set<int> usedColors;
                for (int neighbor : currentAdjList.at(n)) {
                    if (allocation[neighbor] != -1) usedColors.insert(allocation[neighbor]);
                }

                for (int c = 0; c < N; ++c) {
                    if (usedColors.find(c) == usedColors.end()) {
                        allocation[n] = c;
                        break;
                    }
                }
            }
            return true;  // Sucesso Absoluto!
        }

        // 4. Se encravou -> PLANO A: Tentar o Splitting
        int nodeToSplitId = -1, maxDegreeSplit = -1, splitIdx = -1;
        if (splitsUsed < maxSplits) {
            for (size_t i = 0; i < finalWebs.size(); ++i) {
                int id = finalWebs[i].id;
                if (spilledWebs.count(id) || !inGraph[id]) continue;

                int degree = 0;
                for (int neighbor : currentAdjList[id])
                    if (inGraph[neighbor]) degree++;

                // Tem de ter a maior interferência E pelo menos 2 linhas de código
                if (degree > maxDegreeSplit && finalWebs[i].lines.size() >= 2) {
                    maxDegreeSplit = degree;
                    nodeToSplitId = id;
                    splitIdx = i;
                }
            }
        }

        if (nodeToSplitId != -1) {
            // Executa o Splitting
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
                if (count < half)
                    w1.lines.insert(line);
                else
                    w2.lines.insert(line);
                count++;
            }

            finalWebs.push_back(w1);
            finalWebs.push_back(w2);
            splitsUsed++;
            cout << "-> [Modo Free] Splitting aplicado na Web " << original.id << endl;
            continue;  // Recomeça o ciclo com as novas webs!
        }

        // 5. Se o Splitting falhar ou esgotar -> PLANO B: Spilling
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
            spilledWebs.insert(nodeToSpillId);  // Atira para a lista negra da memória
            cout << "-> [Modo Free] Spilling aplicado na Web " << nodeToSpillId << endl;
            continue;  // Recomeça o ciclo!
        }

        return false;  // Nunca deve chegar aqui, mas por segurança.
    }
}

void Allocator::generateOutputFile(const string& filename) const {
    ofstream file(filename);
    if (!file.is_open()) return;

    file << "# Total number of webs followed by the listing of the program points of each one\n";
    file << "# program points in each web are sorted in ascending order\n";
    file << "webs: " << finalWebs.size() << "\n";  // Usa as webs finais!

    for (const auto& web : finalWebs) {
        file << "web" << web.id << ": ";
        bool first = true;
        for (int line : web.lines) {
            if (!first) file << ",";
            file << line;
            first = false;
        }
        file << "\n";
    }

    file << "# Total number of registers used, followed by assignment to webs\n";
    file << "registers: " << (isFeasible ? config.numRegisters : 0) << "\n";

    for (const auto& web : finalWebs) {
        int reg = allocation.at(web.id);
        if (reg == -1)
            file << "M: web" << web.id << "\n";
        else
            file << "r" << reg << ": web" << web.id << "\n";
    }

    file.close();
    cout << "Resultados exportados para: " << filename << endl;
}