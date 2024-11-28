#include <config.hpp>
#include <distance.hpp>

int main(int argc, char **argv)
{
    EnableUTF8Console _lifespan;
    std::ios_base::sync_with_stdio(false);

    char default_wordlist_path[] = "data/wordlist.txt", default_corpus_path[] = "data/corpus.txt";
    char *wordlist_path = default_wordlist_path, *corpus_path = default_corpus_path;
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
    }

    const auto wordlist = import_wordlist(wordlist_path);
    int max_tokens_per_word = 0;
    for (auto &word : wordlist)
    {
        max_tokens_per_word = std::max<int>(max_tokens_per_word, std::count(word.begin(), word.end(), ' ') + 1);
    }

    const auto variants = delete_variants(wordlist);

    std::string input;
    std::cout << "Enter a Vietnamese string>" << std::flush;
    std::getline(std::cin, input);

    std::cout << "Received \"" << input << "\"" << std::endl;
    for (auto &c : input)
    {
        std::cout << std::hex << short(c) << " ";
    }
    std::cout << std::endl;

    return 0;
}
