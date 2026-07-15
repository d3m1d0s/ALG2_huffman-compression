#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <cstring>
#include <sstream>
#include <bitset>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

/// @struct TreeNode
/// @brief A node structure for Huffman tree.
///
/// This structure represents a node in the Huffman tree,
/// containing a character, its frequency, and pointers to left and right child nodes.
struct TreeNode
{
    char character; ///< Character data of the node.
    unsigned frequency; ///< Frequency of the character.
    TreeNode* left, * right; ///< Pointers to the left and right child nodes.

    TreeNode(char character, unsigned frequency)
        : character(character), frequency(frequency), left(nullptr), right(nullptr)
    {} ///< Initializes a new node with given character and frequency, left and right pointers are set to nullptr.
};

/// @struct CompareNodes
/// @brief A functor for priority queue in Huffman tree construction.
///
/// This functor provides a comparison operation for TreeNode pointers,
/// facilitating the construction of a min heap based on frequency.
struct CompareNodes
{
    bool operator()(TreeNode* left, TreeNode* right)
    {
        return (left->frequency > right->frequency); ///< Defines comparison operation for two TreeNode pointers, used in priority queue.
    }
};

/// @brief Generates Huffman codes for characters in the tree.
/// @param root Pointer to the root of the Huffman tree.
/// @param str A string representing the path to the current node.
/// @param huffmanCodes A map to store characters and their corresponding Huffman codes.
void GenerateHuffmanCodes(TreeNode* root, const string& str, unordered_map<char, string>& huffmanCodes)
{
    if (root == nullptr)
        return; ///< Base case: if the root is null, do nothing.

    if (!root->left && !root->right)
        // A one-symbol alphabet leaves the root a leaf with an empty path.
        huffmanCodes[root->character] = str.empty() ? "0" : str;

    GenerateHuffmanCodes(root->left, str + "0", huffmanCodes); ///< Recursively traverse the left child, appending "0" to the code string.
    GenerateHuffmanCodes(root->right, str + "1", huffmanCodes); ///< Recursively traverse the right child, appending "1" to the code string.
}

/// @brief Frees all nodes of a Huffman tree.
/// @param root Pointer to the root of the tree.
void FreeHuffmanTree(TreeNode* root)
{
    if (root == nullptr)
        return;
    FreeHuffmanTree(root->left);
    FreeHuffmanTree(root->right);
    delete root;
}

/// @brief Writes the encoded string to a binary file.
/// @param encodedString The string of encoded data.
/// @param outputFileName The name of the file to write the encoded data to.
/// @returns True on success, false if the output file cannot be created.
bool WriteEncodedStringToFile(const string& encodedString, const string& outputFileName) {
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
    outputFile.close();
    return true;
}

/// @brief Builds the Huffman tree from the input text.
/// @param inputText The text to be encoded.
/// @param outputFileName The name of the file to store the Huffman codes.
/// @returns True on success, false if an output file cannot be created.
bool BuildHuffmanTree(const string& inputText, const string& outputFileName)
{
    unordered_map<char, unsigned> charFrequencyMap;
    for (char ch : inputText)
        charFrequencyMap[ch]++;

    // No alphabet to build a tree from; write an empty table and a header-only stream.
    if (charFrequencyMap.empty()) {
        ofstream codeFile(outputFileName + ".huff", ios::binary);
        if (!codeFile.is_open()) {
            cerr << "Error: cannot create the code table: " << outputFileName << ".huff" << endl;
            return false;
        }
        return WriteEncodedStringToFile("", outputFileName);
    }

    priority_queue<TreeNode*, vector<TreeNode*>, CompareNodes> priorityQueue;
    for (const auto& pair : charFrequencyMap)
        priorityQueue.push(new TreeNode(pair.first, pair.second));

    while (priorityQueue.size() != 1)
    {
        TreeNode* leftNode = priorityQueue.top(); priorityQueue.pop(); ///< Takes the node with the smallest frequency as the left child.
        TreeNode* rightNode = priorityQueue.top(); priorityQueue.pop(); ///< Takes the next smallest node as the right child.
        TreeNode* parentNode = new TreeNode('$', leftNode->frequency + rightNode->frequency); ///< Creates a new parent node with a sum of frequencies.
        parentNode->left = leftNode; ///< Assigns the left child to the parent node.
        parentNode->right = rightNode; ///< Assigns the right child to the parent node.
        priorityQueue.push(parentNode); ///< Pushes the parent node back into the priority queue.
    }

    TreeNode* root = priorityQueue.top();
    unordered_map<char, string> huffmanCodeMap;
    GenerateHuffmanCodes(root, "", huffmanCodeMap); ///< Generates Huffman codes starting from the root of the tree.

    ofstream codeFile(outputFileName + ".huff", ios::binary);
    if (!codeFile.is_open()) {
        cerr << "Error: cannot create the code table: " << outputFileName << ".huff" << endl;
        FreeHuffmanTree(root);
        return false;
    }
    // Numeric byte values let any symbol, including ':', survive the text format.
    for (const auto& pair : huffmanCodeMap)
        codeFile << static_cast<int>(static_cast<unsigned char>(pair.first)) << ":" << pair.second << "\n";
    codeFile.close();

    string encodedString;
    for (char ch : inputText) encodedString += huffmanCodeMap[ch]; ///< Encodes the input text using the generated Huffman codes.

    bool result = WriteEncodedStringToFile(encodedString, outputFileName);
    FreeHuffmanTree(root);
    return result;
}

/// @brief Reads the contents of a file into a string.
/// @param fileName The name of the file to read.
/// @param content Receives the contents of the file.
/// @returns True on success, false if the file cannot be opened.
bool ReadFile(const string& fileName, string& content) {
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: cannot open the input file: " << fileName << endl;
        return false;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    content = buffer.str();
    return true;
}

/// @brief Decodes the encoded string.
/// @param encodedString The string of encoded data.
/// @param outputFile The output file stream to write the decoded data.
/// @param huffmanCodes The map of Huffman codes to their corresponding characters.
void Decode(const string& encodedString, ofstream& outputFile, const unordered_map<string, char>& huffmanCodes) {
    string temp = "";
    for (const auto& bit : encodedString) {
        temp += bit; ///< Appends each bit to a temporary string.
        if (huffmanCodes.find(temp) != huffmanCodes.end()) {
            outputFile << huffmanCodes.at(temp); ///< Writes the corresponding character to the output file.
            temp = ""; ///< Resets the temporary string for the next character.
        }
    }
}

/// @brief Decodes a binary encoded file.
/// @param encodedFileName The name of the file containing encoded data.
/// @param outputFile The output file stream to write the decoded data.
/// @param huffmanCodes The map of Huffman codes to their corresponding characters.
void DecodeBinaryFile(const string& encodedFileName, ofstream& outputFile, const unordered_map<string, char>& huffmanCodes) {
    ifstream inputFile(encodedFileName, ios::binary);
    stringstream sstream;
    sstream << inputFile.rdbuf(); ///< Reads the entire binary file into a string stream.
    string encodedBinaryString = sstream.str(); ///< Converts the string stream to a string.
    inputFile.close(); ///< Closes the input file stream.

    if (encodedBinaryString.empty())
        return;

    // The first byte is the number of filler bits to drop from the tail.
    unsigned char padding = static_cast<unsigned char>(encodedBinaryString[0]);

    string encodedString;
    for (size_t i = 1; i < encodedBinaryString.size(); ++i)
    {
        bitset<8> bits(static_cast<unsigned char>(encodedBinaryString[i]));
        encodedString += bits.to_string();
    }
    if (padding <= encodedString.size())
        encodedString.resize(encodedString.size() - padding);
    Decode(encodedString, outputFile, huffmanCodes); ///< Decodes the encoded string and writes it to the output file.
}

/// @brief Decodes a file encoded with Huffman coding.
/// @param encodedFileName The name of the file containing encoded data.
/// @param huffFileName The name of the file containing Huffman codes.
/// @param outputFileName The name of the file to write the decoded data.
/// @returns True on success, false if any of the files cannot be used.
bool DecodeFile(const string& encodedFileName, const string& huffFileName, const string& outputFileName) {
    ifstream codeFile(huffFileName, ios::binary);
    if (!codeFile.is_open()) {
        cerr << "Error: cannot open the code table: " << huffFileName << endl;
        return false;
    }
    string line;
    unordered_map<string, char> huffmanCodes;
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

        huffmanCodes[code] = static_cast<char>(value);
    }
    codeFile.close();

    if (!fs::exists(encodedFileName)) {
        cerr << "Error: cannot open the compressed file: " << encodedFileName << endl;
        return false;
    }
    // An empty table is only valid for the one-byte stream of an empty file.
    if (huffmanCodes.empty() && fs::file_size(encodedFileName) > 1) {
        cerr << "Error: no valid entries in the code table: " << huffFileName << endl;
        return false;
    }

    ofstream outputFile(outputFileName, ios::binary);
    if (!outputFile.is_open()) {
        cerr << "Error: cannot create the output file: " << outputFileName << endl;
        return false;
    }
    DecodeBinaryFile(encodedFileName, outputFile, huffmanCodes);
    outputFile.close();
    return true;
}

/// @brief Calculates and displays the file size before and after compression.
/// @param inputFileName The name of the input file.
/// @param outputFileName The name of the output file.
void FileSizeCompress(const string& inputFileName, const string& outputFileName) {
    string tableFileName = outputFileName + ".huff";
    if (!fs::exists(inputFileName) || !fs::exists(outputFileName) || !fs::exists(tableFileName))
        return;

    auto inputSize = fs::file_size(inputFileName);
    auto outputSize = fs::file_size(outputFileName);
    auto tableSize = fs::file_size(tableFileName);
    // The code table is required for decompression, so honest numbers count it.
    auto totalSize = outputSize + tableSize;

    cout << "Compression completed!" << endl;
    cout << "Original Size: " << inputSize << " bytes\n";
    cout << "Compressed Size: " << outputSize << " bytes (+ " << tableSize << " bytes code table)\n";

    if (inputSize > 0) {
        double compressionPercent = 100.0 * (1 - (double)totalSize / inputSize);
        cout << "Compression Percentage: " << compressionPercent << "%\n";
    }
}

/// @brief Calculates and displays the file size before and after decompression.
/// @param inputFileName The name of the compressed input file.
/// @param outputFileName The name of the decompressed output file.
void FileSizeDecompress(const string& inputFileName, const string& outputFileName) {
    string tableFileName = inputFileName + ".huff";
    if (!fs::exists(inputFileName) || !fs::exists(outputFileName) || !fs::exists(tableFileName))
        return;

    auto inputSize = fs::file_size(inputFileName);
    auto tableSize = fs::file_size(tableFileName);
    auto outputSize = fs::file_size(outputFileName);
    auto totalSize = inputSize + tableSize;

    cout << "Decompression completed!" << endl;
    cout << "Compressed Size: " << inputSize << " bytes (+ " << tableSize << " bytes code table)\n";
    cout << "Decompressed Size: " << outputSize << " bytes\n";

    if (totalSize > 0) {
        double decompressionIncreasePercent = 100.0 * ((double)outputSize / totalSize - 1);
        cout << "Decompression Increase Percentage: " << decompressionIncreasePercent << "%\n";
    }
}

/// @brief The main function handling command line arguments for compressing or decompressing files.
/// @param argc Number of command line arguments.
/// @param argv Array of command line arguments.
/// @returns Returns 0 on successful execution, or 1 on error.
int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <action> <input file> <output file>" << endl; ///< Checks for the correct number of arguments and displays usage instructions.
        return 1; ///< Exits with an error code if the number of arguments is incorrect.
    }

    string action = argv[1]; ///< Stores the action ('c' for compress, 'd' for decompress).
    string inputFileName = argv[2]; ///< Stores the name of the input file.
    string outputFileName = argv[3]; ///< Stores the name of the output file.

    if (action == "c") {
        string text;
        if (!ReadFile(inputFileName, text))
            return 1;
        if (!BuildHuffmanTree(text, outputFileName))
            return 1;
        FileSizeCompress(inputFileName, outputFileName);
    }
    else if (action == "d") {
        if (!DecodeFile(inputFileName, inputFileName + ".huff", outputFileName))
            return 1;
        FileSizeDecompress(inputFileName, outputFileName);
    }
    else {
        cerr << "Invalid action. Use 'c' for compress and 'd' for decompress." << endl; ///< Handles invalid actions.
        return 1; ///< Exits with an error code for invalid actions.
    }

    return 0; ///< Indicates successful execution.
}
