#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "DataTypes.h"

/**
 * @brief Structure to store the definitions read from the registers/configuration file.
 */
struct ConfigRegisters {
    int numRegisters;      /**< @brief Maximum number of physical registers (colors) available for allocation. */
    std::string algorithm; /**< @brief Algorithm variant to apply ("basic", "spilling", "splitting", "free"). */
    int k_param;           /**< @brief Numeric parameter K limiting the number of spills or splits allowed. */
};

/**
 * @brief Class responsible for reading and parsing the input files.
 * @details Extracts the register configuration and the variable live ranges,
 * applying cleanup routines to ignore comment lines, whitespace and complex formatting.
 */
class Parser {
private:
    std::string rangesFilePath;
    std::string registersFilePath;

public:
    /**
     * @brief Parser class constructor.
     * @param rangesPath String path to the file containing live ranges (e.g. ranges.txt).
     * @param registersPath String path to the file containing register limits (e.g. registers.txt).
     */
    Parser(const std::string& rangesPath, const std::string& registersPath);
    
    /**
     * @brief Parses the registers/configuration file.
     * @return ConfigRegisters Structure filled with the number of registers, chosen algorithm and extracted K parameter.
     * @note Time complexity: O(L), where L is the total number of lines in the registers file.
     */
    ConfigRegisters parseRegistersFile();
    
    /**
     * @brief Parses the live ranges file.
     * @details Reads each variable, ignores isolated markers like "+" or "-", and stores the raw live-range blocks.
     * @return std::vector<LiveRange> Clean list containing all live-range blocks ready for grouping.
     * @note Time complexity: O(L * T log T), where L is the number of lines in the file and T is the number of temporal markers on a line (due to std::set lookups).
     */
    std::vector<LiveRange> parseRangesFile(); 
};

#endif