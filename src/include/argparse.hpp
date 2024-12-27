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
    std::size_t max_candidates_per_token = 1000;
    std::size_t edit_distance_threshold = 2;
    double edit_penalty_factor = 0.01;

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
            else if (std::strcmp(argv[i], "--max-candidates-per-token") == 0)
            {
                if (++i < argc)
                {
                    max_candidates_per_token = std::stoul(argv[i]);
                }
                else
                {
                    throw std::out_of_range("Expected integer after \"--max-candidates-per-token\"");
                }
            }
            else if (std::strcmp(argv[i], "--edit-distance-threshold") == 0)
            {
                if (++i < argc)
                {
                    edit_distance_threshold = std::stoul(argv[i]);
                }
                else
                {
                    throw std::out_of_range("Expected integer after \"--edit-distance-threshold\"");
                }
            }
            else if (std::strcmp(argv[i], "--edit-penalty-factor") == 0)
            {
                if (++i < argc)
                {
                    edit_penalty_factor = std::atof(argv[i]);
                }
                else
                {
                    throw std::out_of_range("Expected float after \"--edit-penalty-factor\"");
                }
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

namespace std
{
    template <typename CharT>
    basic_ostream<CharT> &operator<<(basic_ostream<CharT> &stream, const Namespace &argparse)
    {
        stream << "Namespace(";
        stream << "wordlist_path=\"" << argparse.wordlist_path << "\", ";
        stream << "corpus_path=\"" << argparse.corpus_path << "\", ";
        stream << "frequency_path=\"" << argparse.frequency_path << "\", ";
        stream << "input_path=\"" << argparse.input_path << "\", ";
        stream << "output_path=\"" << argparse.output_path << "\", ";
        stream << "verbose=" << argparse.verbose << ", ";
        stream << "interactive=" << argparse.interactive << ", ";
        stream << "max_candidates_per_token=" << argparse.max_candidates_per_token << ", ";
        stream << "edit_distance_threshold=" << argparse.edit_distance_threshold << ", ";
        stream << "edit_penalty_factor=" << argparse.edit_penalty_factor;
        stream << ")";

        return stream;
    }

}
