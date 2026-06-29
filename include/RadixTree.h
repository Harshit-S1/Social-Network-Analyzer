#ifndef RADIX_TREE_H
#define RADIX_TREE_H

#include <map>
#include <string>
#include <vector>
#include <cctype>
using namespace std;

// node in our Radix Tree 
// instead of storing just 1 character like a standard trie, we store chunks of words (edgeLabels) to save memory
class RadixNode {
public:
    map<char, RadixNode*> children;
    string edgeLabel;
    bool isEndOfWord;
    string fullWord; // keeping the original casing for display purposes
    int userId;

    RadixNode(const string& label = "") 
        : edgeLabel(label), isEndOfWord(false), userId(-1) {}
};

// gives our ultra-fast search/autocomplete box
class RadixTree {
private:
    RadixNode* root;

    // recursively destroy the tree from memory so we don't leak
    void destroy(RadixNode* node) {
        if (!node) return;
        for (auto& pair : node->children) {
            destroy(pair.second);
        }
        delete node;
    }

    // go down the branches to collect autocomplete suggestions
    void dfs(RadixNode* node, vector<string>& results, int limit) const {
        if (!node || results.size() >= (size_t)limit) return;
        
        // if this node completes a valid username, add it to our results
        if (node->isEndOfWord) {
            results.push_back(node->fullWord);
        }
        
        // keep going deeper
        for (auto& pair : node->children) {
            dfs(pair.second, results, limit);
        }
    }

public:
    RadixTree() { root = new RadixNode(); }
    ~RadixTree() { destroy(root); }
    RadixTree(const RadixTree&) = delete;
    RadixTree& operator=(const RadixTree&) = delete;

    // adding a new username into the search index
    void insert(const string& word, int userId) {
        // forcing everything to lowercase for case-insensitive searching
        string lowerWord = "";
        for (char ch : word) lowerWord += tolower(static_cast<unsigned char>(ch));

        RadixNode* current = root;
        int i = 0;

        while (i < lowerWord.length()) {
            char firstChar = lowerWord[i];
            
            // Case 1: dead end, have to create a new branch for the rest of the word
            if (current->children.find(firstChar) == current->children.end()) {
                RadixNode* newNode = new RadixNode(lowerWord.substr(i));
                newNode->isEndOfWord = true;
                newNode->fullWord = word;
                newNode->userId = userId;
                current->children[firstChar] = newNode;
                return;
            }

            RadixNode* child = current->children[firstChar];
            string label = child->edgeLabel;
            int j = 0;
            
            // finding out how much of the current word matches the existing edge label
            while (j < label.length() && i + j < lowerWord.length() && label[j] == lowerWord[i + j]) {
                j++;
            }

            // Case 2: paths diverge, we have to split the existing edge into two
            if (j < label.length()) {
                RadixNode* splitNode = new RadixNode(label.substr(j));
                splitNode->children = child->children;
                splitNode->isEndOfWord = child->isEndOfWord;
                splitNode->fullWord = child->fullWord;
                splitNode->userId = child->userId;

                // updating the current node to end at the split point
                child->edgeLabel = label.substr(0, j);
                child->children.clear();
                child->children[label[j]] = splitNode;
                child->isEndOfWord = false;

                // if the new word ends exactly at the split point
                if (i + j == lowerWord.length()) {
                    child->isEndOfWord = true;
                    child->fullWord = word;
                    child->userId = userId;
                    return;
                } else {
                    // otherwise, branch off with the rest of the new word
                    RadixNode* newNode = new RadixNode(lowerWord.substr(i + j));
                    newNode->isEndOfWord = true;
                    newNode->fullWord = word;
                    newNode->userId = userId;
                    child->children[lowerWord[i + j]] = newNode;
                    return;
                }
            } else {
                // Case 3: the whole edge matched perfectly so jump to the next node and keep going
                i += j;
                if (i == lowerWord.length()) {
                    if (child->isEndOfWord) return; // prevents duplicate work if they already exist
                    child->isEndOfWord = true;
                    child->fullWord = word;
                    child->userId = userId;
                    return;
                }
                current = child;
            }
        }
    }

    // finding all users that start with the given letters (e.g. typing "ali" returns "Alice", "Alicia")
    vector<string> searchPrefix(const string& prefix, int limit = 5) const {
        vector<string> results;
        if (prefix.empty()) return results;
        
        string lowerPref = "";
        for (char ch : prefix) lowerPref += tolower(static_cast<unsigned char>(ch));

        RadixNode* current = root;
        int i = 0;

        // walking down the tree following the characters of the search prefix
        while (i < lowerPref.length()) {
            char firstChar = lowerPref[i];
            
            // if the path just drops off, there are no matches
            if (current->children.find(firstChar) == current->children.end()) {
                return results; 
            }

            RadixNode* child = current->children[firstChar];
            string label = child->edgeLabel;
            int j = 0;

            // matching the search prefix against this part of the tree
            while (j < label.length() && i + j < lowerPref.length() && label[j] == lowerPref[i + j]) {
                j++;
            }

            // we found the node where the search prefix ends
            // now just take all the full words attached below it
            if (i + j == lowerPref.length()) {
                dfs(child, results, limit);
                return results;
            }

            // the prefix diverged from the tree path so no matches
            if (j < label.length()) return results; 
            
            i += j;
            current = child;
        }
        return results;
    }
};

#endif 