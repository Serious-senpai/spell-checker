#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <data.hpp>
#include <distance.hpp>
#include <standard.hpp>
#include <utils.hpp>

namespace py = pybind11;

std::unordered_map<std::string, uint32_t> token_map;
std::vector<std::string> reversed_token_map;
std::unordered_map<uint64_t, unsigned int> frequency;
std::vector<std::pair<uint64_t, unsigned int>> frequency_forward;
std::vector<std::pair<uint64_t, unsigned int>> frequency_backward;
std::unordered_set<std::string> wordlist_set;

void initialize(
    const std::string &frequency_path,
    const std::string &wordlist_path)
{
    // Clear all global states
    token_map.clear();
    reversed_token_map.clear();
    frequency.clear();
    frequency_forward.clear();
    frequency_backward.clear();
    wordlist_set.clear();

    // Populate `frequency`, `token_map` and `reversed_token_map`
    std::fstream frequency_input(frequency_path, std::ios::in);
    if (frequency_input)
    {
        std::string token;
        while (frequency_input >> token)
        {
            auto first = tokenize(token, token_map);

            frequency_input >> token;
            auto second = tokenize(token, token_map);

            unsigned int freq;
            frequency_input >> freq;

            frequency[(static_cast<uint64_t>(first) << 32) | second] = freq;
        }

        frequency_input.close();

        index_tokens(token_map, reversed_token_map);
    }
    else
    {
        throw std::runtime_error(utils::format("Failed to read \"%s\"", frequency_path.c_str()));
    }

    // Populate `frequency_forward`
    frequency_forward.assign(frequency.begin(), frequency.end());
    std::sort(frequency_forward.begin(), frequency_forward.end());

    // Populate `frequency_backward`
    frequency_backward.clear();
    for (const auto &[mask, freq] : frequency)
    {
        frequency_backward.emplace_back(std::rotl(mask, 32), freq);
    }
    std::sort(frequency_backward.begin(), frequency_backward.end());

    // Populate `wordlist_set`
    std::fstream wordlist_file(wordlist_path, std::ios::in);
    if (!wordlist_file)
    {
        throw std::runtime_error(utils::format("Failed to read \"%s\"", wordlist_path.c_str()));
    }

    std::string word;
    while (wordlist_file >> word)
    {
        utils::to_lower(word);
        std::replace(word.begin(), word.end(), '_', ' ');
        wordlist_set.insert(word);
    }
}

std::string inference(
    const std::string &input,
    const std::size_t &edit_distance_threshold,
    const std::size_t &max_candidates_per_token,
    const double &edit_penalty_factor)
{
    std::vector<std::string> tokens;
    std::stringstream input_buf(input), output;

    const auto process_tokens = [&](bool prepend_space) -> bool
    {
        // std::cerr << "Processing " << tokens << std::endl;
        if (tokens.empty())
        {
            return false;
        }

        // Get the first and last byte of the token group.
        // If they're not tokenizable char, they must be in the ASCII range.
        auto first_char = tokens.front().front();
        auto last_char = tokens.back().back();

        bool first_valid = is_tokenizable_char(first_char);
        bool last_valid = is_tokenizable_char(last_char);

        if (!first_valid)
        {
            // We will have to prepend `first_char` later.
            tokens.front().erase(0, 1);
        }
        if (!last_valid)
        {
            // We will have to append `last_char` later.
            tokens.back().pop_back();
        }

        std::vector<std::string> lowercase(tokens);
        for (auto &token : lowercase)
        {
            utils::to_lower(token);
        }

        // std::cerr << "lowercase = " << lowercase << std::endl;

        std::vector<std::vector<std::size_t>> combined;
        combine_tokens(lowercase, wordlist_set, combined);
        // std::cerr << "combined = " << combined << std::endl;

        std::vector<bool> inspection(tokens.size());
        for (const auto &indices : combined)
        {
            if (indices.size() == 1)
            {
                inspection[indices[0]] = true;
            }
        }

        // Types of token cases:
        // 0 - first letter uppercase
        // 1 - all uppercase
        // 2 - the rest (treat as all lowercase)
        std::vector<int> case_types(tokens.size(), -1);

        // Calculate `case_types`
        for (std::size_t i = 0; i < tokens.size(); i++)
        {
            if (inspection[i])
            {
                // std::cerr << "Calculating case type for \"" << tokens[i] << "\"..." << std::endl;
                if (utils::is_upper(tokens[i].data()))
                {
                    // The first character is uppercase
                    bool skip_first_flag = true, has_upper = false, all_upper = true;
                    for (auto &c : tokens[i])
                    {
                        if (utils::is_utf8_char(&c))
                        {
                            if (skip_first_flag)
                            {
                                skip_first_flag = false;
                                continue;
                            }

                            if (utils::is_upper(&c))
                            {
                                has_upper = true;
                            }
                            else
                            {
                                all_upper = false;
                            }
                        }
                    }

                    if (all_upper)
                    {
                        // All characters are uppercase
                        case_types[i] = 1;
                    }
                    else if (has_upper)
                    {
                        // Not all characters are uppercase, but at least 1 of them is
                        case_types[i] = 2;
                    }
                    else
                    {
                        // No uppercase characters
                        case_types[i] = 0;
                    }
                }
                else
                {
                    // The first character is lowercase, so the token clearly belongs to type 2
                    case_types[i] = 2;
                }
            }
        }

        // std::cerr << "case_types = " << case_types << std::endl;

        // Perform spell-checking in `lowercase`
        for (std::size_t i = 0; i < lowercase.size(); i++)
        {
            if (inspection[i])
            {
                std::unordered_map<uint32_t, unsigned int> left, right;
                if (i > 0)
                {
                    auto iter = token_map.find(lowercase[i - 1]);
                    if (iter != token_map.end())
                    {
                        const uint64_t tokenized = iter->second;
                        for (
                            auto it = std::lower_bound(
                                frequency_forward.begin(),
                                frequency_forward.end(),
                                std::make_pair<uint64_t, unsigned int>(tokenized << 32, 0));
                            it != frequency_forward.end() && (it->first >> 32) == tokenized;
                            it++)
                        {
                            left[it->first & 0xFFFFFFFF] = it->second;
                        }
                    }
                }

                if (i + 1 < lowercase.size())
                {
                    auto iter = token_map.find(lowercase[i + 1]);
                    if (iter != token_map.end())
                    {
                        const uint64_t tokenized = iter->second;
                        for (
                            auto it = std::lower_bound(
                                frequency_backward.begin(),
                                frequency_backward.end(),
                                std::make_pair<uint64_t, unsigned int>(tokenized << 32, 0));
                            it != frequency_backward.end() && (it->first >> 32) == tokenized;
                            it++)
                        {
                            right[it->first & 0xFFFFFFFF] += it->second;
                        }
                    }
                }

                if (left.empty() && right.empty())
                {
                    continue;
                }

                double total_left = std::accumulate(
                    left.begin(), left.end(),
                    static_cast<unsigned int>(0),
                    [](unsigned int sum, const std::pair<uint32_t, unsigned int> &p)
                    { return sum + p.second; });
                double total_right = std::accumulate(
                    right.begin(), right.end(),
                    static_cast<unsigned int>(0),
                    [](unsigned int sum, const std::pair<uint32_t, unsigned int> &p)
                    { return sum + p.second; });

                std::unordered_map<uint32_t, double> scores;
                if (left.empty())
                {
                    for (const auto &[candidate, score] : right)
                    {
                        scores[candidate] = static_cast<double>(score) / total_right;
                    }
                }
                else if (right.empty())
                {
                    for (const auto &[candidate, score] : left)
                    {
                        scores[candidate] = static_cast<double>(score) / total_left;
                    }
                }
                else
                {
                    for (const auto &[candidate, score] : left)
                    {
                        const auto x = static_cast<double>(score) / total_left;
                        const auto y = static_cast<double>(right[candidate]) / total_right;
                        scores[candidate] = utils::sqrt(x * y);
                    }
                }

                std::vector<std::pair<double, uint32_t>> candidates;
                for (const auto &[index, score] : scores)
                {
                    candidates.emplace_back(score, index);
                }
                std::sort(candidates.begin(), candidates.end(), std::greater<>());
                candidates.resize(std::min(candidates.size(), max_candidates_per_token));

                double max_fitness = std::numeric_limits<double>::min();
                uint32_t result = static_cast<uint32_t>(-1);
                for (const auto &[score, index] : candidates)
                {
                    std::string word = reversed_token_map[index];
                    auto d = damerau_levenshtein(lowercase[i], word);
                    auto fitness = static_cast<double>(score) * std::pow(edit_penalty_factor, d);
                    // std::cerr << "Comparing \"" << lowercase[i] << "\" and \"" << word << "\" with d = " << d << ", score = " << score << std::endl;                    if (d <= edit_distance_threshold && fitness > max_fitness)
                    if (d <= edit_distance_threshold && fitness > max_fitness)
                    {
                        max_fitness = fitness;
                        result = index;
                    }
                }

                if (result != static_cast<uint32_t>(-1))
                {
                    lowercase[i] = reversed_token_map[result];
                }
            }
        }

        // Replace incorrect tokens in `tokens` with the correct ones in `lowercase`
        for (std::size_t i = 0; i < lowercase.size(); i++)
        {
            if (inspection[i])
            {
                tokens[i] = lowercase[i];
                if (case_types[i] == 0)
                {
                    utils::capitalize(tokens[i].data());
                }
                else if (case_types[i] == 1)
                {
                    for (auto &c : tokens[i])
                    {
                        if (utils::is_utf8_char(&c))
                        {
                            utils::capitalize(&c);
                        }
                    }
                }
            }
        }

        if (!first_valid)
        {
            tokens.front().insert(0, 1, first_char);
        }
        if (!last_valid)
        {
            tokens.back().push_back(last_char);
        }

        if (prepend_space)
        {
            output << ' ';
        }
        output << tokens[0];
        for (std::size_t i = 1; i < tokens.size(); i++)
        {
            output << ' ' << tokens[i];
        }

        tokens.clear();
        return true;
    };

    std::string line, token;
    while (std::getline(input_buf, line))
    {
        bool is_first_token_group = true;
        std::istringstream buffer(line);
        while (buffer >> token)
        {
            // `token` has at least 1 character
            bool first_valid = is_tokenizable_char(token.front());
            bool mid_valid = std::all_of(token.begin() + 1, token.end() - 1, is_tokenizable_char);
            bool last_valid = is_tokenizable_char(token.back());

            auto mask = (first_valid << 2) | (mid_valid << 1) | last_valid;
            // std::cerr << "Examining \"" << token << "\", mask = " << first_valid << mid_valid << last_valid << std::endl;
            if (mask == 0b111)
            {
                tokens.push_back(token);
            }
            else if (mask == 0b011)
            {
                if (process_tokens(!is_first_token_group))
                {
                    is_first_token_group = false;
                }

                tokens.push_back(token);
            }
            else
            {
                if (mask == 0b110)
                {
                    tokens.push_back(token);
                }

                if (process_tokens(!is_first_token_group))
                {
                    is_first_token_group = false;
                }

                if (mask != 0b110)
                {
                    if (!is_first_token_group)
                    {
                        output << ' ';
                    }
                    output << token;
                    is_first_token_group = false;
                }
            }
        }

        if (!tokens.empty())
        {
            process_tokens(!is_first_token_group);
        }

        output << '\n';
    }

    return output.str();
}

PYBIND11_MODULE(c_utils, m)
{
    m.def(
        "initialize", &initialize,
        py::kw_only(),
        py::arg("frequency_path"),
        py::arg("wordlist_path"));
    m.def(
        "inference", &inference,
        py::arg("input"),
        py::kw_only(),
        py::arg("edit_distance_threshold"),
        py::arg("max_candidates_per_token"),
        py::arg("edit_penalty_factor"));
}
