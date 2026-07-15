#pragma once

#include <string>
#include <unordered_map>

namespace huffman {

/// Owns the tree built from symbol frequencies and releases it on destruction.
class HuffmanTree {
public:
    explicit HuffmanTree(const std::unordered_map<char, unsigned>& frequencies);
    ~HuffmanTree();

    HuffmanTree(const HuffmanTree&) = delete;
    HuffmanTree& operator=(const HuffmanTree&) = delete;

    /// Builds the code table: symbol -> its path in the tree as a '0'/'1' string.
    std::unordered_map<char, std::string> BuildCodeTable() const;

private:
    struct Node {
        char character;
        unsigned frequency;
        Node* left;
        Node* right;
    };

    static void CollectCodes(const Node* node, const std::string& path,
                             std::unordered_map<char, std::string>& codes);
    static void FreeNodes(Node* node);

    Node* root = nullptr;
};

/// Compresses the input file into outputFileName and writes the code table
/// next to it as outputFileName + ".huff". Returns false on a file error.
bool Compress(const std::string& inputFileName, const std::string& outputFileName);

/// Restores the original file from a compressed file and its .huff table.
/// Returns false on a file error or an unusable table.
bool Decompress(const std::string& inputFileName, const std::string& outputFileName);

} // namespace huffman
