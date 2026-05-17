/**
 * @file DataTypes.h
 * @brief Core data structures: LiveRange and Web for register allocation.
 */
#ifndef DATATYPES_H
#define DATATYPES_H

#include <set>
#include <string>
#include <vector>

/**
 * @brief Represents a live range segment of a variable read directly from the input file.
 */
struct LiveRange {
    std::string variableName; /**< @brief Original variable name (e.g., "sum", "i"). */
    std::set<int> lines; /**< @brief Ordered unique set of program lines where the variable is alive. */
    std::set<int> defLines;    /**< @brief Lines where the variable is defined ('+' marker). */
    std::set<int> useEndLines; /**< @brief Lines of the variable's last use ('-' marker). */
};

/**
 * @brief Represents a "Web", formed by the union of several overlapping LiveRanges of the same
 * variable.
 * @details Each instance of this structure acts as a node in the Interference Graph.
 */
struct Web {
    int id;                   /**< @brief Unique numeric Web identifier (e.g., 0 for web0). */
    std::string variableName; /**< @brief Variable name associated with this live-range block. */
    std::set<int> lines; /**< @brief All merged program lines for this Web, kept sorted and unique. */
    std::set<int> defLines;    /**< @brief Definition lines for this Web. */
    std::set<int> useEndLines; /**< @brief Last-use lines for this Web. */

    /**
     * @brief Checks whether the current Web interferes with another provided Web.
     * @details Two Webs interfere if their live ranges share at least one execution point (a common
     * source line).
     * @param other The other Web structure to test for line intersection.
     * @return true If the Webs share at least one source line (there is interference).
     * @return false If the Webs occur at completely disjoint times in the program.
     * @note Time complexity: O(L log L), where L is the maximum number of lines in a live range (due to std::set lookups).
     */
    bool interferesWith(const Web& other) const {
        for (int line : lines) {
            if (other.lines.find(line) != other.lines.end()) {
                const bool thisStartsHere = defLines.find(line) != defLines.end();
                const bool otherEndsHere = other.useEndLines.find(line) != other.useEndLines.end();
                const bool otherStartsHere = other.defLines.find(line) != other.defLines.end();
                const bool thisEndsHere = useEndLines.find(line) != useEndLines.end();

                // Exception: definition at X does not interfere with a last use at X.
                const bool boundaryException =
                    (thisStartsHere && otherEndsHere) || (otherStartsHere && thisEndsHere);

                if (!boundaryException) {
                    return true;
                }
            }
        }
        return false;
    }
};

#endif