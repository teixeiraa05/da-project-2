#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "DataTypes.h"

/**
 * @brief Estrutura para armazenar as definicoes lidas do ficheiro de registos.
 */
struct ConfigRegisters {
    int numRegisters;      /**< @brief Numero maximo de registos fisicos (cores) disponiveis para alocacao. */
    std::string algorithm; /**< @brief Variante do algoritmo a aplicar ("basic", "spilling", "splitting", "free"). */
    int k_param;           /**< @brief Parametro numerico K que limita a quantidade de spills ou splits permitidos. */
};

/**
 * @brief Classe responsavel pela leitura e analise (parsing) dos ficheiros de input.
 * @details Extrai a configuracao dos registos e os intervalos de vida (Live Ranges) das variaveis,
 * aplicando rotinas de limpeza para ignorar linhas de comentarios, espacos e formatacoes complexas.
 */
class Parser {
private:
    std::string rangesFilePath;
    std::string registersFilePath;

public:
    /**
     * @brief Construtor da classe Parser.
     * * @param rangesPath Caminho em formato string para o ficheiro contendo os tempos de vida (ex: ranges.txt).
     * @param registersPath Caminho em formato string para o ficheiro contendo as limitacoes de registos (ex: registers.txt).
     */
    Parser(const std::string& rangesPath, const std::string& registersPath);
    
    /**
     * @brief Efetua o parse do ficheiro de configuracao de registos.
     * * @return ConfigRegisters Estrutura preenchida com o numero de registos, algoritmo escolhido e parametro K extraido.
     * * @note Complexidade Temporal: O(L), onde L e o numero total de linhas do ficheiro de registos.
     */
    ConfigRegisters parseRegistersFile();
    
    /**
     * @brief Efetua o parse do ficheiro de intervalos de vida.
     * @details Le cada variavel, ignora identificadores isolados como "+" ou "-", e guarda as linhas de atuacao 
     * em blocos base nao processados.
     * * @return std::vector<LiveRange> Lista limpa contendo todos os blocos de tempos de vida prontos a agrupar.
     * * @note Complexidade Temporal: O(L * T log T), onde L e o numero de linhas no ficheiro, T e o numero de marcadores temporais numa linha, com a pesquisa logaritmica do std::set.
     */
    std::vector<LiveRange> parseRangesFile(); 
};

#endif