#include "table.h"
#include "csv_parser.h"
#include <algorithm>
#include <iostream>

bool Table::load_csv(const std::string& path) {
    auto data = csv::parse_file(path);
    if (data.empty()) return false;
    columns = data[0];
    rows.clear();
    for (size_t i = 1; i < data.size(); ++i) {
        // normalize row length to columns
        auto r = data[i];
        r.resize(columns.size());
        rows.push_back(std::move(r));
    }
    indices.clear();
    return true;
}

void Table::build_index(size_t col) {
    if (col >= columns.size()) return;
    auto &mp = indices[col];
    mp.clear();
    for (size_t i = 0; i < rows.size(); ++i) {
        const std::string &val = rows[i][col];
        mp[val].push_back(i);
    }
}

std::vector<size_t> Table::lookup_index(size_t col, const std::string& value) const {
    auto itc = indices.find(col);
    if (itc == indices.end()) return {};
    auto it = itc->second.find(value);
    if (it == itc->second.end()) return {};
    return it->second;
}

int Table::column_index(const std::string& name) const {
    for (size_t i = 0; i < columns.size(); ++i) if (columns[i] == name) return (int)i;
    return -1;
}
