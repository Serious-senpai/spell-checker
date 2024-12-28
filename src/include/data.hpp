#pragma once

#include "utils.hpp"

/**
 * @brief Combine multiple tokens into words.
 * @param tokens The vector of tokens in the sentence.
 * @param wordlist_set The wordlist used to recognize multi-token words.
 * @param words The result vector to write the combined indices to.
 */
void combine_tokens(
    const std::vector<std::string> &tokens,
    const std::unordered_set<std::string> &wordlist_set,
    std::vector<std::vector<std::size_t>> &words)
{
    std::string current;
    words.clear();
    for (std::size_t i = 0; i < tokens.size(); i++)
    {
        std::vector<std::size_t> indices = {i};
        current = tokens[i];
        while (++i < tokens.size())
        {
            std::string next_word = current + ' ' + tokens[i];
            if (wordlist_set.find(next_word) != wordlist_set.end())
            {
                indices.push_back(i);
                current = next_word;
            }
            else
            {
                i--;
                break;
            }
        }

        words.push_back(indices);
    }
}

/**
 * @brief Check if a character is a tokenizable one (i.e. is a Vietnamese alphabet character).
 *
 * We assume that multibyte characters are always valid (e.g. "à", "á", "ê", ...).
 * Thus, we only need to check characters from U+0000 to U+007F.
 */
bool is_tokenizable_char(const char &c)
{
    return (c & static_cast<char>(0x80)) || std::isalpha(c);
}

void index_tokens(
    const std::unordered_map<std::string, uint32_t> &token_map,
    std::vector<std::string> &reversed_token_map)
{
    reversed_token_map.resize(token_map.size());
    for (const auto &[token, index] : token_map)
    {
        reversed_token_map[index] = token;
    }
}

uint32_t tokenize(const std::string &token, std::unordered_map<std::string, uint32_t> &token_map)
{
    auto iter = token_map.find(token);
    if (iter == token_map.end())
    {
        iter = token_map.emplace(token, token_map.size()).first;
    }

    return iter->second;
}
