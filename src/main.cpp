#include <argparse.hpp>
#include <distance.hpp>
#include <utils.hpp>

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

/**
 * @brief Remove non-alphabet characters from a token.
 */
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
        utils::to_lower(word);
        std::replace(word.begin(), word.end(), '_', ' ');
        wordlist.push_back(word);
    }

    return wordlist;
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

void learn(
    const char *corpus_path,
    const bool verbose,
    std::unordered_map<std::string, uint32_t> &token_map,
    std::unordered_map<uint64_t, unsigned int> &frequency)
{
    std::istream *input_ptr;
    std::fstream file_input;
    if (std::strcmp(corpus_path, "-") == 0)
    {
        input_ptr = &std::cin;
    }
    else
    {
        file_input.open(corpus_path, std::ios::in);
        input_ptr = &file_input;
    }

    std::string token;
    std::vector<uint32_t> tokens;

    const auto process_tokens = [&]()
    {
        const auto size = tokens.size();
        for (std::size_t i = 0; i + 1 < size; i++)
        {
            auto mask = (static_cast<uint64_t>(tokens[i]) << 32) | tokens[i + 1];
            frequency[mask]++;
        }

        tokens.clear();
    };

    const auto time_offset = std::chrono::high_resolution_clock::now();
    unsigned long long counter = 0;
    while (*input_ptr >> token)
    {
        // `token` has at least 1 character
        bool first_valid = is_tokenizable_char(token.front());
        bool mid_valid = std::all_of(token.begin() + 1, token.end() - 1, is_tokenizable_char);
        bool last_valid = is_tokenizable_char(token.back());

        auto mask = (first_valid << 2) | (mid_valid << 1) | last_valid;
        // std::cerr << "Examining \"" << token << "\", mask = " << first_valid << mid_valid << last_valid << std::endl;
        if (mask == 0b111)
        {
            utils::to_lower(token);
            tokens.push_back(tokenize(token, token_map));
        }
        else if (mask == 0b011)
        {
            process_tokens();

            utils::to_lower(token);
            tokens.push_back(tokenize(token.substr(1), token_map));
        }
        else
        {
            if (mask == 0b110)
            {
                token.pop_back();
                utils::to_lower(token);

                tokens.push_back(tokenize(token, token_map));
            }

            process_tokens();
        }

        if (verbose && !(++counter & 0xFFFFF))
        {
            const auto size = input_ptr->tellg();
            auto speed = 1e6l * size;
            speed /= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_offset).count();

            std::cout << "Reading corpus: " << utils::memory_size(size);
            std::cout << " (" << utils::memory_size(speed) << "/s, tuple count = " << frequency.size() << ")      \r" << std::flush;
        }
    }
}

void inference(
    const Namespace &argparse,
    const std::unordered_map<std::string, uint32_t> &token_map,
    const std::vector<std::string> &reversed_token_map,
    const std::vector<std::pair<uint64_t, unsigned int>> &frequency_forward,
    const std::vector<std::pair<uint64_t, unsigned int>> &frequency_backward,
    const std::unordered_set<std::string> &wordlist_set)
{
    std::fstream input(argparse.input_path, std::ios::in);
    std::fstream output(argparse.output_path, std::ios::out);
    std::cout << "Reading \"" << argparse.input_path << "\" and writing to \"" << argparse.output_path << "\"..." << std::endl;

    std::vector<std::string> tokens;
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

                std::unordered_map<uint32_t, unsigned long long> scores;
                for (const auto &[candidate, score] : left)
                {
                    const auto x = static_cast<unsigned long long>(score);
                    const auto y = static_cast<unsigned long long>(right[candidate]);
                    scores[candidate] = x + y + utils::sqrt(x * y);
                }
                for (const auto &[candidate, score] : right)
                {
                    if (left.find(candidate) == left.end())
                    {
                        scores[candidate] = score;
                    }
                }

                std::vector<std::pair<unsigned long long, uint32_t>> candidates;
                for (const auto &[index, score] : scores)
                {
                    candidates.emplace_back(score, index);
                }

                std::sort(candidates.begin(), candidates.end(), std::greater<>());
                candidates.resize(std::min<std::size_t>(candidates.size(), argparse.max_candidates_per_token));

                double max_fitness = std::numeric_limits<double>::min();
                uint32_t result = static_cast<uint32_t>(-1);
                for (const auto &[score, index] : candidates)
                {
                    std::string word = reversed_token_map[index];
                    auto d = damerau_levenshtein(lowercase[i], word);
                    auto fitness = static_cast<double>(score) * std::pow(argparse.edit_penalty_factor, d);
                    if (d <= argparse.edit_distance_threshold && fitness > max_fitness)
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
    while (std::getline(input, line))
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

    input.close();
    output.close();
}

int main(int argc, char **argv)
{
    std::ios_base::sync_with_stdio(false);
    Namespace argparse(argc, argv);
    std::cout << "Command line arguments: " << argparse << std::endl;

    std::unordered_map<std::string, uint32_t> token_map;
    std::vector<std::string> reversed_token_map;

    const auto index_tokens = [&token_map, &reversed_token_map]()
    {
        reversed_token_map.resize(token_map.size());
        for (auto &[token, index] : token_map)
        {
            reversed_token_map[index] = token;
        }
    };

    std::unordered_map<uint64_t, unsigned int> frequency;
    std::fstream frequency_input(argparse.frequency_path, std::ios::in);
    if (frequency_input)
    {
        std::cout << "Importing data from \"" << argparse.frequency_path << "\"..." << std::endl;

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

        index_tokens();
        std::cout << "Found " << frequency.size() << " tuples." << std::endl;
    }
    else
    {
        frequency_input.close();
        std::cout << "Frequency file \"" << argparse.frequency_path << "\" cannot be found. Learning from corpus..." << std::endl;

        token_map.reserve(1 << 24);
        frequency.reserve(1 << 24);
        learn(argparse.corpus_path, argparse.verbose, token_map, frequency);

        std::cout << "\nSaving " << frequency.size() << " tuples to \"" << argparse.frequency_path << "\"..." << std::endl;

        index_tokens();

        std::erase_if(
            frequency,
            [](const std::pair<uint64_t, unsigned int> &p)
            { return p.second < 3; });

        std::fstream frequency_output(argparse.frequency_path, std::ios::out);
        for (auto &[mask, freq] : frequency)
        {
            auto first = mask >> 32, second = mask & 0xFFFFFFFF;
            frequency_output << reversed_token_map[first] << ' ' << reversed_token_map[second] << ' ' << freq << '\n';
        }

        frequency_output.close();
    }

    std::cout << "Hashmap load factor = " << frequency.load_factor() << " with " << frequency.bucket_count() << " buckets." << std::endl;

    auto iter = std::max_element(
        frequency.begin(), frequency.end(),
        [](const auto &lhs, const auto &rhs)
        { return lhs.second < rhs.second; });

    std::cout << "Most frequent tuple: \"" << reversed_token_map[iter->first >> 32] << ' ' << reversed_token_map[iter->first & 0xFFFFFFFF] << "\" with a count of " << iter->second << std::endl;
    std::cout << "Constructing support binary search arrays..." << std::endl;

    std::vector<std::pair<uint64_t, unsigned int>> frequency_forward(frequency.begin(), frequency.end());
    std::sort(frequency_forward.begin(), frequency_forward.end());

    std::vector<std::pair<uint64_t, unsigned int>> frequency_backward;
    for (const auto &[mask, freq] : frequency)
    {
        frequency_backward.emplace_back(std::rotl(mask, 32), freq);
    }
    std::sort(frequency_backward.begin(), frequency_backward.end());

    std::cout << "Constructed binary search arrays of size " << frequency_forward.size() << " and " << frequency_backward.size() << std::endl;

    const auto wordlist = import_wordlist(argparse.wordlist_path);
    if (argparse.interactive)
    {
        std::signal(SIGINT, SIG_IGN);
        std::signal(SIGTERM, SIG_IGN);

        while (true)
        {
            std::string command;
            std::cout << "command>" << std::flush;
            std::getline(std::cin, command);
            if (std::cin.fail() || std::cin.eof())
            {
                std::cin.clear();
                std::cout << std::endl;
                continue;
            }

            if (command == "exit")
            {
                break;
            }
            else if (command == "run")
            {
                inference(
                    argparse,
                    token_map,
                    reversed_token_map,
                    frequency_forward,
                    frequency_backward,
                    std::unordered_set<std::string>(wordlist.begin(), wordlist.end()));
            }
            else
            {
                std::cout << "Unknown command \"" << command << "\"" << std::endl;
            }
        }
    }
    else
    {
        inference(
            argparse,
            token_map,
            reversed_token_map,
            frequency_forward,
            frequency_backward,
            std::unordered_set<std::string>(wordlist.begin(), wordlist.end()));
    }

    return 0;
}
