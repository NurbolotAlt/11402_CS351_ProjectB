// ─────────────────────────────────────────────────────────────────────────────
// indexer.cpp
// ─────────────────────────────────────────────────────────────────────────────
#include "indexer.h"
#include <iostream>
#include <iomanip>

TableIndex Indexer::build(const Table& table) {
    TableIndex idx;

    for (size_t col = 0; col < table.columns.size(); ++col) {
        const std::string& col_name = table.columns[col];
        ColumnIndex col_idx;

        for (size_t row = 0; row < table.rows.size(); ++row) {
            if (col < table.rows[row].size()) {
                col_idx[table.rows[row][col]].push_back(row);
            }
        }
        idx[col_name] = std::move(col_idx);
    }

    std::cout << "  Indexed " << table.columns.size()
              << " columns for table '" << table.name << "'\n";
    return idx;
}

void Indexer::printStats(const TableIndex& idx, const Table& table) {
    std::string hline40(40, '-');
    std::cout << "\n  +- Index Stats: " << table.name
              << " " << hline40 << "+\n";

    for (const auto& col : table.columns) {
        auto it = idx.find(col);
        size_t unique = (it != idx.end()) ? it->second.size() : 0;
        size_t total  = table.rows.size();
        double selectivity = total > 0
            ? (static_cast<double>(unique) / total) * 100.0
            : 0.0;

        std::cout << "  │  " << std::left << std::setw(20) << col
                  << "  " << std::right << std::setw(6) << unique
                  << " unique / " << std::setw(6) << total
                  << " rows  (" << std::fixed << std::setprecision(1)
                  << std::setw(5) << selectivity << "% selectivity)\n";
    }
    std::string hline57(57, '-');
    std::cout << "  +" << hline57 << "+\n\n";
}
