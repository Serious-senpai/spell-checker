#include <bk_tree.hpp>
#include <config.hpp>
#include <distance.hpp>

void learn(
    const char *corpus_path,
    const bool verbose,
    const std::unordered_map<std::string, std::size_t> &token_map,
    tuple_frequency<FREQUENCY_RECORD_LIMIT> &frequency)
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

    std::streampos size_offset = 0;
    auto time_offset = std::chrono::high_resolution_clock::now();

    std::vector<std::string> sentence;
    std::unordered_map<uint64_t, unsigned int> temp_record;
    for (std::size_t counter = 0; read_single_sentence(*input_ptr, sentence); counter++)
    {
        for (auto &word : sentence)
        {
            utils::to_lower(word);
        }

        std::deque<std::size_t> window;
        std::size_t begin = 0, end = 0;

        for (std::size_t i = 0; i + 2 < sentence.size(); i++)
        {
            // Ensure that window contains subarray [i, i + 3), currently [begin, end)
            while (begin < i)
            {
                window.pop_front();
                begin++;
            }

            bool filled = true;
            while (end < i + 3)
            {
                auto iter = token_map.find(sentence[end]);
                if (iter != token_map.end())
                {
                    window.push_back(iter->second);
                    end++;
                }
                else
                {
                    window.clear();
                    i = end;
                    begin = ++end;

                    filled = false;
                    break;
                }
            }

            if (filled)
            {
                int64_t mask = (window[0] << 32) | (window[1] << 16) | window[2];
                temp_record[mask]++;
            }
        }

        if (!(counter & 0x1FFF))
        {
            const auto size = input_ptr->tellg();
            auto speed = 1e6l * (size - size_offset);
            speed /= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_offset).count();

            if (verbose)
            {
                std::cerr << "Reading corpus: " << utils::memory_size(size);
                std::cerr << " (" << utils::memory_size(speed) << "/s, tuple count = " << frequency.size() << ")      \r" << std::flush;
            }

            size_offset = size;
            time_offset = std::chrono::high_resolution_clock::now();

            // Bottleneck
            frequency.record(temp_record);
            temp_record.clear();
        }
    }
}

void extract_tokens(const std::vector<std::string> &wordlist, std::vector<std::string> &single_tokens)
{
    std::unordered_set<std::string> token_set;
    std::string token;
    for (auto &word : wordlist)
    {
        std::stringstream ss(word);
        while (ss >> token)
        {
            token_set.insert(token);
        }
    }

    single_tokens.assign(token_set.begin(), token_set.end());
}

char default_wordlist_path[] = "data/wordlist.txt";
char default_corpus_path[] = "data/corpus.txt";
char default_frequency_path[] = "data/frequency.txt";
char default_input_path[] = "data/input.txt";

int main(int argc, char **argv)
{
    std::ios_base::sync_with_stdio(false);

    char *wordlist_path = default_wordlist_path,
         *corpus_path = default_corpus_path,
         *frequency_path = default_frequency_path,
         *input_path = default_input_path;
    bool verbose = false;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--wordlist") == 0)
        {
            if (++i < argc)
            {
                wordlist_path = argv[i];
            }
            else
            {
                throw std::out_of_range("Expected path to wordlist file after \"--wordlist\"");
            }
        }
        else if (std::strcmp(argv[i], "--corpus") == 0)
        {
            if (++i < argc)
            {
                corpus_path = argv[i];
            }
            else
            {
                throw std::out_of_range("Expected path to corpus file after \"--corpus\"");
            }
        }
        else if (std::strcmp(argv[i], "--frequency") == 0)
        {
            if (++i < argc)
            {
                frequency_path = argv[i];
            }
            else
            {
                throw std::out_of_range("Expected path to frequency file after \"--frequency\"");
            }
        }
        else if (std::strcmp(argv[i], "--input") == 0)
        {
            if (++i < argc)
            {
                input_path = argv[i];
            }
            else
            {
                throw std::out_of_range("Expected path to input file after \"--input\"");
            }
        }
        else if (std::strcmp(argv[i], "--verbose") == 0)
        {
            verbose = true;
        }
    }

    auto wordlist = import_wordlist(wordlist_path);
    std::cout << "Found " << wordlist.size() << " words in wordlist." << std::endl;

    for (auto &word : wordlist)
    {
        utils::to_lower(word);
    }

    std::unordered_set<std::string> wordlist_set(wordlist.begin(), wordlist.end());

    std::vector<std::string> single_tokens;
    extract_tokens(wordlist, single_tokens);
    std::cout << "Extracted " << single_tokens.size() << " single tokens." << std::endl;

    std::unordered_map<std::string, std::size_t> token_map;
    for (std::size_t i = 0; i < single_tokens.size(); i++)
    {
        token_map[single_tokens[i]] = i;
    }

    std::map<std::size_t, std::size_t> count;
    for (auto &word : wordlist)
    {
        auto syllables = std::count(word.begin(), word.end(), ' ') + 1;
        count[syllables]++;
    }
    std::cout << "Syllable count of wordlist: " << count << std::endl;

    std::unordered_map<uint64_t, unsigned int> frequency;
    std::fstream frequency_input(frequency_path, std::ios::in);
    if (frequency_input)
    {
        std::cout << "Importing data from \"" << frequency_path << "\"..." << std::endl;
        while (frequency_input)
        {
            bool valid = true;
            std::vector<std::string> tuple(3);
            for (std::size_t i = 0; i < 3; i++)
            {
                if (!(frequency_input >> tuple[i]) || token_map.find(tuple[i]) == token_map.end())
                {
                    valid = false;
                    break;
                }
            }

            if (valid)
            {
                std::size_t freq;
                if (frequency_input >> freq)
                {
                    int64_t mask = (token_map[tuple[0]] << 32) | (token_map[tuple[1]] << 16) | token_map[tuple[2]];
                    frequency[mask] = freq;
                }
            }
            else
            {
                break;
            }
        }

        frequency_input.close();
        std::cout << "Found " << frequency.size() << " tuples" << std::endl;
    }
    else
    {
        frequency_input.close();

        std::cout << "Frequency file \"" << frequency_path << "\" cannot be found. Learning from corpus..." << std::endl;
        tuple_frequency<FREQUENCY_RECORD_LIMIT> frequency_learned;
        learn(corpus_path, verbose, token_map, frequency_learned);

        std::cout << "\nSaving " << frequency_learned.size() << " tuples to \"" << frequency_path << "\"..." << std::endl;

        std::fstream frequency_output(frequency_path, std::ios::out);
        for (auto [mask, iterator] : frequency_learned)
        {
            std::size_t first = mask >> 32, second = ((mask & 0xFFFF0000) >> 16), third = (mask & 0xFFFF);
            frequency_output << single_tokens[first] << ' ' << single_tokens[second] << ' ' << single_tokens[third] << ' ' << iterator->first << '\n';
            frequency[mask] = iterator->first;
        }

        frequency_output.close();
    }

    // std::cout << "Building BK-tree from wordlist..." << std::endl;
    // BKTree bk_tree(wordlist);

    std::cout << "Reading \"" << input_path << "\"..." << std::endl;
    std::fstream input(input_path, std::ios::in);

    std::vector<std::string> tokens;
    while (read_single_sentence<true>(input, tokens))
    {
        // `tokens` also includes non-alphabet characters
        for (auto &token : tokens)
        {
            utils::to_lower(token);
        }

        std::vector<std::string *> normalized_tokens_ptr;
        for (auto &token : tokens)
        {
            auto *ptr = new std::string();
            remove_non_alphabet_characters(token, *ptr);
            if (ptr->empty())
            {
                normalized_tokens_ptr.push_back(nullptr);
                delete ptr;
            }
            else
            {
                normalized_tokens_ptr.push_back(ptr);
            }
        }

        // std::cerr << "normalized_tokens_ptr = " << normalized_tokens_ptr << std::endl;

        std::vector<std::string *> normalized_tokens_ptr_not_null;
        for (auto &ptr : normalized_tokens_ptr)
        {
            if (ptr != nullptr)
            {
                normalized_tokens_ptr_not_null.push_back(ptr);
            }
        }

        // std::cerr << "normalized_tokens_ptr_not_null = " << normalized_tokens_ptr_not_null << std::endl;

        std::vector<std::string> normalized_tokens;
        for (auto &ptr : normalized_tokens_ptr_not_null)
        {
            normalized_tokens.emplace_back(*ptr);
        }

        std::vector<std::vector<std::size_t>> words;
        combine_tokens(normalized_tokens, wordlist_set, words);
        // std::cerr << "words = " << words << std::endl;

        std::vector<bool> need_fix(normalized_tokens.size());
        for (auto &indices : words)
        {
            if (indices.size() == 1)
            {
                need_fix[indices[0]] = true;
            }
        }

        for (std::size_t i = 0; i < normalized_tokens.size(); i++)
        {
            if (need_fix[i])
            {
                std::unordered_map<uint64_t, unsigned int> scores;
                if (i > 0)
                {
                    auto prev = token_map.find(normalized_tokens[i - 1]);
                    if (prev != token_map.end())
                    {
                        auto prev_index = prev->second;
                        for (auto &[mask, freq] : frequency)
                        {
                            if (((mask >> 16) & 0xFFFF) == prev_index)
                            {
                                scores[mask & 0xFFFF]++;
                            }
                            else if (((mask >> 32) & 0xFFFF) == prev_index)
                            {
                                scores[(mask >> 16) & 0xFFFF]++;
                            }
                        }
                    }
                }
                if (i + 1 < normalized_tokens.size())
                {
                    auto next = token_map.find(normalized_tokens[i + 1]);
                    if (next != token_map.end())
                    {
                        auto next_index = next->second;
                        for (auto &[mask, freq] : frequency)
                        {
                            if ((mask & 0xFFFF) == next_index)
                            {
                                scores[(mask >> 16) & 0xFFFF]++;
                            }
                            else if (((mask >> 16) & 0xFFFF) == next_index)
                            {
                                scores[(mask >> 32) & 0xFFFF]++;
                            }
                        }
                    }
                }

                std::vector<std::pair<uint64_t, unsigned int>> candidates(scores.begin(), scores.end());
                std::sort(
                    candidates.begin(), candidates.end(),
                    [](const auto &a, const auto &b)
                    {
                        return a.second > b.second;
                    });

                std::vector<std::string> candidates_str;
                for (std::size_t i = 0; i < std::min<std::size_t>(1000, candidates.size()); i++)
                {
                    candidates_str.push_back(single_tokens[candidates[i].first]);
                }

                // std::cerr << "\"" << normalized_tokens[i] << "\":\ncandidates_str = " << candidates_str << std::endl;
                // std::cerr << "scores = " << candidates << std::endl;

                std::vector<std::size_t> distances;
                for (auto &candidate : candidates_str)
                {
                    distances.push_back(damerau_levenshtein(normalized_tokens[i], candidate));
                }

                auto result_index = std::distance(distances.begin(), std::min_element(distances.begin(), distances.end()));
                normalized_tokens[i] = *normalized_tokens_ptr_not_null[i] = candidates_str[result_index];
            }
        }

        // Print results
        for (std::size_t i = 0; i < tokens.size(); i++)
        {
            if (normalized_tokens_ptr[i] != nullptr)
            {
                std::cout << *normalized_tokens_ptr[i];
            }
            else
            {
                std::cout << tokens[i];
            }

            std::cout << (i + 1 == tokens.size() ? ". " : " ");
        }

        for (auto &ptr : normalized_tokens_ptr_not_null)
        {
            delete ptr;
        }
    }
    std::cout << std::endl;

    input.close();

    return 0;
}
