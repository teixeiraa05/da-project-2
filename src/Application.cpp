/**
 * @file Application.cpp
 * @brief Implementation of the application driver (batch and interactive modes).
 */
#include "../include/Application.h"

#include <iostream>

#include "../include/Allocator.h"
#include "../include/InterferenceGraph.h"
#include "../include/Parser.h"

using namespace std;

Application::Application(int argc, char* argv[]) {
    isBatchMode = false;
    interactiveAlgorithm = "basic";
    configVariant = 2;
    configFilePath = "data/basic/registers2.txt";

    if (argc >= 5 && string(argv[1]) == "-b") {
        isBatchMode = true;
        rangesFile = argv[2];
        registersFile = argv[3];
        outputFile = argv[4];
    } else if (argc > 1 && string(argv[1]) == "-b") {
        cerr << "Error: Insufficient arguments for batch mode." << endl;
        cerr << "Usage: ./myProg -b <ranges.txt> <registers.txt> <allocation.txt>" << endl;
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
    cout << "========================================" << endl;
    cout << "Batch Allocation Run" << endl;
    cout << "========================================" << endl;

    Parser parser(rangesFile, registersFile);
    ConfigRegisters config = parser.parseRegistersFile();
    vector<LiveRange> ranges = parser.parseRangesFile();

    if (ranges.empty()) {
        cerr << "Error: No ranges read. Exiting." << endl;
        return;
    }

    InterferenceGraph graph(ranges);

    // Export DOT file for the parsed interference graph to help visualization
    if (!outputFile.empty()) {
        std::string dotName = outputFile + ".gv";
        graph.emitDOTFile(outputFile);
        cout << "Graph preview saved to: " << dotName << endl;
    }

    Allocator allocator(graph, config);

    allocator.run();
    
    // Export colored DOT file after allocation with register assignments
    if (!outputFile.empty()) {
        std::string allocatedDotName = outputFile + "_allocated.gv";
        allocator.generateColoredDOT(outputFile + "_allocated");
        cout << "Allocated graph saved to: " << allocatedDotName << endl;
    }
    
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
            case 1: {
                const string rangeFiles[] = {
                    "ranges1.txt", "ranges2.txt", "ranges3.txt",
                    "ranges4.txt", "ranges5.txt", "ranges6.txt",
                    "ranges7.txt", "ranges8.txt",
                    "ranges_cycle_split.txt", "ranges_pres1_dsatur.txt",
                    "ranges_pres2_split.txt", "ranges_pres3_clique.txt",
                    "ranges_split_win.txt"
                };
                const int numFiles = sizeof(rangeFiles) / sizeof(rangeFiles[0]);
                cout << "\nAvailable ranges files:" << endl;
                for (int i = 0; i < numFiles; ++i)
                    cout << "  " << (i + 1) << ". " << rangeFiles[i] << endl;
                cout << "Choose [1-" << numFiles << "]: ";
                int r;
                cin >> r;
                if (r >= 1 && r <= numFiles) {
                    rangesFile = "data/ranges/" + rangeFiles[r - 1];
                    cout << "Ranges set to: " << rangesFile << endl;
                } else {
                    cerr << "Invalid choice." << endl;
                }
                break;
            }
            case 2:
                cout << "\nAvailable algorithms:" << endl;
                cout << "  1. basic" << endl;
                cout << "  2. spilling" << endl;
                cout << "  3. splitting" << endl;
                cout << "  4. free" << endl;
                cout << "Choose [1-4]: ";
                {
                    int algoChoice;
                    cin >> algoChoice;
                    if (algoChoice < 1 || algoChoice > 4) {
                        cerr << "Invalid choice. Keeping: " << interactiveAlgorithm << endl;
                        break;
                    }
                    string names[] = {"basic", "spilling", "splitting", "free"};
                    interactiveAlgorithm = names[algoChoice - 1];

                    if (interactiveAlgorithm == "spilling" || interactiveAlgorithm == "splitting") {
                        int N, X;
                        cout << "Registers (N) [1-4]: ";
                        cin >> N;
                        if (N < 1 || N > 4) N = 1;
                        cout << "File variant (X) [1-4]: ";
                        cin >> X;
                        if (X < 1 || X > 4) X = 1;
                        configVariant = N;
                        if (interactiveAlgorithm == "spilling")
                            configFilePath = "data/spilling/" + to_string(N) + "/spilling" + to_string(X) + ".txt";
                        else
                            configFilePath = "data/splitting/" + to_string(N) + "/splitting" + to_string(X) + ".txt";
                        cout << "Set to: " << interactiveAlgorithm << " (N=" << N << ", X=" << X << ")" << endl;
                    } else {
                        int minV, maxV;
                        if (interactiveAlgorithm == "basic")      { minV = 2; maxV = 3; }
                        else                                      { minV = 1; maxV = 3; }

                        cout << "Config variant (registers) [" << minV << "-" << maxV << "]: ";
                        cin >> configVariant;
                        if (configVariant < minV || configVariant > maxV) {
                            configVariant = minV;
                            cout << "Invalid. Using: " << configVariant << endl;
                        }

                        if (interactiveAlgorithm == "basic")
                            configFilePath = "data/basic/registers" + to_string(configVariant) + ".txt";
                        else
                            configFilePath = "data/free/free" + to_string(configVariant) + ".txt";

                        cout << "Set to: " << interactiveAlgorithm << " (variant " << configVariant << ")" << endl;
                    }
                }
                break;
            case 3:
                if (rangesFile.empty()) {
                    cerr << "Error: Please set input file first (Option 1)." << endl;
                } else {
                    cout << "Enter path for the output file to generate: ";
                    cin >> outputFile;
                    cout << "\n========================================" << endl;
                    cout << "Interactive Allocation Run" << endl;
                    cout << "Config: " << configFilePath << endl;
                    cout << "========================================" << endl;

                    Parser parser(rangesFile, configFilePath);
                    ConfigRegisters config = parser.parseRegistersFile();
                    vector<LiveRange> ranges = parser.parseRangesFile();

                    if (!ranges.empty()) {
                        InterferenceGraph graph(ranges);
                        graph.printGraph();
                        if (!outputFile.empty()) {
                            graph.emitDOTFile(outputFile);
                            cout << "Graph preview saved to: " << outputFile << ".gv" << endl;
                        }

                        Allocator allocator(graph, config);
                        allocator.run();
                        
                        if (!outputFile.empty()) {
                            allocator.generateColoredDOT(outputFile + "_allocated");
                            cout << "Allocated graph saved to: " << outputFile << "_allocated.gv" << endl;
                        }
                        
                        allocator.generateOutputFile(outputFile);
                    }
                }
                break;
            case 0:
                cout << "Exiting application..." << endl;
                break;
            default:
                cerr << "Invalid option. Please try again." << endl;
        }
    } while (choice != 0);
}

void Application::showMenu() {
    string ranges;
    if (rangesFile.empty()) {
        ranges = "(not set)";
    } else {
        size_t s = rangesFile.find_last_of('/');
        ranges = (s != string::npos) ? rangesFile.substr(s + 1) : rangesFile;
    }

    int N = 0, X = 0;
    if (interactiveAlgorithm == "spilling" || interactiveAlgorithm == "splitting") {
        size_t slash = configFilePath.find_last_of('/');
        if (slash != string::npos) {
            string dir = configFilePath.substr(0, slash);
            size_t prevSlash = dir.find_last_of('/');
            if (prevSlash != string::npos)
                N = stoi(dir.substr(prevSlash + 1));
        }
        size_t dot = configFilePath.find_last_of('.');
        size_t lastSlash = configFilePath.find_last_of('/');
        if (dot != string::npos && lastSlash != string::npos) {
            string base = configFilePath.substr(lastSlash + 1, dot - lastSlash - 1);
            string prefix = (interactiveAlgorithm == "spilling") ? "spilling" : "splitting";
            if (base.find(prefix) == 0)
                X = stoi(base.substr(prefix.size()));
        }
    } else {
        N = configVariant;
        X = 0;
    }

    cout << "\n=== Register Allocation Tool ===" << endl;
    cout << "   Ranges: " << ranges << endl;
    cout << "   Config: " << configFilePath << endl;
    cout << "   Algo:   " << interactiveAlgorithm;
    if (interactiveAlgorithm == "spilling" || interactiveAlgorithm == "splitting")
        cout << "  (N=" << N << ", X=" << X << ")";
    else if (interactiveAlgorithm == "basic" || interactiveAlgorithm == "free")
        cout << "  (variant " << N << ")";
    cout << endl;
    cout << "----------------------------------" << endl;
    cout << "1. Set input file" << endl;
    cout << "2. Choose algorithm + config" << endl;
    cout << "3. Run register allocation" << endl;
    cout << "0. Exit" << endl;
    cout << "Choose an option: ";
}