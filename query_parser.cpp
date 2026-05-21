// ─────────────────────────────────────────────────────────────────────────────
// query_parser.cpp
//
// Autocorrect strategy
// ─────────────────────────────────────────────────────────────────────────────
// Every token that sits in a keyword-position goes through resolveInPlace()
// which:
//   1. Checks for an exact case-insensitive match  → normalise silently
//   2. Finds the closest keyword (Levenshtein ≤ 2) → show confirmation prompt
//   3. Updates toks[pos].value with the result
//
// Updating in-place is the critical detail: if the outer clause-loop and the
// inner AND/OR loop both inspect the same token, only the first call ever
// prompts the user — the second call sees the already-corrected value and
// matches silently (or sees the rejected typo and falls through to an error).
// ─────────────────────────────────────────────────────────────────────────────
#include "query_parser.h"
#include "fuzzy.h"
#include <iostream>
#include <algorithm>
#include <cctype>

// ─── Helpers ──────────────────────────────────────────────────────────────────

static std::string toUpper(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return u;
}

bool QueryParser::atEnd(const std::vector<Token>& t, size_t p) {
    return p >= t.size();
}
std::string QueryParser::peek(const std::vector<Token>& t, size_t p) {
    return (p < t.size()) ? t[p].value : "";
}
std::string QueryParser::consume(const std::vector<Token>& t, size_t& p) {
    return (p < t.size()) ? t[p++].value : "";
}

std::string QueryParser::resolveKeyword(const std::string& tok) {
    // Only attempt correction on pure-alphabetic tokens (not numbers, not
    // column.path style identifiers, not operators).
    if (tok.size() < 2) return toUpper(tok);
    bool wordLike = std::all_of(tok.begin(), tok.end(),
        [](unsigned char c){ return std::isalpha(c) || c == '_'; });
    if (!wordLike) return tok;
    return Fuzzy::autocorrect(tok);
}

std::string QueryParser::resolveInPlace(std::vector<Token>& toks, size_t pos) {
    if (pos >= toks.size() || toks[pos].is_string) return peek(toks, pos);
    toks[pos].value = resolveKeyword(toks[pos].value);
    return toks[pos].value;
}

std::optional<Op> QueryParser::parseOp(const std::string& s) {
    if (s == "=")           return Op::EQ;
    if (s == "!=")          return Op::NEQ;
    if (s == ">")           return Op::GT;
    if (s == "<")           return Op::LT;
    if (s == ">=")          return Op::GTE;
    if (s == "<=")          return Op::LTE;
    if (toUpper(s) == "LIKE") return Op::LIKE;
    return std::nullopt;
}

// ─── Tokeniser ────────────────────────────────────────────────────────────────

std::vector<QueryParser::Token> QueryParser::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;
    auto isWS = [](char c){ return c==' '||c=='\t'||c=='\r'||c=='\n'; };

    while (i < input.size()) {
        if (isWS(input[i])) { ++i; continue; }

        // Quoted string literal
        if (input[i] == '\'' || input[i] == '"') {
            char q = input[i++];
            std::string val;
            while (i < input.size() && input[i] != q) {
                if (input[i]=='\\' && i+1<input.size()) { ++i; val+=input[i++]; }
                else val += input[i++];
            }
            if (i < input.size()) ++i;
            tokens.push_back({val, true});
            continue;
        }

        // Two-char operators
        if (i+1 < input.size()) {
            std::string two = input.substr(i,2);
            if (two=="!="||two==">="||two=="<=") {
                tokens.push_back({two,false}); i+=2; continue;
            }
        }

        // Single-char punctuation/operators
        char c = input[i];
        if (c=='='||c=='<'||c=='>'||c==','||c=='('||c==')'||c=='*') {
            tokens.push_back({std::string(1,c),false}); ++i; continue;
        }

        // Word / identifier / number
        std::string word;
        while (i < input.size() && !isWS(input[i]) &&
               input[i]!=',' && input[i]!='(' && input[i]!=')' &&
               input[i]!='\''&& input[i]!='"' &&
               input[i]!='=' && input[i]!='<' && input[i]!='>' &&
               input[i]!='!') {
            word += input[i++];
        }
        if (!word.empty()) tokens.push_back({word,false});
    }
    return tokens;
}

// ─── Condition parser ─────────────────────────────────────────────────────────

std::optional<Condition> QueryParser::parseCondition(
        std::vector<Token>& toks, size_t& pos) {
    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected condition\n"; return std::nullopt;
    }
    Condition c;
    c.column = consume(toks, pos);   // column name — NOT a keyword position

    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected operator after '" << c.column << "'\n";
        return std::nullopt;
    }
    // Operator: could be LIKE (alphabetic) → resolve as keyword first,
    // then consume the (now-corrected) token.
    if (!toks[pos].is_string) resolveInPlace(toks, pos);
    std::string op_tok = consume(toks, pos);
    auto op = parseOp(op_tok);
    if (!op) {
        std::cerr << "  [error] Unknown operator '" << op_tok << "'\n";
        return std::nullopt;
    }
    c.op = *op;

    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected value after operator\n"; return std::nullopt;
    }
    c.value = consume(toks, pos);    // literal value — NOT a keyword position
    return c;
}

// ─── SELECT parser ────────────────────────────────────────────────────────────

std::optional<Query> QueryParser::parseSelect(
        std::vector<Token>& toks, size_t& pos) {
    Query q;

    // ── DISTINCT (optional) ───────────────────────────────────────────────
    // Resolve in-place first so the value is normalised before we check it.
    if (!atEnd(toks, pos)) resolveInPlace(toks, pos);
    if (!atEnd(toks, pos) && toks[pos].value == "DISTINCT") {
        consume(toks, pos);
        q.distinct = true;
    }

    // ── Column list ───────────────────────────────────────────────────────
    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected column list after SELECT\n";
        return std::nullopt;
    }

    // Re-resolve in case we just consumed DISTINCT
    if (!atEnd(toks, pos) && !toks[pos].is_string) resolveInPlace(toks, pos);
    std::string first = peek(toks, pos);

    if (first == "*") {
        consume(toks, pos); q.select_all = true;

    } else if (first == "COUNT") {
        consume(toks, pos);
        if (peek(toks,pos)=="(")  consume(toks,pos);
        if (peek(toks,pos)=="*")  consume(toks,pos);
        if (peek(toks,pos)==")")  consume(toks,pos);
        q.count_only = true; q.select_all = true;

    } else {
        // Named columns — identifiers, not keyword-position
        while (!atEnd(toks, pos)) {
            q.select_cols.push_back(consume(toks, pos));
            if (!atEnd(toks,pos) && peek(toks,pos)==",") consume(toks,pos);
            else break;
        }
    }

    // ── FROM ──────────────────────────────────────────────────────────────
    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected FROM\n"; return std::nullopt;
    }
    {
        std::string kw = resolveInPlace(toks, pos);
        consume(toks, pos);
        if (kw != "FROM") {
            std::cerr << "  [error] Expected FROM, got '" << kw << "'\n";
            return std::nullopt;
        }
    }

    if (atEnd(toks, pos)) {
        std::cerr << "  [error] Expected table name after FROM\n"; return std::nullopt;
    }
    q.from_table = consume(toks, pos);   // table name — not keyword-position

    // ── Optional clauses ──────────────────────────────────────────────────
    while (!atEnd(toks, pos)) {
        // Resolve the clause keyword in-place before inspecting it.
        std::string kw = resolveInPlace(toks, pos);

        if (kw == "WHERE") {
            consume(toks, pos);

            auto cond = parseCondition(toks, pos);
            if (!cond) return std::nullopt;
            q.conditions.push_back(*cond);

            // AND / OR chains — FIXED: also resolve in-place here so typos
            // like "ADN", "OOR", "annd" are caught inside this inner loop
            // and never bubble up to the outer clause loop.
            while (!atEnd(toks, pos)) {
                std::string logic = resolveInPlace(toks, pos);  // ← was toUpper()
                if (logic != "AND" && logic != "OR") break;
                consume(toks, pos);
                q.condition_ops.push_back(logic == "AND" ? LogicOp::AND : LogicOp::OR);

                auto c2 = parseCondition(toks, pos);
                if (!c2) return std::nullopt;
                q.conditions.push_back(*c2);
            }

        } else if (kw == "ORDER") {
            consume(toks, pos);
            std::string by = resolveInPlace(toks, pos);   // catches "BI", "b", etc.
            consume(toks, pos);
            if (by != "BY") {
                std::cerr << "  [error] Expected BY after ORDER\n"; return std::nullopt;
            }
            if (atEnd(toks, pos)) {
                std::cerr << "  [error] Expected column after ORDER BY\n"; return std::nullopt;
            }
            q.order_by_col = consume(toks, pos);   // column name

            // ASC / DESC — FIXED: resolve in-place so "ACE", "DECS", etc. work
            if (!atEnd(toks, pos)) {
                std::string dir = resolveInPlace(toks, pos);  // ← was toUpper()
                if (dir == "ASC" || dir == "DESC") {
                    consume(toks, pos);
                    q.order_asc = (dir == "ASC");
                }
                // If it's something else, leave it for the next clause iteration
            }

        } else if (kw == "LIMIT") {
            consume(toks, pos);
            if (atEnd(toks, pos)) {
                std::cerr << "  [error] Expected integer after LIMIT\n"; return std::nullopt;
            }
            try { q.limit = std::stoi(consume(toks, pos)); }
            catch (...) {
                std::cerr << "  [error] LIMIT value must be an integer\n"; return std::nullopt;
            }

        } else {
            std::cerr << "  [error] Unexpected token '" << peek(toks,pos) << "'\n";
            return std::nullopt;
        }
    }
    return q;
}

// ─── Meta-command parser ──────────────────────────────────────────────────────

std::optional<MetaCmd> QueryParser::parseMeta(
        std::vector<Token>& toks, size_t& pos) {
    std::string kw = toUpper(consume(toks, pos));

    if (kw == "LOAD") {
        if (atEnd(toks, pos)) {
            std::cerr << "  [error] LOAD requires a filename\n"; return std::nullopt;
        }
        return MetaCmd{MetaKind::LOAD, consume(toks, pos)};
    }
    if (kw == "HELP")                    return MetaCmd{MetaKind::HELP,""};
    if (kw == "EXIT" || kw == "QUIT")    return MetaCmd{MetaKind::EXIT,""};

    if (kw == "SHOW") {
        if (atEnd(toks, pos)) return MetaCmd{MetaKind::SHOW_TABLES,""};
        std::string what = toUpper(consume(toks, pos));
        if (what == "TABLES")  return MetaCmd{MetaKind::SHOW_TABLES,""};
        if (what == "COLUMNS") {
            std::string t = atEnd(toks,pos) ? "" : consume(toks,pos);
            return MetaCmd{MetaKind::SHOW_COLUMNS, t};
        }
        if (what == "INDEX") {
            std::string t = atEnd(toks,pos) ? "" : consume(toks,pos);
            return MetaCmd{MetaKind::SHOW_INDEX, t};
        }
        std::cerr << "  [error] Unknown SHOW target '" << what << "'\n";
        return std::nullopt;
    }
    std::cerr << "  [error] Unknown command '" << kw << "'\n";
    return std::nullopt;
}

// ─── Top-level entry ──────────────────────────────────────────────────────────

std::optional<ParseResult> QueryParser::parse(const std::string& input) {
    auto toks = tokenize(input);
    if (toks.empty()) return std::nullopt;

    size_t pos = 0;
    // Resolve the first token in-place (the command keyword)
    std::string first = resolveInPlace(toks, pos);

    if (first == "SELECT") {
        ++pos;
        auto q = parseSelect(toks, pos);
        if (!q) return std::nullopt;
        return ParseResult{*q};
    }
    auto m = parseMeta(toks, pos);
    if (!m) return std::nullopt;
    return ParseResult{*m};
}
