#include <config.hpp>
#include <distance.hpp>

void learn(
    const char *corpus_path,
    const std::unordered_map<std::string, std::size_t> &token_map,
    tuple_frequency<FREQUENCY_RECORD_LIMIT> &frequency)
{
    std::fstream input(corpus_path, std::ios::in);

    std::streampos size_offset = 0;
    auto time_offset = std::chrono::high_resolution_clock::now();

    std::vector<std::string> sentence;
    std::unordered_map<uint64_t, unsigned int> temp_record;
    for (std::size_t counter = 0; read_corpus_sentence(input, sentence); counter++)
    {
        for (auto &word : sentence)
        {
            to_lower(word);
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
            const auto size = input.tellg();
            auto speed = 1e6l * (size - size_offset);
            speed /= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_offset).count();
            std::cerr << "Reading corpus: " << memory_size(size);
            std::cerr << " (" << memory_size(speed) << "/s, tuple count = " << frequency.size() << ")      \r" << std::flush;

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

int main(int argc, char **argv)
{
    EnableUTF8Console _lifespan;
    std::ios_base::sync_with_stdio(false);

    char *wordlist_path = default_wordlist_path, *corpus_path = default_corpus_path, *frequency_path = default_frequency_path;
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
    }

    auto wordlist = import_wordlist(wordlist_path);
    std::cout << "Found " << wordlist.size() << " words in wordlist." << std::endl;

    for (auto &word : wordlist)
    {
        to_lower(word);
    }

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
        learn(corpus_path, token_map, frequency_learned);

        std::cout << "Saving " << frequency_learned.size() << " tuples to \"" << frequency_path << "\"..." << std::endl;

        std::fstream frequency_output(frequency_path, std::ios::out);
        for (auto [mask, iterator] : frequency_learned)
        {
            std::size_t first = mask >> 32, second = ((mask & 0xFFFF0000) >> 16), third = (mask & 0xFFFF);
            frequency_output << single_tokens[first] << ' ' << single_tokens[second] << ' ' << single_tokens[third] << ' ' << iterator->first << '\n';
            frequency[mask] = iterator->first;
        }

        frequency_output.close();
    }

    return 0;
}
