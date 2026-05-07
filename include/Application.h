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
    // Constructor that receives the arguments from main
    Application(int argc, char* argv[]);
    
    // Main method to start the application
    void run();
};

#endif