#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <vector>

/**
 * @brief Top-level application driver.
 *
 * The `Application` class parses command-line arguments and runs the program
 * in either batch or interactive mode. It encapsulates the input/output
 * filenames and the main control flow used by `main()`.
 */
class Application {
private:
    /** @brief True when the program should run in non-interactive (batch) mode. */
    bool isBatchMode;

    /** @brief Path to the live ranges input file (first CLI argument in batch mode). */
    std::string rangesFile;

    /** @brief Path to the registers/configuration file (second CLI argument in batch mode). */
    std::string registersFile;

    /** @brief Path to the output allocation file (third CLI argument in batch mode). */
    std::string outputFile;

    /** @brief Algorithm chosen interactively (default: "basic"). */
    std::string interactiveAlgorithm;

    /** @brief Variant number for the config file (1-4, meaning register count). */
    int configVariant;

    /** @brief Path to the config file selected for interactive mode. */
    std::string configFilePath;

    /**
     * @brief Execute the program in batch mode using `rangesFile`, `registersFile` and `outputFile`.
     *
     * This routine implements the non-interactive workflow used by the assignment's
     * batch execution mode (`-b`), reading inputs, invoking the parser/allocator and
     * producing the resulting file.
     */
    void runBatchMode();

    /**
     * @brief Execute the interactive (menu-driven) application flow.
     *
     * Presents the user with a menu, allows selecting input files and allocation
     * parameters, and runs the allocation pipeline on demand.
     */
    void runInteractiveMode();

    /** @brief Show the textual interactive menu to the user. */
    void showMenu();

public:
    /**
     * @brief Construct the Application from program arguments.
     * @param argc Argument count passed from `main()`.
     * @param argv Argument vector passed from `main()`.
     *
     * The constructor interprets command-line flags and initializes internal
     * fields such as `isBatchMode` and the input/output file paths.
     */
    Application(int argc, char* argv[]);
    
    /**
     * @brief Run the application.
     *
     * Dispatches to `runBatchMode()` or `runInteractiveMode()` depending on the
     * parsed command-line options.
     */
    void run();
};

#endif