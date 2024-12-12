#pragma once

#include "utils.hpp"

#define EDIT_DISTANCE_THRESHOLD 5
#define THREADS_COUNT 8
#define FREQUENCY_RECORD_LIMIT 300000

void combine_tokens(
    const std::vector<std::string> &sentence,
    const std::unordered_map<std::string, std::size_t> &wordlist_map,
    std::vector<std::string> &tokens)
{
    std::string current;
    tokens.clear();
    for (std::size_t i = 0; i < sentence.size(); i++)
    {
        current = sentence[i];
        while (++i < sentence.size())
        {
            std::string next_word = current + ' ' + sentence[i];
            if (wordlist_map.find(next_word) != wordlist_map.end())
            {
                current = next_word;
            }
            else
            {
                i--;
                break;
            }
        }

        tokens.push_back(current);
    }
}

bool is_valid_word(const std::string &token)
{
    // We assume that multibyte characters are always valid (e.g. "à", "á", "ê", ...).
    // Thus, we only need to check characters from U+0000 to U+007F.
    return !token.empty() &&
           std::all_of(
               // std::execution::par_unseq, // Profiled: actually makes it worse
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

bool read_corpus_sentence(std::istream &input, std::vector<std::string> &sentence)
{
    if (!input)
    {
        return false;
    }

    std::string word;
    sentence.clear();
    while (input >> word)
    {
        bool end = false;
        char last = word.back();
        if (!(last & static_cast<char>(0x80)))
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
            sentence.push_back(word);
        }

        if (end)
        {
            break;
        }
    }

    return true;
}

template <std::size_t _Limit>
class tuple_frequency
{
private:
    /// @brief Map to track the frequency of each tuple
    std::unordered_map<uint64_t, std::list<std::pair<unsigned int, uint64_t>>::iterator> _map;

    /// @brief List to track the frequency of each tuple
    std::list<std::pair<unsigned int, uint64_t>> _nodes;

    /// @brief Multiset to track the lowest frequency
    std::multiset<unsigned int> _values;

    /// @brief Remove elements with the lowest frequency, prioritize least recently used ones.
    void _prune()
    {
        auto nodes_iter = _nodes.begin();

        // Remove exactly 1 element after each iteration
        for (auto iter = _values.begin(); size() > _Limit;)
        {
            const auto remove_val = *iter;
            for (auto it = nodes_iter;; it++)
            {
                if (it->first == remove_val)
                {
                    _map.erase(it->second);        // Updated _map
                    nodes_iter = _nodes.erase(it); // Updated _nodes
                    break;
                }
            }

            iter = _values.erase(iter); // Updated _values
            if (*iter != remove_val)
            {
                nodes_iter = _nodes.begin();
            }
        }
    }

public:
    using const_iterator = std::unordered_map<uint64_t, std::list<std::pair<unsigned int, uint64_t>>::iterator>::const_iterator;

    void record(const std::unordered_map<uint64_t, unsigned int> &temp_record)
    {
        for (auto &[key, value] : temp_record)
        {
            auto iter = _map.find(key);
            if (iter == _map.end())
            {
                _nodes.push_back(std::make_pair(value, key)); // Updated _nodes
                _map[key] = std::prev(_nodes.end());          // Updated _map
                _values.insert(value);                        // Updated _values
            }
            else
            {
                auto it = iter->second;

                _values.erase(_values.find(it->first));

                it->first += value;
                _values.insert(it->first); // Updated _values

                _nodes.splice(_nodes.end(), _nodes, it); // Updated _nodes
                // No need to update _map since `_nodes.splice` does not invalidate any iterators.
            }
        }

        _prune();
    }

    void record(const uint64_t &key, const unsigned int &increment)
    {
        auto iter = _map.find(key);
        if (iter == _map.end())
        {
            _nodes.push_back(std::make_pair(increment, key)); // Updated _nodes
            _map[key] = std::prev(_nodes.end());              // Updated _map
            _values.insert(increment);                        // Updated _values

            _prune();
        }
        else
        {
            auto it = iter->second;

            _values.erase(_values.find(it->first));

            it->first += increment;
            _values.insert(it->first); // Updated _values

            _nodes.splice(_nodes.end(), _nodes, it); // Updated _nodes
            // No need to update _map since `_nodes.splice` does not invalidate any iterators.
            // No need to perform pruning since the container size is unchanged.
        }
    }

    std::size_t size() const
    {
        return _nodes.size();
    }

    unsigned int get(const uint64_t &key) const
    {
        auto iter = _map.find(key);
        if (iter == _map.end())
        {
            return 0;
        }

        return iter->second->first;
    }

    const_iterator begin() const
    {
        return _map.begin();
    }

    const_iterator end() const
    {
        return _map.end();
    }
};
