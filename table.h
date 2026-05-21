#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// table.h  –  Core data structures for csv_db
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <vector>
#include <unordered_map>

// ── Comparison operators used in WHERE clauses ────────────────────────────────
enum class Op { EQ, NEQ, GT, LT, GTE, LTE, LIKE };

inline std::string opToStr(Op op) {
    switch (op) {
        case Op::EQ:   return "=";
        case Op::NEQ:  return "!=";
        case Op::GT:   return ">";
        case Op::LT:   return "<";
        case Op::GTE:  return ">=";
        case Op::LTE:  return "<=";
        case Op::LIKE: return "LIKE";
    }
    return "?";
}

// ── Logical connectives between WHERE conditions ──────────────────────────────
enum class LogicOp { AND, OR };

// ── A single WHERE predicate  (column  op  value) ────────────────────────────
struct Condition {
    std::string column;
    Op          op;
    std::string value;
};

// ── A fully-parsed SELECT statement ──────────────────────────────────────────
struct Query {
    bool                     select_all    = false; // SELECT *
    bool                     count_only    = false; // SELECT COUNT(*)
    bool                     distinct      = false; // SELECT DISTINCT …
    std::vector<std::string> select_cols;           // named columns (if not *)
    std::string              from_table;
    std::vector<Condition>   conditions;
    std::vector<LogicOp>     condition_ops;         // [n-1] connectives for n conditions
    std::string              order_by_col;
    bool                     order_asc     = true;
    int                      limit         = -1;    // -1 = no limit
};

// ── In-memory representation of one loaded CSV file ──────────────────────────
struct Table {
    std::string                              name;
    std::vector<std::string>                 columns;  // original order
    std::vector<std::vector<std::string>>    rows;
    std::unordered_map<std::string, size_t>  col_map;  // lowercase name -> col index
};
