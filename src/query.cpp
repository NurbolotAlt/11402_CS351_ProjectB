#include "query.h"
#include <sstream>
#include <cctype>
#include <iostream>

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && std::isspace((unsigned char)s[a])) ++a;
    size_t b = s.size(); while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

bool parse_query(const std::string& s, Query& out) {
    std::string lower = s;
    for (auto &c : lower) c = (char)std::tolower((unsigned char)c);
    auto pos = lower.find("select ");
    if (pos != 0) return false;
    auto wherePos = lower.find(" where ");
    std::string selPart = (wherePos == std::string::npos) ? s.substr(7) : s.substr(7, wherePos - 7);
    std::string wherePart;
    if (wherePos != std::string::npos) wherePart = s.substr(wherePos + 7);

    // parse select columns
    out.select_cols.clear();
    std::istringstream ss(selPart);
    std::string token;
    while (std::getline(ss, token, ',')) out.select_cols.push_back(trim(token));

    if (wherePart.empty()) return true;

    // wherePart: col OP value
    std::istringstream ws(wherePart);
    std::string col, op, val;
    if (!(ws >> col >> op)) return false;
    std::getline(ws, val);
    val = trim(val);
    // strip optional quotes
    if (!val.empty() && val.front() == '"' && val.back() == '"' && val.size() >= 2) val = val.substr(1, val.size()-2);

    out.where_col = col;
    out.op = op;
    out.where_val = val;
    return true;
}

static bool numeric_compare(const std::string& a, const std::string& b, const std::string& op) {
    try {
        double da = std::stod(a);
        double db = std::stod(b);
        if (op == "=") return da == db;
        if (op == ">") return da > db;
        if (op == "<") return da < db;
    } catch (...) {
        return false;
    }
    return false;
}

std::vector<std::vector<std::string>> execute_query(const Table& t, const Query& q) {
    std::vector<std::vector<std::string>> result;
    // map selected cols to indices
    std::vector<int> scols;
    for (auto &c : q.select_cols) {
        int idx = t.column_index(c);
        if (idx < 0) scols.push_back(-1);
        else scols.push_back(idx);
    }

    int whereIdx = -1;
    if (!q.where_col.empty()) whereIdx = t.column_index(q.where_col);

    std::vector<size_t> candidates;
    if (whereIdx >= 0 && q.op == "=") {
        // try index
        auto idxres = t.lookup_index((size_t)whereIdx, q.where_val);
        if (!idxres.empty()) candidates = idxres;
    }
    if (candidates.empty()) {
        candidates.resize(t.rows.size());
        for (size_t i = 0; i < t.rows.size(); ++i) candidates[i] = i;
    }

    for (size_t rid : candidates) {
        const auto &row = t.rows[rid];
        bool ok = true;
        if (whereIdx >= 0 && !q.where_col.empty()) {
            const std::string &cell = row[whereIdx];
            if (q.op == "=") ok = (cell == q.where_val);
            else ok = numeric_compare(cell, q.where_val, q.op);
        }
        if (ok) {
            std::vector<std::string> outRow;
            for (int si : scols) {
                if (si < 0) outRow.push_back(""); else outRow.push_back(row[si]);
            }
            result.push_back(std::move(outRow));
        }
    }
    return result;
}
