#include "../include/Application.h"
#include "../include/Parser.h"
#include "../include/InterferenceGraph.h"
#include "../include/Allocator.h"
#include <iostream>

using namespace std;

Application::Application(int argc, char* argv[]) {
    isBatchMode = false;
    
    // Verify required format: myProg -b ranges.txt registers.txt allocation.txt
    if (argc >= 5 && string(argv[1]) == "-b") {
        isBatchMode = true;
        rangesFile = argv[2];
        registersFile = argv[3];
        outputFile = argv[4];
    } 
    else if (argc > 1 && string(argv[1]) == "-b") {
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
    cout << "Starting batch execution..." << endl;
    
    Parser parser(rangesFile, registersFile);
    ConfigRegisters config = parser.parseRegistersFile();
    vector<LiveRange> ranges = parser.parseRangesFile();
    
    if (ranges.empty()) {
        cerr << "Error: No ranges read. Exiting." << endl;
        return;
    }

    InterferenceGraph graph(ranges);
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
                cout << "Enter path to ranges file: ";
                cin >> rangesFile;
                cout << "Enter path to registers file: ";
                cin >> registersFile;
                cout << "Enter path for the output file to generate: ";
                cin >> outputFile;
                break;
            case 2:
                if (rangesFile.empty() || registersFile.empty() || outputFile.empty()) {
                    cerr << "Error: Please set input and output files first (Option 1)." << endl;
                } else {
                    cout << "\n--- Running allocation pipeline ---" << endl;
                    Parser parser(rangesFile, registersFile);
                    ConfigRegisters config = parser.parseRegistersFile();
                    vector<LiveRange> ranges = parser.parseRangesFile();
                    
                    if (!ranges.empty()) {
                        InterferenceGraph graph(ranges);
                        graph.printGraph(); // Print the graph to the terminal so we can see the allocation process

                        Allocator allocator(graph, config);
                        allocator.run();
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
    cout << "\n=== Register Allocation Tool ===" << endl;
    cout << "1. Set input and output files" << endl;
    cout << "2. Run register allocation" << endl;
    cout << "0. Exit" << endl;
    cout << "Choose an option: ";
}