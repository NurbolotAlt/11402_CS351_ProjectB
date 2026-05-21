#include "csv_parser.h"
#include "indexer.h"
#include "query_parser.h"
#include "query_engine.h"
#include <iostream>
#include <unordered_map>

static std::unordered_map<std::string, Table>      g_tables;
static std::unordered_map<std::string, TableIndex> g_indices;

static void printBanner() {
    std::cout << "\n"
        "  +==============================================================+\n"
        "  |       csv_db  --  CSV Mini Database & Query Engine           |\n"
        "  |                   type HELP for commands                     |\n"
        "  +==============================================================+\n\n";
}

static void printHelp() {
    std::cout << R"(
  +-- Commands -----------------------------------------------------------+
  |                                                                       |
  |  LOAD <file.csv>          Load a CSV file as a queryable table        |
  |  SHOW TABLES              List all loaded tables                      |
  |  SHOW COLUMNS <table>     Show column names                          |
  |  SHOW INDEX   <table>     Show index statistics                      |
  |                                                                       |
  |  SELECT Syntax (SQLite-compatible subset):                            |
  |    SELECT * FROM <table>                                              |
  |    SELECT col1, col2 FROM <table>                                     |
  |    SELECT COUNT(*) FROM <table>                                       |
  |    SELECT DISTINCT col FROM <table>                                   |
  |    ... WHERE col = 'value'                                            |
  |    ... WHERE col > 10 AND col2 LIKE '%text%'                          |
  |    ... WHERE a = 1 OR b = 2                                           |
  |    ... ORDER BY col [ASC|DESC]                                        |
  |    ... LIMIT n                                                        |
  |                                                                       |
  |  Autocorrect: typos in ANY keyword trigger a per-token prompt         |
  |    "selevt * frum t" -> prompts SELECT, then FROM separately          |
  |    "... WHER a=1 ADN b=2" -> prompts WHERE, then AND separately       |
  |    "... ORDER BY col DECS" -> prompts DESC                            |
  |    Case is always ignored: Select = SELECT = sElEcT                  |
  |                                                                       |
  |  EXIT / QUIT                                                          |
  +-----------------------------------------------------------------------+
)";
}

static void handleMeta(const MetaCmd& cmd) {
    switch (cmd.kind) {
    case MetaKind::LOAD: {
        auto tbl = CSVParser::parse(cmd.arg);
        if (!tbl) break;
        const std::string& name = tbl->name;
        g_indices[name] = Indexer::build(*tbl);
        g_tables[name]  = std::move(*tbl);
        break;
    }
    case MetaKind::SHOW_TABLES:
        if (g_tables.empty()) { std::cout << "  (no tables loaded)\n"; break; }
        std::cout << "\n  Loaded tables:\n";
        for (const auto& [n,t]:g_tables)
            std::cout<<"    * "<<n<<"  ("<<t.columns.size()<<" cols, "
                     <<t.rows.size()<<" rows)\n";
        std::cout<<"\n";
        break;
    case MetaKind::SHOW_COLUMNS: {
        auto it=g_tables.find(cmd.arg);
        if (it==g_tables.end()){std::cerr<<"  [error] Table '"<<cmd.arg<<"' not found\n";break;}
        std::cout<<"\n  Columns in '"<<cmd.arg<<"':\n";
        for (size_t i=0;i<it->second.columns.size();++i)
            std::cout<<"    ["<<i<<"]  "<<it->second.columns[i]<<"\n";
        std::cout<<"\n";
        break;
    }
    case MetaKind::SHOW_INDEX: {
        auto ti=g_tables.find(cmd.arg);
        auto ii=g_indices.find(cmd.arg);
        if (ti==g_tables.end()){std::cerr<<"  [error] Table '"<<cmd.arg<<"' not found\n";break;}
        Indexer::printStats(ii->second,ti->second);
        break;
    }
    case MetaKind::HELP: printHelp(); break;
    case MetaKind::EXIT: std::cout<<"  Goodbye.\n"; std::exit(0);
    }
}

static void handleQuery(const Query& q) {
    auto ti=g_tables.find(q.from_table);
    if (ti==g_tables.end()) {
        std::cerr<<"  [error] Table '"<<q.from_table<<"' not found. Use LOAD first.\n";
        return;
    }
    QueryEngine::execute(q, ti->second, g_indices[q.from_table]);
}

int main(int argc, char* argv[]) {
    printBanner();
    for (int i=1;i<argc;++i)
        handleMeta(MetaCmd{MetaKind::LOAD, argv[i]});

    std::string line;
    while (true) {
        std::cout << "csv_db> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) break;

        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        line = line.substr(s);

        auto result = QueryParser::parse(line);
        if (!result) continue;

        if (std::holds_alternative<Query>(*result))
            handleQuery(std::get<Query>(*result));
        else
            handleMeta(std::get<MetaCmd>(*result));
    }
    std::cout<<"\n  (EOF)\n";
    return 0;
}
