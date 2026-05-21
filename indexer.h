#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// indexer.h  –  Builds and stores an inverted index per column
//
// Design:
//   For each column, we maintain a hash-map: value -> [row_indices]
//   This gives O(1) average-case lookup for equality (=) conditions.
//   Range queries (>, <, >=, <=) fall back to a linear scan because a
//   hash-map has no ordering.  A B-tree index would support ranges in
//   O(log n), but that's future work.
// ─────────────────────────────────────────────────────────────────────────────
#include "table.h"
#include <unordered_map>
#include <vector>

// column_value -> sorted list of row indices
using ColumnIndex = std::unordered_map<std::string, std::vector<size_t>>;
// column_name  -> its ColumnIndex
using TableIndex  = std::unordered_map<std::string, ColumnIndex>;

class Indexer {
public:
    /// Build a full index for every column in the table.
    static TableIndex build(const Table& table);

    /// Print a summary of each column's unique-value count.
    static void printStats(const TableIndex& idx, const Table& table);
};
