#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class Table {
public:
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;

    // indices[col][value] -> list of row indices
    std::unordered_map<size_t, std::unordered_map<std::string, std::vector<size_t>>> indices;

    bool load_csv(const std::string& path);
    void build_index(size_t col);
    std::vector<size_t> lookup_index(size_t col, const std::string& value) const;
    int column_index(const std::string& name) const;
};
