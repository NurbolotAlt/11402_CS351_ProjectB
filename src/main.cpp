#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "table.h"
#include "query.h"

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && std::isspace((unsigned char)s[a])) ++a;
    size_t b = s.size(); while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

int main(int argc, char** argv) {
    Table t;
    if (argc < 2) {
        std::cout << "Usage: csvdb <file.csv>\n";
        return 1;
    }
    std::string path = argv[1];
    if (!t.load_csv(path)) {
        std::cerr << "Failed to load: " << path << "\n";
        return 1;
    }
    std::cout << "Loaded " << t.rows.size() << " rows, " << t.columns.size() << " columns.\n";
    std::cout << "Columns: ";
    for (auto &c : t.columns) std::cout << c << " ";
    std::cout << "\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;
        if (line == "exit" || line == "quit") break;
        if (line == "help") {
            std::cout << "Commands:\n  index <col>    -- build hash index on column\n  select ...     -- run a query, e.g. select name where id = 2\n  exit/quit\n";
            continue;
        }
        if (line.rfind("index ", 0) == 0) {
            std::string col = trim(line.substr(6));
            int ci = t.column_index(col);
            if (ci < 0) { std::cout << "Unknown column\n"; continue; }
            t.build_index((size_t)ci);
            std::cout << "Index built on '" << col << "'\n";
            continue;
        }
        if (line.rfind("select ", 0) == 0) {
            Query q;
            if (!parse_query(line, q)) { std::cout << "Failed to parse query\n"; continue; }
            auto rows = execute_query(t, q);
            // print header
            for (size_t i = 0; i < q.select_cols.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << q.select_cols[i];
            }
            std::cout << "\n";
            for (auto &r : rows) {
                for (size_t i = 0; i < r.size(); ++i) {
                    if (i) std::cout << ", ";
                    std::cout << r[i];
                }
                std::cout << "\n";
            }
            std::cout << rows.size() << " row(s)\n";
            continue;
        }
        std::cout << "Unknown command. Type 'help' for commands.\n";
    }
    return 0;
}
