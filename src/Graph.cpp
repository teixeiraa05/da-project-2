#include "../include/Graph.h"
#include <iostream>

using namespace std;

Graph::Graph(const vector<LiveRange>& liveRanges) {
    buildWebs(liveRanges);
    buildInterferences();
}

void Graph::buildWebs(const vector<LiveRange>& liveRanges) {
    int webCounter = 0;
    
    // Passo 1: Agrupar LiveRanges pelo nome da variável
    map<string, vector<set<int>>> varRanges;
    for (const auto& lr : liveRanges) {
        varRanges[lr.variableName].push_back(lr.lines);
    }

    // Passo 2: Para cada variável, fundir os ranges que se intersetam
    for (auto& pair : varRanges) {
        string varName = pair.first;
        vector<set<int>>& ranges = pair.second;

        bool merged;
        do {
            merged = false;
            for (size_t i = 0; i < ranges.size(); ++i) {
                for (size_t j = i + 1; j < ranges.size(); ++j) {
                    
                    // Verificar se existe interseção (partilham alguma linha?)
                    bool intersect = false;
                    for (int line : ranges[i]) {
                        if (ranges[j].find(line) != ranges[j].end()) {
                            intersect = true;
                            break;
                        }
                    }
                    
                    if (intersect) {
                        // Fundir o range j no range i
                        ranges[i].insert(ranges[j].begin(), ranges[j].end());
                        // Apagar o range j (pois já foi fundido)
                        ranges.erase(ranges.begin() + j);
                        merged = true;
                        break; // Quebra o ciclo interno e recomeça (os índices mudaram)
                    }
                }
                if (merged) break; // Recomeça o ciclo 'do-while'
            }
        } while (merged);

        // Passo 3: Transformar os ranges finais (já fundidos) em Webs reais
        for (const auto& r : ranges) {
            Web newWeb;
            newWeb.id = webCounter++;
            newWeb.variableName = varName;
            newWeb.lines = r;
            webs.push_back(newWeb);
        }
    }
}

void Graph::buildInterferences() {
    for (size_t i = 0; i < webs.size(); ++i) {
        // Garantir que a Web existe na lista de adjacências, mesmo que não tenha arestas (grau 0)
        adjList[webs[i].id]; 
        
        for (size_t j = i + 1; j < webs.size(); ++j) {
            // Verificar se interferem usando o método prático que criámos no DataTypes.h
            if (webs[i].interferesWith(webs[j])) {
                // Adicionar a aresta em ambas as direções (grafo não direcionado)
                adjList[webs[i].id].insert(webs[j].id);
                adjList[webs[j].id].insert(webs[i].id);
            }
        }
    }
}

void Graph::printGraph() const {
    cout << "\n=== Grafo de Interferencia (Webs: " << webs.size() << ") ===" << endl;
    for (const auto& web : webs) {
        cout << "Web " << web.id << " (" << web.variableName << "): [ ";
        for (int line : web.lines) cout << line << " ";
        cout << "] -> Interfere com: { ";
        for (int neighbor : adjList.at(web.id)) {
            cout << "Web" << neighbor << " ";
        }
        cout << "}" << endl;
    }
    cout << "======================================\n" << endl;
}