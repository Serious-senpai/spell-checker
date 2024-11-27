#include <standard.hpp>

std::vector<std::vector<std::string>> read(const std::string &wordlist_path)
{
    std::fstream input(wordlist_path, std::ios::in);
    std::vector<std::string> wordlist;
    std::string word;
    while (input >> word)
    {
        std::replace(word.begin(), word.end(), '_', ' ');
        wordlist.push_back(word);
    }

    std::vector<std::vector<std::string>> words_by_syllable; // starting from 0 (1 syllable)
    for (const auto &word : wordlist)
    {
        auto syllable = std::count(word.begin(), word.end(), ' ');
        words_by_syllable.resize(syllable + 1);
        words_by_syllable[syllable].push_back(word);
    }

    return words_by_syllable;
}

int main(int argc, char **argv)
{
    std::string wordlist = "data/wordlist.txt", corpus = "data/corpus.txt";
    for (int i = 1; i < argc; i++)
    {
        std::string arg(argv[i]);
        if (arg == "--wordlist")
        {
            if (i + 1 < argc)
            {
                wordlist = argv[i + 1];
            }
            else
            {
                throw std::out_of_range("Expected path to wordlist file after \"--wordlist\"");
            }
        }
        else if (arg == "--corpus")
        {
            if (i + 1 < argc)
            {
                corpus = argv[i + 1];
            }
            else
            {
                throw std::out_of_range("Expected path to corpus file after \"--corpus\"");
            }
        }
    }

    const auto words_by_syllable = read(wordlist);
    return 0;
}
