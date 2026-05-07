#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <set>

/**
 * @brief Represents a live range segment of a variable read directly from the input file.
 */
struct LiveRange {
    std::string variableName; /**< @brief Nome da variavel original (ex: "sum", "i"). */
    std::set<int> lines;      /**< @brief Conjunto ordenado e unico de linhas de execucao onde a variavel esta ativa. */
};

/**
 * @brief Represents a "Web", formed by the union of several overlapping LiveRanges of the same variable.
 * @details Each instance of this structure acts as a node in the Interference Graph.
 */
struct Web {
    int id;                   /**< @brief Identificador unico e numerico da Web (ex: 0 para web0). */
    std::string variableName; /**< @brief Nome da variavel a que este bloco de tempo de vida pertence. */
    std::set<int> lines;      /**< @brief Todas as linhas de execucao fundidas num unico conjunto ordenado. */
    
    /**
     * @brief Checks whether the current Web interferes with another provided Web.
     * @details Two Webs interfere if their live ranges share at least one execution point (a common source line).
     * @param other The other Web structure to test for line intersection.
     * @return true If the Webs share at least one source line (there is interference).
     * @return false If the Webs occur at completely disjoint times in the program.
     * @note Time complexity: O(N log M), where N is the number of lines in the current Web and M is the number in 'other' (due to std::set lookups).
     */
    bool interferesWith(const Web& other) const {
        for (int line : lines) {
            if (other.lines.find(line) != other.lines.end()) {
                return true; // They share at least one line
            }
        }
        return false;
    }
};

#endif