#pragma once
#include <string>
#include <vector>
#include <optional>

class Fuzzy {
public:
    static size_t distance(const std::string& a, const std::string& b);
    static std::optional<std::string> findKeyword(const std::string& token);
    static std::string autocorrect(const std::string& token);

    static const std::vector<std::string> KEYWORDS;
};
