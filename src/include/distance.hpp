#pragma once

#include "standard.hpp"

template <bool _AllowTransposition>
std::size_t _levenshtein_dp(
    const std::string &first,
    const std::string &second,
    const std::size_t &i,
    const std::size_t &j,
    std::vector<std::vector<std::size_t>> &dp)
{
    std::size_t &result = dp[i][j];
    if (result != std::numeric_limits<std::size_t>::max())
    {
        return result;
    }

    if (i == j && i == 0)
    {
        return result = 0;
    }

    if (i > 0)
    {
        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 1, j, dp) + 1);
    }

    if (j > 0)
    {
        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i, j - 1, dp) + 1);
    }

    if (i > 0 && j > 0)
    {
        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 1, j - 1, dp) + (first[i - 1] != second[j - 1]));

        if constexpr (_AllowTransposition)
        {
            if (i > 1 && j > 1 && first[i - 1] == second[j - 2] && first[i - 2] == second[j - 1])
            {
                result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 2, j - 2, dp) + 1);
            }
        }
    }

    return result;
}

std::size_t damerau_levenshtein(const std::string &first, const std::string &second)
{
    std::size_t n = first.size(), m = second.size();
    std::vector<std::vector<std::size_t>> dp(n + 1, std::vector<std::size_t>(m + 1, std::numeric_limits<std::size_t>::max()));
    return _levenshtein_dp<true>(first, second, n, m, dp);
}
