#include "../include/Parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// Constructor
Parser::Parser(const std::string& rangesPath, const std::string& registersPath) {
    rangesFilePath = rangesPath;
    registersFilePath = registersPath;
}

// --- HELPER FUNCTION ---
// Cleans a token like "12+" or "14-" returning only the integer 12 or 14
int cleanLineNumber(const string& token) {
    string cleanStr = "";
    for (char c : token) {
        if (isdigit(c)) {
            cleanStr += c;
        }
    }
    if (cleanStr.empty()) return -1;
    return stoi(cleanStr);
}

// --- REGISTERS PARSER ---
ConfigRegisters Parser::parseRegistersFile() {
    ConfigRegisters config = {0, "basic", 0}; // Valores por defeito
    ifstream file(registersFilePath);
    
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << registersFilePath << endl;
        return config;
    }

    string line;
    while (getline(file, line)) {
        // Ignorar linhas vazias e comentários
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;

        string key = line.substr(0, colonPos);
        string valuePart = line.substr(colonPos + 1);
        
        // Remover espaços da chave ("registers " -> "registers")
        key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());

        if (key == "registers") {
            config.numRegisters = stoi(valuePart);
        } 
        else if (key == "algorithm") {
            // Lidar com algo como " spilling, 2"
            stringstream ss(valuePart);
            string algoName;
            
            // Read until the comma
            getline(ss, algoName, ',');
            algoName.erase(remove_if(algoName.begin(), algoName.end(), ::isspace), algoName.end());
            config.algorithm = algoName;

            // Tentar ler o parâmetro K (se existir)
            string kParamStr;
            if (getline(ss, kParamStr, ',')) {
                config.k_param = stoi(kParamStr);
            }
        }
    }
    
    file.close();
    return config;
}

// --- RANGES PARSER ---
vector<LiveRange> Parser::parseRangesFile() {
    vector<LiveRange> liveRanges;
    ifstream file(rangesFilePath);
    
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << rangesFilePath << endl;
        return liveRanges;
    }

    string line;
    string currentVarName = "";

    while (getline(file, line)) {
        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;

        string varPart = line.substr(0, colonPos);
        string rangesPart = line.substr(colonPos + 1);

        // Trim whitespace
        varPart.erase(remove_if(varPart.begin(), varPart.end(), ::isspace), varPart.end());

        // Update current variable name (if empty, keep previous)
        if (!varPart.empty()) {
            currentVarName = varPart; 
        }

        LiveRange lr;
        lr.variableName = currentVarName;

        // Split numbers by commas
        stringstream ss(rangesPart);
        string token;
        while (getline(ss, token, ',')) {
            int lineNum = cleanLineNumber(token);
            if (lineNum != -1) {
                lr.lines.insert(lineNum);
            }
        }

        if (!lr.lines.empty()) {
            liveRanges.push_back(lr);
        }
    }
    
    file.close();
    return liveRanges;
}