#pragma once

#include "utils.hpp"

#define EDIT_DISTANCE_THRESHOLD 5

bool is_valid_word(const std::string &token)
{
    // We assume that multibyte characters are always valid (e.g. "à", "á", "ê", ...).
    // Thus, we only need to check characters from U+0000 to U+007F.
    return !token.empty() &&
           std::all_of(
               std::execution::par_unseq, // TODO: Profile performance
               token.begin(), token.end(),
               [](const char &c)
               {
                   return (c & static_cast<char>(0x80)) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
               });
}

std::vector<std::string> import_wordlist(const char *wordlist_path)
{
    std::fstream input(wordlist_path, std::ios::in);
    if (!input)
    {
        throw std::runtime_error(format("Failed to open \"%s\"", wordlist_path));
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

std::vector<std::string> read_corpus_sentence(const char *corpus_path)
{
    static std::fstream input(corpus_path, std::ios::in);
    if (!input)
    {
        throw std::runtime_error(format("Failed to open \"%s\"", corpus_path));
    }

    std::vector<std::string> words;
    std::string word;
    while (input >> word)
    {
        bool end = false;
        char last = word.back();
        if ((last & static_cast<char>(0x80)) == static_cast<char>(0))
        {
            // Last byte represents a character.
            if (!((last >= 'A' && last <= 'Z') || (last >= 'a' && last <= 'z')))
            {
                // Last character is not an alphabet character.
                word.pop_back();

                if (last == '.')
                {
                    end = true;
                }
            }
        }

        if (is_valid_word(word))
        {
            words.push_back(word);
        }

        if (end)
        {
            break;
        }
    }

    return words;
}
