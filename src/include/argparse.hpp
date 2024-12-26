#pragma once

#include "utils.hpp"

class Namespace
{
private:
    static char _default_wordlist_path[];
    static char _default_corpus_path[];
    static char _default_frequency_path[];
    static char _default_input_path[];
    static char _default_output_path[];

public:
    char *wordlist_path = _default_wordlist_path,
         *corpus_path = _default_corpus_path,
         *frequency_path = _default_frequency_path,
         *input_path = _default_input_path,
         *output_path = _default_output_path;

    bool verbose = false;
    bool interactive = false;

    Namespace(int argc, char **argv)
    {
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
            else if (std::strcmp(argv[i], "--output") == 0)
            {
                if (++i < argc)
                {
                    output_path = argv[i];
                }
                else
                {
                    throw std::out_of_range("Expected path to output file after \"--output\"");
                }
            }
            else if (std::strcmp(argv[i], "-v") == 0)
            {
                verbose = true;
            }
            else if (std::strcmp(argv[i], "--interactive") == 0)
            {
                interactive = true;
            }
            else
            {
                throw std::invalid_argument(utils::format("Unrecognized argument \"%s\"", argv[i]));
            }
        }
    }
};

char Namespace::_default_wordlist_path[] = "data/wordlist.txt";
char Namespace::_default_corpus_path[] = "data/corpus.txt";
char Namespace::_default_frequency_path[] = "data/frequency.txt";
char Namespace::_default_input_path[] = "data/input.txt";
char Namespace::_default_output_path[] = "data/output.txt";
