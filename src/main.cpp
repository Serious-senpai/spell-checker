#include <bk_tree.hpp>
#include <config.hpp>
#include <distance.hpp>

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

            std::cerr << "Reading corpus: " << utils::memory_size(size);
            std::cerr << " (" << utils::memory_size(speed) << "/s, tuple count = " << frequency.size() << ")      \r" << std::flush;
        }
    }
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

    std::unordered_map<std::string, uint32_t> token_map;
    std::vector<std::string> reversed_token_map;
    std::unordered_map<uint64_t, unsigned int> frequency;
    std::fstream frequency_input(frequency_path, std::ios::in);
    if (frequency_input)
    {
        std::cout << "Importing data from \"" << frequency_path << "\"..." << std::endl;

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

        reversed_token_map.resize(token_map.size());
        for (auto &[token, index] : token_map)
        {
            reversed_token_map[index] = token;
        }
        std::cout << "Found " << frequency.size() << " tuples." << std::endl;
    }
    else
    {
        frequency_input.close();
        std::cout << "Frequency file \"" << frequency_path << "\" cannot be found. Learning from corpus..." << std::endl;

        token_map.reserve(1 << 24);
        frequency.reserve(1 << 24);
        learn(corpus_path, verbose, token_map, frequency);

        std::cout << "\nSaving " << frequency.size() << " tuples to \"" << frequency_path << "\"..." << std::endl;

        reversed_token_map.resize(token_map.size());
        for (auto &[token, index] : token_map)
        {
            reversed_token_map[index] = token;
        }

        std::erase_if(
            frequency,
            [](const std::pair<uint64_t, unsigned int> &p)
            { return p.second < 2; });

        std::fstream frequency_output(frequency_path, std::ios::out);
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

    std::cout << "Reading \"" << input_path << "\"..." << std::endl;
    std::fstream input(input_path, std::ios::in);

    input.close();

    return 0;
}
