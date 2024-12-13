#include "distance.hpp"

class _BKTreeNode
{
public:
    std::string word;
    std::map<std::size_t, _BKTreeNode *> children;

    _BKTreeNode(const std::string &w) : word(w) {}
    ~_BKTreeNode()
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
    _BKTreeNode *_root = nullptr;

public:
    BKTree(const std::vector<std::string> &wordlist)
    {
        for (const std::string &word : wordlist)
        {
            add(word);
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
            _root = new _BKTreeNode(word);
            return;
        }

        _BKTreeNode *current = _root;
        while (true)
        {
            std::size_t distance = damerau_levenshtein(current->word, word);

            if (current->children.find(distance) == current->children.end())
            {
                current->children[distance] = new _BKTreeNode(word);
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
            std::vector<std::pair<_BKTreeNode *, bool>> stack = {std::make_pair(_root, false)};
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
                        stack.push_back(std::make_pair(child, prune));
                    }
                }
            }
        }

        return results;
    }
};
