// ─────────────────────────────────────────────────────────────────────────────
// csv_parser.cpp
// ─────────────────────────────────────────────────────────────────────────────
#include "csv_parser.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// ── Public ────────────────────────────────────────────────────────────────────

std::optional<Table> CSVParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "  [error] Cannot open '" << filename << "'\n";
        return std::nullopt;
    }

    Table table;
    table.name = stemName(filename);

    std::string line;
    bool first = true;

    while (std::getline(file, line)) {
        // Strip Windows carriage return
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        auto fields = parseLine(line);

        if (first) {
            // ── Header row: build column map ──────────────────────────────
            table.columns = fields;
            for (size_t i = 0; i < fields.size(); ++i) {
                std::string lower = fields[i];
                std::transform(lower.begin(), lower.end(), lower.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                table.col_map[lower]     = i;
                table.col_map[fields[i]] = i;   // original case too
            }
            first = false;
        } else {
            // Pad short rows so every row has the same column count
            while (fields.size() < table.columns.size()) fields.push_back("");
            table.rows.push_back(std::move(fields));
        }
    }

    std::cout << "  Loaded table '" << table.name << "'  ("
              << table.columns.size() << " columns, "
              << table.rows.size()    << " rows)\n";
    return table;
}

// ── Private ───────────────────────────────────────────────────────────────────

/// RFC-4180-compliant CSV field parser.
/// Handles:  "quoted, fields"  "escaped ""double"" quotes"  unquoted fields
std::vector<std::string> CSVParser::parseLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (in_quotes) {
            if (c == '"') {
                // Peek: escaped quote ("") inside a quoted field
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    in_quotes = false;  // closing quote
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(trim(field));
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.push_back(trim(field));
    return fields;
}

std::string CSVParser::trim(const std::string& s) {
    const char* ws = " \t";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

/// Extract bare filename without path or extension.
std::string CSVParser::stemName(const std::string& filepath) {
    size_t slash = filepath.find_last_of("/\\");
    size_t start = (slash == std::string::npos) ? 0 : slash + 1;
    size_t dot   = filepath.find_last_of('.');
    size_t len   = (dot == std::string::npos || dot < start)
                       ? std::string::npos
                       : dot - start;
    return filepath.substr(start, len);
}
