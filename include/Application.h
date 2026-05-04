#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <vector>

class Application {
private:
    bool isBatchMode;
    std::string rangesFile;
    std::string registersFile;
    std::string outputFile;

    void runBatchMode();
    void runInteractiveMode();
    void showMenu();

public:
    // Construtor que recebe os argumentos do main
    Application(int argc, char* argv[]);
    
    // Método principal para arrancar o programa
    void run();
};

#endif