#include "query_engine.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_set>
#include <numeric>
#include <cctype>

// ─── LIKE matching (% = any sequence, _ = any single char) ───────────────────
static bool likeMatch(const std::string& str, const std::string& pat) {
    size_t m = str.size(), n = pat.size();
    std::vector<std::vector<bool>> dp(m+1, std::vector<bool>(n+1,false));
    dp[0][0] = true;
    for (size_t j=1;j<=n;++j) if(pat[j-1]=='%') dp[0][j]=dp[0][j-1];
    for (size_t i=1;i<=m;++i)
        for (size_t j=1;j<=n;++j)
            if (pat[j-1]=='%')  dp[i][j]=dp[i-1][j]||dp[i][j-1];
            else if (pat[j-1]=='_'||std::tolower(str[i-1])==std::tolower(pat[j-1]))
                                dp[i][j]=dp[i-1][j-1];
    return dp[m][n];
}

// ─── Numeric-aware compare (falls back to lexicographic) ─────────────────────
static int cmpValues(const std::string& a, const std::string& b) {
    try   { double da=std::stod(a),db=std::stod(b); return da<db?-1:da>db?1:0; }
    catch (...) { return a.compare(b); }
}

bool QueryEngine::matchesCondition(const std::string& cell, Op op,
                                   const std::string& val) {
    switch (op) {
        case Op::EQ:   return cell == val;
        case Op::NEQ:  return cell != val;
        case Op::GT:   return cmpValues(cell,val) >  0;
        case Op::LT:   return cmpValues(cell,val) <  0;
        case Op::GTE:  return cmpValues(cell,val) >= 0;
        case Op::LTE:  return cmpValues(cell,val) <= 0;
        case Op::LIKE: return likeMatch(cell,val);
    }
    return false;
}

std::vector<size_t> QueryEngine::evalCondition(const Condition& c,
                                               const Table& table,
                                               const TableIndex& idx) {
    std::string lower = c.column;
    std::transform(lower.begin(),lower.end(),lower.begin(),
                   [](unsigned char x){return std::tolower(x);});
    auto col_it = table.col_map.find(lower);
    if (col_it == table.col_map.end()) {
        std::cerr << "  [error] Unknown column '" << c.column << "'\n";
        return {};
    }
    size_t col_idx       = col_it->second;
    const std::string& col_name = table.columns[col_idx];

    // O(1) index path for equality
    if (c.op == Op::EQ) {
        auto ii = idx.find(col_name);
        if (ii != idx.end()) {
            auto vi = ii->second.find(c.value);
            return (vi != ii->second.end()) ? vi->second : std::vector<size_t>{};
        }
    }
    // O(n) linear scan for everything else
    std::vector<size_t> result;
    for (size_t r=0;r<table.rows.size();++r) {
        const std::string& cell = col_idx<table.rows[r].size()
                                      ? table.rows[r][col_idx] : "";
        if (matchesCondition(cell, c.op, c.value)) result.push_back(r);
    }
    return result;
}

std::vector<size_t> QueryEngine::evalConditions(const Query& q,
                                                const Table& table,
                                                const TableIndex& idx) {
    if (q.conditions.empty()) {
        std::vector<size_t> all(table.rows.size());
        std::iota(all.begin(),all.end(),0);
        return all;
    }
    auto current = evalCondition(q.conditions[0], table, idx);
    for (size_t i=0;i<q.condition_ops.size();++i) {
        auto next = evalCondition(q.conditions[i+1], table, idx);
        if (q.condition_ops[i] == LogicOp::AND) {
            std::unordered_set<size_t> s(next.begin(),next.end());
            std::vector<size_t> inter;
            for (size_t r:current) if(s.count(r)) inter.push_back(r);
            current = std::move(inter);
        } else {
            std::unordered_set<size_t> seen(current.begin(),current.end());
            for (size_t r:next) if(seen.insert(r).second) current.push_back(r);
            std::sort(current.begin(),current.end());
        }
    }
    return current;
}

void QueryEngine::execute(const Query& q, const Table& table,
                          const TableIndex& idx) {
    // Resolve projected columns
    std::vector<size_t>      col_indices;
    std::vector<std::string> headers;

    if (q.select_all || q.count_only) {
        for (size_t i=0;i<table.columns.size();++i) {
            col_indices.push_back(i);
            headers.push_back(table.columns[i]);
        }
    } else {
        for (const auto& col : q.select_cols) {
            std::string low=col;
            std::transform(low.begin(),low.end(),low.begin(),
                           [](unsigned char c){return std::tolower(c);});
            auto it=table.col_map.find(low);
            if (it==table.col_map.end()) {
                std::cerr<<"  [error] Unknown column '"<<col<<"'\n"; return;
            }
            col_indices.push_back(it->second);
            headers.push_back(table.columns[it->second]);
        }
    }

    auto row_indices = evalConditions(q, table, idx);

    // Project
    std::vector<std::vector<std::string>> projected;
    projected.reserve(row_indices.size());
    for (size_t r : row_indices) {
        std::vector<std::string> row;
        for (size_t c : col_indices)
            row.push_back(c<table.rows[r].size() ? table.rows[r][c] : "");
        projected.push_back(std::move(row));
    }

    // DISTINCT
    if (q.distinct) {
        std::vector<std::vector<std::string>> dd;
        std::unordered_set<std::string> seen;
        for (auto& row : projected) {
            std::string key;
            for (auto& v:row){key+=v;key+='\x01';}
            if (seen.insert(key).second) dd.push_back(std::move(row));
        }
        projected=std::move(dd);
    }

    // ORDER BY
    if (!q.order_by_col.empty()) {
        std::string low=q.order_by_col;
        std::transform(low.begin(),low.end(),low.begin(),
                       [](unsigned char c){return std::tolower(c);});
        int op=-1;
        for (size_t i=0;i<headers.size();++i) {
            std::string h=headers[i];
            std::transform(h.begin(),h.end(),h.begin(),
                           [](unsigned char c){return std::tolower(c);});
            if(h==low){op=(int)i;break;}
        }
        if (op<0) std::cerr<<"  [warning] ORDER BY column not in projection\n";
        else std::stable_sort(projected.begin(),projected.end(),
            [&](const auto& a,const auto& b){
                return q.order_asc ? cmpValues(a[op],b[op])<0
                                   : cmpValues(a[op],b[op])>0;
            });
    }

    // LIMIT
    if (q.limit>=0 && (int)projected.size()>q.limit)
        projected.resize((size_t)q.limit);

    printResults(projected, headers, q.count_only);
}

std::string QueryEngine::padRight(const std::string& s, size_t w) {
    return s.size()>=w ? s.substr(0,w) : s+std::string(w-s.size(),' ');
}

void QueryEngine::printResults(
        const std::vector<std::vector<std::string>>& rows,
        const std::vector<std::string>& headers,
        bool count_only) {
    if (count_only) {
        std::cout<<"\n  COUNT(*)\n  --------\n  "<<rows.size()<<"\n\n";
        return;
    }
    if (rows.empty()) { std::cout<<"\n  (no rows matched)\n\n"; return; }

    std::vector<size_t> widths(headers.size(),0);
    for (size_t i=0;i<headers.size();++i) widths[i]=headers[i].size();
    for (const auto& row:rows)
        for (size_t i=0;i<row.size()&&i<widths.size();++i)
            widths[i]=std::max(widths[i],row[i].size());

    auto sep=[&](){
        std::cout<<"  +";
        for (size_t w:widths) std::cout<<std::string(w+2,'-')<<"+";
        std::cout<<"\n";
    };

    std::cout<<"\n"; sep();
    std::cout<<"  |";
    for (size_t i=0;i<headers.size();++i)
        std::cout<<" "<<padRight(headers[i],widths[i])<<" |";
    std::cout<<"\n"; sep();
    for (const auto& row:rows) {
        std::cout<<"  |";
        for (size_t i=0;i<headers.size();++i)
            std::cout<<" "<<padRight(i<row.size()?row[i]:"",widths[i])<<" |";
        std::cout<<"\n";
    }
    sep();
    std::cout<<"  "<<rows.size()<<(rows.size()==1?" row":" rows")<<" returned\n\n";
}
