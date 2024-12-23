#pragma once

#include "utils.hpp"

#define EDIT_DISTANCE_THRESHOLD 3

/// @brief Combine multiple tokens into words.
/// @param tokens The vector of tokens in the sentence.
/// @param wordlist_set The wordlist used to recognize multi-token words.
/// @param words The result vector to write the combined indices to.
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

/// @brief Check if a character is a tokenizable one (i.e. is a Vietnamese alphabet character).
///
/// We assume that multibyte characters are always valid (e.g. "à", "á", "ê", ...).
/// Thus, we only need to check characters from U+0000 to U+007F.
bool is_tokenizable_char(const char &c)
{
    return (c & static_cast<char>(0x80)) || std::isalpha(c);
}

/// @brief Remove non-alphabet characters from a token.
void remove_non_alphabet_characters(std::string &token)
{
    auto end = std::remove_if(
        token.begin(), token.end(),
        [](const char &c)
        {
            return !is_tokenizable_char(c);
        });

    token.resize(std::distance(token.begin(), end));
}

/// @brief Readthe wordlist from the specified file location.
/// @param wordlist_path The path to the wordlist file
/// @return The wordlist
std::vector<std::string> import_wordlist(const char *wordlist_path)
{
    std::fstream input(wordlist_path, std::ios::in);
    if (!input)
    {
        throw std::runtime_error(utils::format("Failed to open \"%s\"", wordlist_path));
    }

    std::vector<std::string> wordlist;
    std::string word;
    while (input >> word)
    {
        std::replace(word.begin(), word.end(), '_', ' ');
        wordlist.push_back(word);
    }

    return wordlist;
}

std::map<std::string, std::vector<std::vector<std::string>::const_iterator>>
delete_variants(const std::vector<std::string> &wordlist)
{
    std::map<std::string, std::vector<std::vector<std::string>::const_iterator>> variants;
    for (auto iter = wordlist.begin(); iter != wordlist.end(); iter++)
    {
        variants[*iter].push_back(iter);

        if (iter->size() > 1)
        {
            // Perform deletion of each character. Note that each character may be
            // represented by several bytes (since we're using utf-8).
            // Reference: https://en.wikipedia.org/wiki/UTF-8
            std::vector<std::size_t> offset; // Offsets of characters
            for (std::size_t i = 0; i < iter->size(); i++)
            {
                char byte = iter->at(i);
                if ((byte & static_cast<char>(0xc0)) != static_cast<char>(0x80))
                {
                    offset.push_back(i);
                }
            }

            offset.push_back(iter->size());
            for (std::size_t i = 0; i + 1 < offset.size(); i++)
            {
                std::string variant(*iter);
                variant.erase(offset[i], offset[i + 1] - offset[i]);
                variants[variant].push_back(iter);
            }
        }
    }

    return variants;
}
