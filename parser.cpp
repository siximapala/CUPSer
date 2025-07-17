// Implementation of the parse_cups_logs function with recursive traversal.
// Searches for all log files in the $root$/var/log/cups directory and subdirectories,
// reads them, and aggregates into a single CSV or JSON.
// Processes each log line individually and appends the date.

#include "parser.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <tuple>

namespace fs = std::filesystem;

// Reads a file line by line
// (reads line by line instead of loading the entire file)
static std::vector<std::string> read_lines(const fs::path& p) {
    std::ifstream in(p);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Escapes a string for CSV
static std::string escape_csv(const std::string& s) {
    if (s.find('"') == std::string::npos && 
        s.find(',') == std::string::npos) {
        return s;
    }
    std::string result;
    result.reserve(s.length() + 2);
    result += '"';
    for (char c : s) {
        if (c == '"') {
            result += '"';
            result += '"';
        } else {
            result += c;
        }
    }
    result += '"';
    return result;
}

// Escapes a string for JSON
static std::string escape_json(const std::string& s) {
    std::ostringstream out;
    for (char c : s) {
        switch (c) {
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b";  break;
            case '\f': out << "\\f";  break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;
            default:   out << c;      break;
        }
    }
    return out.str();
}

// Extracts the date from a log line depending on the file type
static std::string extract_date(const std::string& filename, const std::string& line) {
    fs::path p(filename);
    std::string fn = p.filename().string();
    
    // For access_log: [17/May/2025:17:41:16]
    if (fn == "access_log") {
        size_t start = line.find('[');
        size_t end = line.find(']', start);
        if (start != std::string::npos && end != std::string::npos) {
            return line.substr(start + 1, end - start - 1);
        }
    } 
    // For error_log: [17/May/2025:17:41:16]
    else if (fn == "error_log") {
        size_t start = line.find('[');
        size_t end = line.find(']', start);
        if (start != std::string::npos && end != std::string::npos) {
            return line.substr(start + 1, end - start - 1);
        }
    } 
    // For page_log: 2025-05-01 10:59:10 ...
    else if (fn == "page_log") {
        if (line.length() >= 19) {
            // Check date format: YYYY-MM-DD HH:MM:SS
            if (line[4] == '-' && line[7] == '-' && line[10] == ' ' && 
                line[13] == ':' && line[16] == ':') {
                return line.substr(0, 19);
            }
        }
    }
    return ""; // If date could not be extracted
}

// Converts the data array into a CSV string.
// (added Date field)
static std::string to_csv(const std::vector<std::tuple<std::string, std::string, std::string>>& data) {
    std::ostringstream out;
    out << "File,Date,Content\n";
    for (const auto& rec : data) {
        const auto& file = escape_csv(std::get<0>(rec));
        const auto& date = escape_csv(std::get<1>(rec));
        const auto& content = escape_csv(std::get<2>(rec));
        out << file << "," << date << "," << content << "\n";
    }
    return out.str();
}

// Converts the data array into a JSON string.
// (also added the date field)
static std::string to_json(const std::vector<std::tuple<std::string, std::string, std::string>>& data) {
    std::ostringstream out;
    out << "[\n";
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& rec = data[i];
        const auto& file = escape_json(std::get<0>(rec));
        const auto& date = escape_json(std::get<1>(rec));
        const auto& content = escape_json(std::get<2>(rec));
        out << "  {\n";
        out << "    \"file\": \"" << file << "\",\n";
        out << "    \"date\": \"" << date << "\",\n";
        out << "    \"content\": \"" << content << "\"\n";
        out << "  }";
        if (i + 1 < data.size()) out << ",";
        out << "\n";
    }
    out << "]";
    return out.str();
}

// Main parse_cups_logs function:
//   1) Constructs cups_root = sysroot/var/log/cups
//   2) Checks that cups_root exists and is a directory
//   3) Recursively traverses all files under cups_root
//   4) Selects files named access_log, error_log, page_log
//   5) Reads each file line by line and collects each line as a separate record
//   6) Extracts the date for each line
//   7) Returns data in CSV or JSON
std::string parse_cups_logs(const std::string& sysroot, const std::string& format) {
    // Root directory for logs
    fs::path cups_root = fs::path(sysroot) / "var" / "log" / "cups";
    if (!fs::exists(cups_root) || !fs::is_directory(cups_root)) {
        return "";
    }

    // Possible log file names
    const std::vector<std::string> log_names = {
        "access_log",
        "error_log",
        "page_log"
    };
    std::vector<std::tuple<std::string, std::string, std::string>> records;
    
    // Recursive traversal checking file names
    for (auto& entry : fs::recursive_directory_iterator(cups_root)) {
        if (!entry.is_regular_file()) continue;
        std::string fn = entry.path().filename().string();
        if (std::find(log_names.begin(), log_names.end(), fn) != log_names.end()) {
            auto lines = read_lines(entry.path());
            for (const auto& line : lines) {
                // Added: extract date for each line
                std::string date = extract_date(entry.path().string(), line);
                records.emplace_back(entry.path().string(), date, line);
            }
        }
    }

    if (records.empty()) {
        return "";
    }

    return (format == "json") ? to_json(records) : to_csv(records);
}
