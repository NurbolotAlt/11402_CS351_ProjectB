#pragma once
#include "table.h"
#include <string>
#include <vector>
#include <optional>
#include <variant>

enum class MetaKind { LOAD, SHOW_TABLES, SHOW_COLUMNS, SHOW_INDEX, HELP, EXIT };
struct MetaCmd {
    MetaKind    kind;
    std::string arg;
};

using ParseResult = std::variant<Query, MetaCmd>;

class QueryParser {
public:
    static std::optional<ParseResult> parse(const std::string& input);

private:
    struct Token {
        std::string value;
        bool        is_string = false;
    };

    static std::vector<Token>         tokenize(const std::string& input);
    static std::optional<Query>       parseSelect(std::vector<Token>& toks, size_t& pos);
    static std::optional<MetaCmd>     parseMeta  (std::vector<Token>& toks, size_t& pos);
    static std::optional<Condition>   parseCondition(std::vector<Token>& toks, size_t& pos);
    static std::optional<Op>          parseOp(const std::string& s);

    static bool        atEnd  (const std::vector<Token>& t, size_t p);
    static std::string peek   (const std::vector<Token>& t, size_t p);
    static std::string consume(const std::vector<Token>& t, size_t& p);

    // Resolve token at pos in-place (updates toks[pos].value).
    // Returns the resolved value.  Calling again never re-prompts because
    // the token now holds the already-corrected (or rejected) value.
    static std::string resolveInPlace(std::vector<Token>& toks, size_t pos);

    // Applies autocorrect only when token looks like a keyword candidate
    // (alphabetic, ≥2 chars).  Does NOT update the token in-place.
    static std::string resolveKeyword(const std::string& tok);
};
