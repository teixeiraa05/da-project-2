#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <set>

/**
 * @brief Representa um segmento de tempo de vida (Live Range) de uma variavel lido diretamente do ficheiro.
 */
struct LiveRange {
    std::string variableName; /**< @brief Nome da variavel original (ex: "sum", "i"). */
    std::set<int> lines;      /**< @brief Conjunto ordenado e unico de linhas de execucao onde a variavel esta ativa. */
};

/**
 * @brief Representa uma "Web", formada pela uniao de varios LiveRanges sobrepostos da mesma variavel.
 * @details Cada instancia desta estrutura atua como um nó no Grafo de Interferencia.
 */
struct Web {
    int id;                   /**< @brief Identificador unico e numerico da Web (ex: 0 para web0). */
    std::string variableName; /**< @brief Nome da variavel a que este bloco de tempo de vida pertence. */
    std::set<int> lines;      /**< @brief Todas as linhas de execucao fundidas num unico conjunto ordenado. */
    
    /**
     * @brief Verifica se a Web atual interfere com outra Web fornecida.
     * @details Duas Webs interferem se os seus tempos de vida partilharem pelo menos um ponto de execucao (uma linha de codigo comum).
     * * @param other A outra estrutura Web com a qual queremos testar a intersecao de linhas.
     * @return true Se as Webs partilharem pelo menos uma linha de codigo (existe interferencia).
     * @return false Se as Webs ocorrerem em momentos totalmente disjuntos do programa.
     * * @note Complexidade Temporal: O(N log M), onde N e o numero de linhas na Web atual e M e o numero de linhas na Web 'other' (devido a pesquisa no std::set).
     */
    bool interferesWith(const Web& other) const {
        for (int line : lines) {
            if (other.lines.find(line) != other.lines.end()) {
                return true; // Partilham pelo menos uma linha!
            }
        }
        return false;
    }
};

#endif