#include "fuzzy.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <cctype>

const std::vector<std::string> Fuzzy::KEYWORDS = {
    "SELECT","FROM","WHERE","AND","OR","NOT","LIKE",
    "LIMIT","ALL","LOAD","EXIT","HELP","SHOW","TABLES",
    "COLUMNS","ORDER","BY","ASC","DESC","COUNT","DISTINCT",
    "INDEX","QUIT"
};

static std::string toUpper(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return u;
}

size_t Fuzzy::distance(const std::string& a, const std::string& b) {
    std::string A = toUpper(a), B = toUpper(b);
    size_t m = A.size(), n = B.size();
    std::vector<std::vector<size_t>> dp(m+1, std::vector<size_t>(n+1, 0));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= m; ++i)
        for (size_t j = 1; j <= n; ++j)
            dp[i][j] = (A[i-1] == B[j-1])
                ? dp[i-1][j-1]
                : 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
    return dp[m][n];
}

std::optional<std::string> Fuzzy::findKeyword(const std::string& token) {
    if (token.size() < 2) return std::nullopt;
    std::string upper = toUpper(token);
    for (const auto& kw : KEYWORDS)
        if (upper == kw) return kw;  // exact

    size_t best_d = std::numeric_limits<size_t>::max();
    std::string best;
    for (const auto& kw : KEYWORDS) {
        size_t d = distance(token, kw);
        if (d < best_d) { best_d = d; best = kw; }
    }
    if (best_d > 0 && best_d <= 2) return best;
    return std::nullopt;
}

std::string Fuzzy::autocorrect(const std::string& token) {
    std::string upper = toUpper(token);
    // Exact match: silent normalisation (covers Select, sElEcT, etc.)
    for (const auto& kw : KEYWORDS)
        if (upper == kw) return kw;

    auto suggestion = findKeyword(token);
    if (!suggestion) return token;

    std::cout << "\n  +== Autocorrect ============================================+\n";
    std::cout << "  |  Unknown keyword : '" << token       << "'\n";
    std::cout << "  |  Did you mean    : '" << *suggestion << "'  ?\n";
    std::cout << "  +===========================================================+\n";
    std::cout << "  Confirm replacement? [y/N]: ";
    std::cout.flush();

    std::string ans;
    std::getline(std::cin, ans);
    if (!ans.empty() && std::tolower((unsigned char)ans[0]) == 'y')
        return *suggestion;

    return token;
}
