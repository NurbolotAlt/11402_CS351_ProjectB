#pragma once

#include <string>
#include <vector>

namespace csv {
    // Parse an entire CSV file into rows of fields.
    // Handles quoted fields, embedded commas, embedded newlines, and escaped quotes ("").
    std::vector<std::vector<std::string>> parse_file(const std::string& path);
}
