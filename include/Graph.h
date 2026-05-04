#ifndef GRAPH_H
#define GRAPH_H

#include "DataTypes.h"
#include <vector>
#include <map>
#include <set>

/**
 * @brief Representa o Grafo de Interferencia para o problema de alocacao de registos.
 * @details Transforma os tempos de vida isolados das variaveis em Webs (nos do grafo) e
 * constroi a lista de adjacencias criando arestas entre Webs que partilham pontos de execucao.
 */
class Graph {
private:
    std::vector<Web> webs;                           /**< @brief Lista de todas as Webs criadas (os nos do grafo). */
    std::map<int, std::set<int>> adjList;            /**< @brief Lista de Adjacencias: Mapeia o ID da Web para um conjunto de IDs de Webs vizinhas (com as quais interfere). */

    /**
     * @brief Agrupa os LiveRanges pertencentes a mesma variavel, fundindo aqueles que se intersetam temporalmente.
     * @param liveRanges Vetor contendo a leitura bruta das linhas de codigo ativas de cada variavel.
     * @note Complexidade Temporal: O(V * R^2), onde V e o numero de variaveis distintas e R o numero medio de ranges por variavel (devido aos ciclos aninhados de fusao).
     */
    void buildWebs(const std::vector<LiveRange>& liveRanges);

    /**
     * @brief Avalia a interferencia entre todas as Webs e constroi a lista de adjacencias.
     * @details Se duas Webs coexistirem numa mesma linha de codigo, e criada uma aresta bidirecional entre elas.
     * @note Complexidade Temporal: O(W^2 * L), onde W e o numero total de Webs e L o numero medio de linhas de codigo contidas numa Web.
     */
    void buildInterferences();

public:
    /**
     * @brief Construtor da classe Graph. Inicializa automaticamente as Webs e as Interferencias.
     * @param liveRanges Vetor de blocos temporais lidos pelo Parser.
     */
    Graph(const std::vector<LiveRange>& liveRanges);

    /**
     * @brief Retorna a lista de Webs (nos) construidas no grafo.
     * @return const std::vector<Web>& Referencia constante para o vetor de Webs.
     */
    const std::vector<Web>& getWebs() const { return webs; }

    /**
     * @brief Retorna o mapeamento de interferencias entre as Webs.
     * @return const std::map<int, std::set<int>>& Referencia constante para a lista de adjacencias.
     */
    const std::map<int, std::set<int>>& getAdjList() const { return adjList; }
    
    /**
     * @brief Imprime o grafo formatado na consola para fins de depuracao e apresentacao (T1.1).
     */
    void printGraph() const;
};

#endif