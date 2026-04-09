#pragma once

#include <string>
#include <vector>
#include "table.h"

struct Query {
    std::vector<std::string> select_cols;
    std::string where_col;
    std::string op; // =, >, <
    std::string where_val;
};

// Parse a very small query language: "select a,b where c = value"
bool parse_query(const std::string& s, Query& out);

// Execute query against the table and return matching rows (as vector of selected fields)
std::vector<std::vector<std::string>> execute_query(const Table& t, const Query& q);
