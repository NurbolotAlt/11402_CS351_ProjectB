# csv_db — CSV Mini Database & Query Engine

A lightweight, interactive database engine written in C++17 that loads CSV files
into memory and queries them with a familiar SQL-like syntax.

---

## Why SQLite-Compatible SQL Syntax?

This project implements a **subset of SQLite's SQL dialect** rather than
inventing a custom DSL.  The reasoning:

| Reason | Detail |
|--------|--------|
| **Familiarity** | SQLite is the world's most-deployed database. Every developer already knows `SELECT … FROM … WHERE`. |
| **Minimal & clean** | SQLite trims all the enterprise bloat (schemas, users, transactions) so there's nothing wasteful to implement. |
| **Transferable skills** | Queries written here work verbatim in SQLite, DuckDB, and most RDBMS. |
| **Well-specified** | The SQLite grammar is fully documented, so parsing rules are unambiguous. |

Specifically supported constructs:

```sql
SELECT *                FROM table
SELECT col1, col2       FROM table
SELECT COUNT(*)         FROM table
SELECT DISTINCT col     FROM table
… WHERE  col  op  value
… WHERE  col  op  val  AND  col2  op  val2
… WHERE  col  op  val  OR   col2  op  val2
… ORDER BY col [ASC | DESC]
… LIMIT n
```

Supported operators: `=`  `!=`  `>`  `<`  `>=`  `<=`  `LIKE`

---

## Project Structure

```
csv_db/
├── table.h           Core data structures (Table, Query, Condition, Op …)
├── csv_parser.h/cpp  RFC-4180 CSV parser (quoted fields, escaped quotes)
├── indexer.h/cpp     Per-column hash-map index builder
├── fuzzy.h/cpp       Levenshtein-distance autocorrect for keywords
├── query_parser.h/cpp  Tokeniser + recursive-descent SQL parser
├── query_engine.h/cpp  Query executor (projection, filtering, ordering)
├── main.cpp          REPL entry point
├── Makefile
├── employees.csv     Sample dataset (15 rows)
└── README.md         This file
```

---

## Building

Requires: **g++ with C++17** support (GCC ≥ 7, Clang ≥ 5).

```bash
make          # compile → produces ./csv_db
make run      # compile + launch REPL
make clean    # remove build artifacts
```

You can also pass CSV files directly on the command line to auto-load them:

```bash
./csv_db employees.csv sales.csv
```

---

## Usage

```
csv_db> LOAD employees.csv
csv_db> SHOW TABLES
csv_db> SHOW COLUMNS employees
csv_db> SHOW INDEX   employees
csv_db> SELECT * FROM employees LIMIT 5
csv_db> EXIT
```

### Meta-Commands

| Command | Description |
|---------|-------------|
| `LOAD <file.csv>` | Parse and index a CSV file |
| `SHOW TABLES` | List all loaded tables |
| `SHOW COLUMNS <table>` | Print ordered column list |
| `SHOW INDEX <table>` | Print per-column index statistics |
| `HELP` | Show syntax reference |
| `EXIT` / `QUIT` | Quit the program |

### Query Examples

```sql
-- All rows
SELECT * FROM employees

-- Named columns
SELECT name, salary FROM employees

-- Filtering with equality (uses index → O(1))
SELECT * FROM employees WHERE department = Engineering

-- Range query (linear scan → O(n))
SELECT name, age FROM employees WHERE age > 30

-- Compound condition
SELECT name, salary FROM employees
    WHERE department = Engineering AND salary >= 90000

-- OR condition
SELECT name, city FROM employees
    WHERE city = 'New York' OR city = Chicago

-- Pattern matching  (% = any sequence,  _ = any single char)
SELECT name FROM employees WHERE name LIKE '%son'
SELECT name FROM employees WHERE name LIKE 'A____%'

-- Count
SELECT COUNT(*) FROM employees WHERE city = Chicago

-- Distinct
SELECT DISTINCT city FROM employees

-- Sorted
SELECT name, salary FROM employees
    WHERE department = Engineering
    ORDER BY salary DESC

-- Limit
SELECT * FROM employees ORDER BY salary DESC LIMIT 3

-- Chained clauses
SELECT DISTINCT city FROM employees
    WHERE salary > 80000
    ORDER BY city ASC
    LIMIT 10
```

---

## Autocorrect Feature

Every **keyword** position in a query is protected by fuzzy matching using the
**Levenshtein (edit) distance** algorithm.

### How it works

1. The tokeniser splits input as usual.
2. When the parser **expects a keyword** and receives an unknown token, it computes
   the edit distance to every known keyword.
3. If the best match has a distance **≤ 2** (i.e., at most 2 insertions, deletions,
   or substitutions away), a confirmation prompt is displayed:

```
  ╔══ Autocorrect ══════════════════════════════════════╗
  ║  Unknown keyword: 'selevt'
  ║  Did you mean   : 'SELECT'  ?
  ╚════════════════════════════════════════════════════╝
  Confirm replacement? [y/N]:
```

4. If the user presses **y**, the corrected keyword is substituted and the query
   continues.  Pressing anything else (including Enter) keeps the original, so the
   parser reports a proper error instead of silently mangling intent.

### Case insensitivity

All keywords are **always** normalised to upper-case before comparison:

```sql
select * from employees        -- works fine
Select * FROM Employees        -- works fine
SELECT * FROM employees        -- canonical form
```

This is handled inside `Fuzzy::autocorrect` — an exact case-insensitive match
never triggers a prompt.

### Design choice: why require confirmation?

Auto-replacing without confirmation would silently corrupt queries where a column
or table name happens to be close to a keyword (e.g., a column named `FORME` is
only 1 edit from `FROM`).  The prompt keeps the user in control.

---

## Indexing Architecture

When a CSV is loaded with `LOAD`, every column is indexed immediately:

```
Table: employees
  Column "department" → {
      "Engineering" → [0, 2, 4, 7, 12],
      "Marketing"   → [1, 6, 10],
      "Sales"       → [3, 8, 11, 14],
      "Management"  → [5, 9, 13]
  }
  ...
```

### Performance trade-offs

| Query type | Index used | Complexity |
|------------|-----------|------------|
| `WHERE col = 'value'` | Hash-map lookup | O(1) average |
| `WHERE col != 'val'` | Linear scan | O(n) |
| `WHERE col > val` | Linear scan | O(n) |
| `WHERE col LIKE 'pat'` | Linear scan | O(n × m) |
| AND of two equality conditions | Index both, then intersect | O(k₁ + k₂) |

A **B-tree index** would make range queries O(log n), but hash-maps are simpler
to implement and cover the very common equality case perfectly.

### Memory overhead

The index is an `unordered_map<string, vector<size_t>>` per column.
For a 1 M-row CSV with 10 columns, expect roughly 2–4× the raw data size in RAM.

---

## Architecture Overview

```
Input string
     │
     ▼
 QueryParser::tokenize()       split on whitespace / operators / quotes
     │
     ▼
 QueryParser::parse()          recursive-descent; calls Fuzzy::autocorrect()
     │                         for every token in keyword position
     ▼
 ParseResult (Query | MetaCmd)
     │
     ├─ MetaCmd → main.cpp handleMeta()
     │
     └─ Query   → QueryEngine::execute()
                        │
                        ├─ evalConditions()   (index or linear scan)
                        ├─ project columns
                        ├─ DISTINCT de-dup
                        ├─ stable_sort (ORDER BY)
                        ├─ truncate   (LIMIT)
                        └─ printResults()     (auto-width table)
```

---

## Known Limitations / Future Work

- No `JOIN` support (would require a nested-loop or hash-join executor)
- No `INSERT` / `UPDATE` / `DELETE` (read-only query engine)
- No persistent storage (all data lives in RAM)
- Range indexes (B-tree) would speed up `>`, `<`, `BETWEEN`
- `GROUP BY` / `HAVING` / aggregate functions beyond `COUNT(*)`
- `NULL` handling is currently absent (empty string used instead)
