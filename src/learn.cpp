#include <data.hpp>
#include <distance.hpp>
#include <utils.hpp>

class Namespace
{
private:
    static char _default_corpus_path[];
    static char _default_frequency_path[];

public:
    char *corpus_path = _default_corpus_path,
         *frequency_path = _default_frequency_path;

    bool verbose = false;

    Namespace(int argc, char **argv)
    {
        for (int i = 1; i < argc; i++)
        {
            if (std::strcmp(argv[i], "--corpus") == 0)
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
            else if (std::strcmp(argv[i], "-v") == 0)
            {
                verbose = true;
            }
            else
            {
                throw std::invalid_argument(utils::format("Unrecognized argument \"%s\"", argv[i]));
            }
        }
    }
};

char Namespace::_default_corpus_path[] = "data/corpus.txt";
char Namespace::_default_frequency_path[] = "data/frequency.txt";

namespace std
{
    template <typename CharT>
    basic_ostream<CharT> &operator<<(basic_ostream<CharT> &stream, const Namespace &argparse)
    {
        stream << "Namespace(";
        stream << "corpus_path=\"" << argparse.corpus_path << "\", ";
        stream << "frequency_path=\"" << argparse.frequency_path << "\", ";
        stream << "verbose=" << argparse.verbose << ")";

        return stream;
    }
}

int main(int argc, char **argv)
{
    std::ios_base::sync_with_stdio(false);

    Namespace argparse(argc, argv);
    std::cout << "Command line arguments: " << argparse << std::endl;

    std::unordered_map<std::string, uint32_t> token_map;
    token_map.reserve(1 << 24);

    std::unordered_map<uint64_t, unsigned int> frequency;
    frequency.reserve(1 << 24);

    std::istream *input_ptr;
    std::fstream file_input;
    if (std::strcmp(argparse.corpus_path, "-") == 0)
    {
        input_ptr = &std::cin;
    }
    else
    {
        file_input.open(argparse.corpus_path, std::ios::in);
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

        if (argparse.verbose && !(++counter & 0xFFFFF))
        {
            const auto size = input_ptr->tellg();
            auto speed = 1e6l * size;
            speed /= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_offset).count();

            std::cout << "Reading corpus: " << utils::memory_size(size);
            std::cout << " (" << utils::memory_size(speed) << "/s, tuple count = " << frequency.size() << ")      \r" << std::flush;
        }
    }
    std::erase_if(
        frequency,
        [](const std::pair<uint64_t, unsigned int> &p)
        { return p.second < 4; });

    std::cout << "\nSaving " << frequency.size() << " tuples to \"" << argparse.frequency_path << "\"..." << std::endl;

    std::vector<std::string> reversed_token_map;
    index_tokens(token_map, reversed_token_map);

    std::fstream frequency_output(argparse.frequency_path, std::ios::out);
    for (auto &[mask, freq] : frequency)
    {
        auto first = mask >> 32, second = mask & 0xFFFFFFFF;
        frequency_output << reversed_token_map[first] << ' ' << reversed_token_map[second] << ' ' << freq << '\n';
    }

    frequency_output.close();

    auto iter = std::max_element(
        frequency.begin(), frequency.end(),
        [](const auto &lhs, const auto &rhs)
        { return lhs.second < rhs.second; });

    std::cout << "Most frequent tuple: \"" << reversed_token_map[iter->first >> 32] << ' ' << reversed_token_map[iter->first & 0xFFFFFFFF] << "\" with a count of " << iter->second << std::endl;

    return 0;
}
