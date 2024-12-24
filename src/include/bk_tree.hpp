#pragma once

#include "distance.hpp"
#include "utils.hpp"

class __BKTreeNode
{
public:
    std::string word;
    std::map<std::size_t, __BKTreeNode *> children;

    __BKTreeNode(const std::string &w) : word(w) {}
    ~__BKTreeNode()
    {
        for (auto &[_, child] : children)
        {
            delete child;
        }
    }
};

class BKTree
{
private:
    __BKTreeNode *_root = nullptr;

public:
    template <typename _InputIter, utils::is_input_iterator_t<_InputIter> = true>
    BKTree(_InputIter __first, _InputIter __last)
    {
        for (; __first != __last; __first++)
        {
            add(*__first);
        }
    }

    ~BKTree()
    {
        delete _root;
    }

    void add(const std::string &word)
    {
        if (_root == nullptr)
        {
            _root = new __BKTreeNode(word);
            return;
        }

        __BKTreeNode *current = _root;
        while (true)
        {
            std::size_t distance = damerau_levenshtein(current->word, word);

            if (current->children.find(distance) == current->children.end())
            {
                current->children[distance] = new __BKTreeNode(word);
                break;
            }

            current = current->children[distance];
        }
    }

    std::vector<std::string> search(const std::string &query, std::size_t max_distance) const
    {
        // std::cerr << "query = \"" << query << "\"" << std::endl;
        std::vector<std::string> results;
        if (_root != nullptr)
        {
            std::vector<std::pair<__BKTreeNode *, bool>> stack = {std::make_pair(_root, false)};
            while (!stack.empty())
            {
                auto [ptr, prune] = stack.back();
                stack.pop_back();

                auto d = damerau_levenshtein(query, ptr->word);
                // std::cerr << "Searching at \"" << ptr->word << "\" with distance = " << d << ", prune = " << prune << std::endl;
                if (d <= max_distance)
                {
                    results.push_back(ptr->word);
                    prune = true;
                }

                for (auto &[d_child, child] : ptr->children)
                {
                    auto diff = d_child > d ? d_child - d : d - d_child;
                    if (diff < max_distance || !prune)
                    {
                        stack.emplace_back(child, prune);
                    }
                }
            }
        }

        return results;
    }
};
