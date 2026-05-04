#include "../include/Application.h"
#include "../include/Parser.h"
#include "../include/Graph.h"
#include "../include/Allocator.h"
#include <iostream>

using namespace std;

Application::Application(int argc, char* argv[]) {
    isBatchMode = false;
    
    // Verifica o formato exigido: myProg -b ranges.txt registers.txt allocation.txt
    if (argc >= 5 && string(argv[1]) == "-b") {
        isBatchMode = true;
        rangesFile = argv[2];
        registersFile = argv[3];
        outputFile = argv[4];
    } 
    else if (argc > 1 && string(argv[1]) == "-b") {
        cerr << "Erro: Argumentos insuficientes para o modo batch." << endl;
        cerr << "Uso correto: ./myProg -b <ranges.txt> <registers.txt> <allocation.txt>" << endl;
        exit(1);
    }
}

void Application::run() {
    if (isBatchMode) {
        runBatchMode();
    } else {
        runInteractiveMode();
    }
}

void Application::runBatchMode() {
    cout << "A iniciar execucao em modo Batch..." << endl;
    
    Parser parser(rangesFile, registersFile);
    ConfigRegisters config = parser.parseRegistersFile();
    vector<LiveRange> ranges = parser.parseRangesFile();
    
    if (ranges.empty()) {
        cerr << "Erro: Nenhuma range lida. A terminar." << endl;
        return;
    }

    Graph graph(ranges);
    Allocator allocator(graph, config);
    
    allocator.run();
    allocator.generateOutputFile(outputFile);
}

void Application::runInteractiveMode() {
    int choice;
    do {
        showMenu();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            choice = -1;
        }

        switch (choice) {
            case 1:
                cout << "Indique o caminho do ficheiro ranges: ";
                cin >> rangesFile;
                cout << "Indique o caminho do ficheiro registers: ";
                cin >> registersFile;
                cout << "Indique o caminho do ficheiro de output a gerar: ";
                cin >> outputFile;
                break;
            case 2:
                if (rangesFile.empty() || registersFile.empty() || outputFile.empty()) {
                    cerr << "Erro: Por favor defina os ficheiros primeiro (Opcao 1)." << endl;
                } else {
                    cout << "\n--- A executar pipeline de alocacao ---" << endl;
                    Parser parser(rangesFile, registersFile);
                    ConfigRegisters config = parser.parseRegistersFile();
                    vector<LiveRange> ranges = parser.parseRangesFile();
                    
                    if (!ranges.empty()) {
                        Graph graph(ranges);
                        graph.printGraph(); // Mostra o grafo no terminal para vermos a magia a acontecer
                        
                        Allocator allocator(graph, config);
                        allocator.run();
                        allocator.generateOutputFile(outputFile);
                    }
                }
                break;
            case 0:
                cout << "A sair da aplicacao..." << endl;
                break;
            default:
                cerr << "Opcao invalida. Tente novamente." << endl;
        }
    } while (choice != 0);
}

void Application::showMenu() {
    cout << "\n=== Ferramenta de Alocacao de Registos ===" << endl;
    cout << "1. Definir ficheiros de input e output" << endl;
    cout << "2. Executar alocacao de registos" << endl;
    cout << "0. Sair" << endl;
    cout << "Escolha uma opcao: ";
}