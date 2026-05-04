#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "Graph.h"
#include "Parser.h"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Classe responsavel pela alocacao de registos baseada na Coloracao de Grafos.
 * @details Tenta atribuir a cada Web (no) uma "cor" (registo fisico) garantindo que
 * nenhuma Web intersetante partilhe o mesmo registo. Implementa taticas de Spilling e Splitting em caso de falta de recursos.
 */
class Allocator {
private:
    Graph graph;                         /**< @brief Instancia do grafo original com as Webs e arestas. */
    ConfigRegisters config;              /**< @brief Configuracoes de limite de registos e parametros do algoritmo. */
    
    std::map<int, int> allocation;       /**< @brief Dicionario final que mapeia o ID da Web a cor do registo (-1 significa Memoria/M). */
    bool isFeasible;                     /**< @brief Flag booleana que indica se o algoritmo conseguiu concluir a alocacao. */
    
    std::vector<Web> finalWebs;          /**< @brief Cópia mutavel das Webs, essencial para permitir cortes (Splitting). */

    /**
     * @brief Tenta colorir o grafo atraves da reducao sucessiva de nos com grau inferior a N (Algoritmo de Chaitin).
     * @return true Se foi possivel colorir o grafo sem recorrer a Spilling/Splitting.
     * @note Complexidade Temporal: O(W^2 + E), onde W e o numero de Webs e E o numero de arestas no grafo.
     */
    bool runBasicAllocation();

    /**
     * @brief Algoritmo de coloracao que, em caso de bloqueio, sacrifica (Spill) a Web com maior grau de interferencia.
     * @return true Se a alocacao foi concluida dentro do limite estipulado de K spills.
     * @note Complexidade Temporal: O(W^3 + E) no pior caso, pois a cada bloqueio precisa de pesquisar e recalcular o grau de todos os nos restantes.
     */
    bool runSpillingAllocation();

    /**
     * @brief Algoritmo que divide Webs bloqueantes a meio (Splitting) para tentar acomoda-las em registos diferentes.
     * @return true Se a alocacao obteve sucesso com o novo grafo transformado.
     * @note Complexidade Temporal: O(S * (W^3 + E)), onde S e o limite de cortes (K_param), dado que recomeca o calculo a cada corte efetuado.
     */
    bool runSplittingAllocation();

    /**
     * @brief Heuristica propria que privilegia o Splitting para maximizar variaveis em registo, usando Spilling apenas como ultimo recurso.
     * @return true O algoritmo e garantido de ter sucesso devido a tatica infalivel de fallback.
     * @note Complexidade Temporal: O(S * (W^3 + E)), pois avalia continuamente o estado do grafo apos cortes e remocoes hibridas.
     */
    bool runFreeAllocation(); 

public:
    /**
     * @brief Construtor da classe Allocator. Prepara as estruturas baseando-se no grafo gerado.
     * @param g Referencia constante do Grafo de Interferencia.
     * @param c Definicoes lidas do ficheiro de registos.
     */
    Allocator(const Graph& g, const ConfigRegisters& c);

    /**
     * @brief Lanca a execucao do algoritmo especifico ditado no ficheiro de configuracao (basic, spilling, splitting ou free).
     */
    void run();

    /**
     * @brief Gera o ficheiro de texto final com o formato rigoroso exigido no enunciado do projeto.
     * @param filename Caminho e nome do ficheiro (ex: output.txt) onde o resultado sera escrito.
     * @note Complexidade Temporal: O(W + L), onde W e o numero de webs resultantes e L a soma total das suas linhas.
     */
    void generateOutputFile(const std::string& filename) const;
};

#endif