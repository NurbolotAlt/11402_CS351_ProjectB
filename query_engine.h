#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// query_engine.h  –  Executes a parsed Query against a Table + its Index
//
// Execution strategy:
//   1. WHERE evaluation
//      - Equality (=) on an indexed column  →  O(1) index lookup
//      - Everything else                    →  O(n) linear scan
//      - Multiple conditions combined with AND/OR  →  row-set intersection/union
//   2. SELECT projection  (pick requested columns from surviving rows)
//   3. DISTINCT            de-duplicate projected rows
//   4. ORDER BY            std::stable_sort
//   5. LIMIT               truncate
//   6. COUNT(*)            count and suppress row output
// ─────────────────────────────────────────────────────────────────────────────
#include "table.h"
#include "indexer.h"
#include <unordered_map>
#include <string>

class QueryEngine {
public:
    /// Execute a Query and print results to stdout.
    static void execute(const Query&      q,
                        const Table&      table,
                        const TableIndex& idx);

private:
    // ── WHERE evaluation ──────────────────────────────────────────────────
    static std::vector<size_t> evalCondition (const Condition&   c,
                                              const Table&       table,
                                              const TableIndex&  idx);
    static std::vector<size_t> evalConditions(const Query&       q,
                                              const Table&       table,
                                              const TableIndex&  idx);

    static bool matchesCondition(const std::string& cell,
                                 Op op,
                                 const std::string& val);

    // ── Output formatting ─────────────────────────────────────────────────
    static void printResults(const std::vector<std::vector<std::string>>& rows,
                             const std::vector<std::string>& headers,
                             bool count_only);

    static std::string padRight(const std::string& s, size_t w);
};
