#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// csv_parser.h  –  Robust CSV parser (handles quoted fields, escaped quotes)
// ─────────────────────────────────────────────────────────────────────────────
#include "table.h"
#include <string>
#include <optional>

class CSVParser {
public:
    /// Load filename into a Table.  Returns nullopt on any I/O error.
    static std::optional<Table> parse(const std::string& filename);

private:
    static std::vector<std::string> parseLine(const std::string& line);
    static std::string              trim(const std::string& s);
    static std::string              stemName(const std::string& filepath);
};
