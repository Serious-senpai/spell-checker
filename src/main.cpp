#include <standard.hpp>

std::vector<std::vector<std::string>> read()
{
    std::fstream input("wordlist.txt", std::ios::in);
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

int main()
{
    const auto words_by_syllable = read();
    return 0;
}
