#include "../include/Parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

struct ParsedRangeToken {
    int line = -1;
    bool isDefinition = false;
    bool isLastUse = false;
};

bool parseRangeToken(const string& rawToken, ParsedRangeToken& parsed, string& errorMsg) {
    string token = rawToken;
    token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());

    if (token.empty()) {
        errorMsg = "empty token";
        return false;
    }

    size_t plusCount = count(token.begin(), token.end(), '+');
    size_t minusCount = count(token.begin(), token.end(), '-');

    if (plusCount > 1 || minusCount > 1 || (plusCount == 1 && minusCount == 1)) {
        errorMsg = "token contains invalid marker combination";
        return false;
    }

    size_t markerPos = token.find_first_of("+-");
    if (markerPos != string::npos && markerPos != token.size() - 1) {
        errorMsg = "marker must appear only at the end of token";
        return false;
    }

    string linePart = (markerPos == string::npos) ? token : token.substr(0, markerPos);
    if (linePart.empty()) {
        errorMsg = "missing line number";
        return false;
    }

    if (!all_of(linePart.begin(), linePart.end(), ::isdigit)) {
        errorMsg = "line number must contain only digits";
        return false;
    }

    parsed.line = stoi(linePart);
    parsed.isDefinition = (markerPos != string::npos && token[markerPos] == '+');
    parsed.isLastUse = (markerPos != string::npos && token[markerPos] == '-');
    return true;
}

// Constructor
Parser::Parser(const std::string& rangesPath, const std::string& registersPath) {
    rangesFilePath = rangesPath;
    registersFilePath = registersPath;
}

// --- REGISTERS PARSER ---
ConfigRegisters Parser::parseRegistersFile() {
    ConfigRegisters config = {0, "basic", 0};  // Default values
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
        } else if (key == "algorithm") {
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
    int fileLineNumber = 0;

    while (getline(file, line)) {
        ++fileLineNumber;
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
            ParsedRangeToken parsedToken;
            string parseError;
            if (!parseRangeToken(token, parsedToken, parseError)) {
                cerr << "Error: Invalid ranges token '" << token << "' at line " << fileLineNumber
                     << " (" << parseError << ")" << endl;
                return {};
            }

            lr.lines.insert(parsedToken.line);
            if (parsedToken.isDefinition) {
                lr.defLines.insert(parsedToken.line);
            }
            if (parsedToken.isLastUse) {
                lr.useEndLines.insert(parsedToken.line);
            }
        }

        if (!lr.lines.empty()) {
            liveRanges.push_back(lr);
        }
    }

    file.close();
    return liveRanges;
}