// Function declaration parse_cups_logs.
//   input - OS directory to system catalogue 
//   format  — "csv" or "json".
// Returns:
//   - string with data in coherent format,
//   - empty string if none files found.

#pragma once
#include <string>

std::string parse_cups_logs(const std::string& sysroot,
                            const std::string& format);

