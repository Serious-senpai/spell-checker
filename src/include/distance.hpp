#pragma once

#include "standard.hpp"

template <bool _AllowTransposition>
std::size_t _levenshtein_dp(
    const std::string &first,
    const std::string &second,
    const std::size_t &i,
    const std::size_t &j,
    const std::vector<std::size_t> &offset_i,
    const std::vector<std::size_t> &offset_j,
    std::vector<std::vector<std::size_t>> &dp)
{
    std::size_t &result = dp[i][j];
    if (result != std::numeric_limits<std::size_t>::max())
    {
        return result;
    }

    // std::cerr << std::make_pair(i, j) << " Comparing \"" << first.substr(0, offset_i[i]) << "\" and \"" << second.substr(0, offset_j[j]) << "\"" << std::endl;
    if (i == j && i == 0)
    {
        return result = 0;
    }

    if (i > 0)
    {
        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 1, j, offset_i, offset_j, dp) + 1);
    }

    if (j > 0)
    {
        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i, j - 1, offset_i, offset_j, dp) + 1);
    }

    if (i > 0 && j > 0)
    {
        bool ij_equal = std::equal(
            first.begin() + offset_i[i - 1], first.begin() + offset_i[i],
            second.begin() + offset_j[j - 1], second.begin() + offset_j[j]);

        result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 1, j - 1, offset_i, offset_j, dp) + !ij_equal);

        if constexpr (_AllowTransposition)
        {
            if (i > 1 && j > 1)
            {
                bool ix_j_equal = std::equal(
                    first.begin() + offset_i[i - 2], first.begin() + offset_i[i - 1],
                    second.begin() + offset_j[j - 1], second.begin() + offset_j[j]);
                bool i_jx_equal = std::equal(
                    first.begin() + offset_i[i - 1], first.begin() + offset_i[i],
                    second.begin() + offset_j[j - 2], second.begin() + offset_j[j - 1]);

                if (ix_j_equal && i_jx_equal)
                {
                    result = std::min(result, _levenshtein_dp<_AllowTransposition>(first, second, i - 2, j - 2, offset_i, offset_j, dp) + 1);
                }
            }
        }
    }

    return result;
}

std::size_t damerau_levenshtein(const std::string &first, const std::string &second)
{
    std::size_t n = first.size(), m = second.size();
    std::vector<std::vector<std::size_t>> dp(n + 1, std::vector<std::size_t>(m + 1, std::numeric_limits<std::size_t>::max()));

    std::vector<std::size_t> offset_i;
    for (std::size_t i = 0; i < n; i++)
    {
        if ((first[i] & 0xC0) != 0x80)
        {
            offset_i.push_back(i);
        }
    }
    offset_i.push_back(n);

    std::vector<std::size_t> offset_j;
    for (std::size_t j = 0; j < m; j++)
    {
        if ((second[j] & 0xC0) != 0x80)
        {
            offset_j.push_back(j);
        }
    }
    offset_j.push_back(m);

    return _levenshtein_dp<true>(first, second, offset_i.size() - 1, offset_j.size() - 1, offset_i, offset_j, dp);
}
