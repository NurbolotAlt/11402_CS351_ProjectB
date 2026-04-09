# CSV Mini Database & Query Engine (Project B)

This repository implements a small, educational CSV mini-database and query engine in modern C++ (C++17).

Features:
- Robust CSV parser that handles quoted fields and embedded commas/quotes.
- In-memory table representation with optional simple hash indexes.
- Very small query grammar supporting `SELECT` and `WHERE` with equality (and numeric comparisons).
- Small CLI to load CSV files, build an index, and run queries.

Build (recommended):

1. Create a build directory and run CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

2. Run the binary from the project root (example):

```bash
./build/csvdb examples/data.csv
```

Notes:
- This project uses a tiny, hand-written CSV parser (no external dependencies). You may optionally integrate a library via `vcpkg` or `Conan`.
- The query engine is intentionally simple for teaching: it shows trade-offs between full parsing, indexing, and execution speed.

Quick CLI examples:

- Load a CSV and run a query (from repo root):

```bash
./build/csvdb examples/data.csv
# then at the prompt:
> index id
> select name,age where id = 2
```

Project layout:
- `CMakeLists.txt` — build rules.
- `src/` — source for parser, table, query, and CLI.
- `examples/` — sample CSV files.

Teaching goals:
- CSV parsing (quoted fields, edge cases)
- Indexing trade-offs (memory vs. query speed)
- Simple query grammar design and implementation
- Performance considerations (streaming vs. full load)

If you'd like, I can:
- Add more query operators (`AND`/`OR`, LIKE),
- Add persistent on-disk indexes, or
- Integrate a CSV parsing library via `vcpkg`.

---
Updated: April 2026