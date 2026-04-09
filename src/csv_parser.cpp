#include "csv_parser.h"
#include <fstream>
#include <sstream>

namespace csv {

std::vector<std::vector<std::string>> parse_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<std::vector<std::string>> out;
    if (!in) return out;

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (c == '"') {
            if (inQuotes && i + 1 < content.size() && content[i+1] == '"') {
                // escaped quote
                field.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
            continue;
        }
        if (!inQuotes) {
            if (c == ',') {
                row.push_back(field);
                field.clear();
                continue;
            }
            if (c == '\r') {
                // skip, handle \r\n pairs by letting \n handle end
                continue;
            }
            if (c == '\n') {
                row.push_back(field);
                field.clear();
                out.push_back(row);
                row.clear();
                continue;
            }
        }
        // normal character (including newlines inside quotes)
        field.push_back(c);
    }
    // final field/row
    if (!inQuotes) {
        if (!field.empty() || !row.empty()) {
            row.push_back(field);
            out.push_back(row);
        }
    }
    return out;
}

} // namespace csv
