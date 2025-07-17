// Entry point: accepts two arguments:
//  1. sysroot - path to the system root directory (e.g., "/" or "/tmp/test-cups")
//  2. format  - string "csv" or "json"
// Calls parse_cups_logs and outputs the result or an error message.
// Optionally accepts -p/--path to override the root directory for searching.

#include <iostream>
#include <string>
#include <filesystem>
#include "parser.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string sysroot = "/";
    std::string fmt;

    // Parse arguments: -p/--path and output format
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-p" || arg == "--path") && i + 1 < argc) {
            sysroot = argv[++i];
        } else if (arg == "csv" || arg == "json") {
            fmt = arg;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n"
                      << "Usage:\n  "
                      << argv[0] << " [csv|json] [-p|--path <system_root>]\n";
            return 1;
        }
    }

    // Check that a format was selected
    if (fmt.empty()) {
        std::cerr << "Error: output format not specified, choose 'csv' or 'json'\n";
        return 2;
    }

    // Construct the full path to /var/log/cups relative to sysroot
    fs::path cups_root = fs::path(sysroot) / "var" / "log" / "cups";

    // Call the parsing function
    std::string out = parse_cups_logs(sysroot, fmt);

    // If result is empty, no logs were found
    if (out.empty()) {
        std::cerr << "CUPS log files not found in directory: "
                  << cups_root.string() << "\n";
        return 3;
    }

    // Output the result
    std::cout << out;
    return 0;
}
