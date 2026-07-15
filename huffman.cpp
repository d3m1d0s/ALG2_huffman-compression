#include "huffman.h"

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

namespace huffman {

HuffmanTree::HuffmanTree(const unordered_map<char, unsigned>& frequencies)
{
    struct CompareNodes {
        bool operator()(const Node* left, const Node* right) const {
            return left->frequency > right->frequency;
        }
    };

    priority_queue<Node*, vector<Node*>, CompareNodes> nodes;
    for (const auto& pair : frequencies)
        nodes.push(new Node{pair.first, pair.second, nullptr, nullptr});

    // The two rarest nodes merge first, so frequent symbols end up near the root.
    while (nodes.size() > 1) {
        Node* left = nodes.top();
        nodes.pop();
        Node* right = nodes.top();
        nodes.pop();
        nodes.push(new Node{'$', left->frequency + right->frequency, left, right});
    }

    if (!nodes.empty())
        root = nodes.top();
}

HuffmanTree::~HuffmanTree()
{
    FreeNodes(root);
}

void HuffmanTree::FreeNodes(Node* node)
{
    if (node == nullptr)
        return;
    FreeNodes(node->left);
    FreeNodes(node->right);
    delete node;
}

unordered_map<char, string> HuffmanTree::BuildCodeTable() const
{
    unordered_map<char, string> codes;
    CollectCodes(root, "", codes);
    return codes;
}

void HuffmanTree::CollectCodes(const Node* node, const string& path,
                               unordered_map<char, string>& codes)
{
    if (node == nullptr)
        return;

    if (!node->left && !node->right)
        // A one-symbol alphabet leaves the root a leaf with an empty path.
        codes[node->character] = path.empty() ? "0" : path;

    CollectCodes(node->left, path + "0", codes);
    CollectCodes(node->right, path + "1", codes);
}

namespace {

bool ReadFile(const string& fileName, string& content)
{
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: cannot open the input file: " << fileName << endl;
        return false;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}

bool WriteCodeTable(const unordered_map<char, string>& codes, const string& tableFileName)
{
    ofstream codeFile(tableFileName, ios::binary);
    if (!codeFile.is_open()) {
        cerr << "Error: cannot create the code table: " << tableFileName << endl;
        return false;
    }
    // Numeric byte values let any symbol, including ':', survive the text format.
    for (const auto& pair : codes)
        codeFile << static_cast<int>(static_cast<unsigned char>(pair.first)) << ":" << pair.second << "\n";
    return true;
}

bool ReadCodeTable(const string& tableFileName, unordered_map<string, char>& codes)
{
    ifstream codeFile(tableFileName, ios::binary);
    if (!codeFile.is_open()) {
        cerr << "Error: cannot open the code table: " << tableFileName << endl;
        return false;
    }

    string line;
    while (getline(codeFile, line)) {
        // Tolerate tables that were written with Windows line endings.
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        // Line format: <byte value 0..255>:<code of '0'/'1'>; anything else is skipped.
        size_t separator = line.find(':');
        if (separator == string::npos || separator == 0 || separator > 3)
            continue;

        string valuePart = line.substr(0, separator);
        string code = line.substr(separator + 1);
        if (valuePart.find_first_not_of("0123456789") != string::npos)
            continue;
        if (code.empty() || code.find_first_not_of("01") != string::npos)
            continue;

        int value = stoi(valuePart);
        if (value > 255)
            continue;

        codes[code] = static_cast<char>(value);
    }
    return true;
}

bool WriteEncodedString(const string& encodedString, const string& outputFileName)
{
    ofstream outputFile(outputFileName, ios::binary);
    if (!outputFile.is_open()) {
        cerr << "Error: cannot create the output file: " << outputFileName << endl;
        return false;
    }

    // The first byte stores how many filler zero bits complete the last data byte (0..7).
    unsigned char padding = (8 - encodedString.size() % 8) % 8;
    outputFile.put(static_cast<char>(padding));

    for (size_t i = 0; i < encodedString.size(); i += 8) {
        string byteBits = encodedString.substr(i, 8);
        byteBits.append(8 - byteBits.size(), '0');
        outputFile.put(static_cast<char>(bitset<8>(byteBits).to_ulong()));
    }
    return true;
}

void DecodeBits(const string& bits, ofstream& outputFile, const unordered_map<string, char>& codes)
{
    string current;
    for (char bit : bits) {
        current += bit;
        auto match = codes.find(current);
        if (match != codes.end()) {
            outputFile << match->second;
            current.clear();
        }
    }
}

void DecodeStream(const string& encodedFileName, ofstream& outputFile, const unordered_map<string, char>& codes)
{
    ifstream inputFile(encodedFileName, ios::binary);
    stringstream sstream;
    sstream << inputFile.rdbuf();
    string bytes = sstream.str();

    if (bytes.empty())
        return;

    // The first byte is the number of filler bits to drop from the tail.
    unsigned char padding = static_cast<unsigned char>(bytes[0]);

    string bits;
    for (size_t i = 1; i < bytes.size(); ++i)
        bits += bitset<8>(static_cast<unsigned char>(bytes[i])).to_string();
    if (padding <= bits.size())
        bits.resize(bits.size() - padding);

    DecodeBits(bits, outputFile, codes);
}

} // namespace

bool Compress(const string& inputFileName, const string& outputFileName)
{
    string text;
    if (!ReadFile(inputFileName, text))
        return false;

    unordered_map<char, unsigned> frequencies;
    for (char ch : text)
        frequencies[ch]++;

    // No alphabet to build a tree from; write an empty table and a header-only stream.
    if (frequencies.empty()) {
        if (!WriteCodeTable({}, outputFileName + ".huff"))
            return false;
        return WriteEncodedString("", outputFileName);
    }

    HuffmanTree tree(frequencies);
    unordered_map<char, string> codes = tree.BuildCodeTable();

    if (!WriteCodeTable(codes, outputFileName + ".huff"))
        return false;

    string encodedString;
    for (char ch : text)
        encodedString += codes[ch];

    return WriteEncodedString(encodedString, outputFileName);
}

bool Decompress(const string& inputFileName, const string& outputFileName)
{
    string tableFileName = inputFileName + ".huff";

    unordered_map<string, char> codes;
    if (!ReadCodeTable(tableFileName, codes))
        return false;

    if (!fs::exists(inputFileName)) {
        cerr << "Error: cannot open the compressed file: " << inputFileName << endl;
        return false;
    }
    // An empty table is only valid for the one-byte stream of an empty file.
    if (codes.empty() && fs::file_size(inputFileName) > 1) {
        cerr << "Error: no valid entries in the code table: " << tableFileName << endl;
        return false;
    }

    ofstream outputFile(outputFileName, ios::binary);
    if (!outputFile.is_open()) {
        cerr << "Error: cannot create the output file: " << outputFileName << endl;
        return false;
    }
    DecodeStream(inputFileName, outputFile, codes);
    return true;
}

} // namespace huffman
