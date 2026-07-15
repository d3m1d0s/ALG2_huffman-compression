#include <iostream> // Standard library for input and output streams.
#include <fstream> // Library for file stream operations.
#include <queue> // Library for using the queue data structure.
#include <unordered_map> // Library for using hash maps.
#include <cstring> // Library for string manipulation functions.
#include <sstream> // Library for string stream operations.
#include <bitset> // Library for bitset operations.
#include <filesystem> // Library for file size operations.

using namespace std; // Using the standard namespace.
namespace fs = std::filesystem; // Use a namespace alias for simplicity.

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

    // Constructor
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
void GenerateHuffmanCodes(TreeNode* root, string str, unordered_map<char, string>& huffmanCodes)
{
    if (root == nullptr)
        return; ///< Base case: if the root is null, do nothing.

    if (!root->left && !root->right)
        // A single-symbol alphabet makes the root itself a leaf, so its path
        // is empty; such a symbol still needs a real code to be decodable.
        huffmanCodes[root->character] = str.empty() ? "0" : str;

    GenerateHuffmanCodes(root->left, str + "0", huffmanCodes); ///< Recursively traverse the left child, appending "0" to the code string.
    GenerateHuffmanCodes(root->right, str + "1", huffmanCodes); ///< Recursively traverse the right child, appending "1" to the code string.
}

/// @brief Writes the encoded string to a binary file.
/// @param encodedString The string of encoded data.
/// @param outputFileName The name of the file to write the encoded data to.
void WriteEncodedStringToFile(const string& encodedString, const string& outputFileName) {
    ofstream outputFile(outputFileName, ios::binary);

    // The first byte of the file stores how many filler zero bits
    // were appended to complete the last data byte (0..7).
    unsigned char padding = (8 - encodedString.size() % 8) % 8;
    outputFile.put(static_cast<char>(padding));

    for (size_t i = 0; i < encodedString.size(); i += 8) {
        string byteBits = encodedString.substr(i, 8);
        byteBits.append(8 - byteBits.size(), '0');
        outputFile.put(static_cast<char>(bitset<8>(byteBits).to_ulong()));
    }
    outputFile.close();
}

/// @brief Builds the Huffman tree from the input text.
/// @param inputText The text to be encoded.
/// @param outputFileName The name of the file to store the Huffman codes.
/// @param priorityQueue The priority queue to construct the Huffman tree.
void BuildHuffmanTree(string inputText, const string& outputFileName, priority_queue<TreeNode*, vector<TreeNode*>, CompareNodes>& priorityQueue)
{
    unordered_map<char, unsigned> charFrequencyMap;
    for (char ch : inputText)
        charFrequencyMap[ch]++;

    // An empty input has no alphabet to build a tree from; emit an empty
    // table and a header-only stream so decompression restores an empty file.
    if (charFrequencyMap.empty()) {
        ofstream codeFile(outputFileName + ".huff");
        WriteEncodedStringToFile("", outputFileName);
        return;
    }

    for (auto pair : charFrequencyMap)
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

    unordered_map<char, string> huffmanCodeMap;
    GenerateHuffmanCodes(priorityQueue.top(), "", huffmanCodeMap); ///< Generates Huffman codes starting from the root of the tree.

    ofstream codeFile(outputFileName + ".huff");
    // Symbols are stored as numeric byte values so that any byte,
    // including ':' and control characters, survives the text format.
    for (const auto& pair : huffmanCodeMap)
        codeFile << static_cast<int>(static_cast<unsigned char>(pair.first)) << ":" << pair.second << "\n";
    codeFile.close();

    string encodedString;
    for (char ch : inputText) encodedString += huffmanCodeMap[ch]; ///< Encodes the input text using the generated Huffman codes.

    WriteEncodedStringToFile(encodedString, outputFileName); ///< Writes the encoded string to a binary file.
}

/// @brief Reads and returns the contents of a file as a string.
/// @param fileName The name of the file to read.
/// @returns The contents of the file as a string.
string ReadFile(const string& fileName) {
    ifstream file(fileName);
    stringstream buffer;
    buffer << file.rdbuf(); ///< Reads the entire contents of the file into a string buffer.
    file.close(); ///< Closes the file stream.
    return buffer.str(); ///< Returns the contents of the file as a string.
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

    // The first byte stores the number of filler bits in the last data byte;
    // they are dropped below so they cannot decode into extra symbols.
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
void DecodeFile(const string& encodedFileName, const string& huffFileName, const string& outputFileName) {
    ifstream codeFile(huffFileName);
    string line;
    unordered_map<string, char> huffmanCodes;
    while (getline(codeFile, line)) {
        if (line.empty())
            continue;

        // Expected line format: <byte value 0..255>:<code made of '0'/'1'>.
        // Lines that do not match are skipped instead of producing garbage entries.
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

    ofstream outputFile(outputFileName);
    DecodeBinaryFile(encodedFileName, outputFile, huffmanCodes); ///< Decodes the binary file and writes the output to a file.
    outputFile.close(); ///< Closes the output file stream.
}

/// @brief Calculates and displays the file size before and after compression.
/// @param inputFileName The name of the input file.
/// @param outputFileName The name of the output file.
void FileSizeCompress(const string& inputFileName, const string& outputFileName) {
    auto inputSize = fs::file_size(inputFileName); ///< Gets the size of the input file in bytes.
    auto outputSize = fs::file_size(outputFileName); ///< Gets the size of the output file in bytes.

    cout << "Compression completed!" << endl;

    cout << "Original Size: " << inputSize << " bytes\n"; ///< Displays the original file size.
    cout << "Compressed Size: " << outputSize << " bytes\n"; ///< Displays the compressed file size.

    double compressionPercent = 100.0 * (1 - (double)outputSize / inputSize); ///< Calculates the compression percentage.
    cout << "Compression Percentage: " << compressionPercent << "%\n"; ///< Displays the compression percentage.
}

/// @brief Calculates and displays the file size before and after decompression.
/// @param inputFileName The name of the compressed input file.
/// @param outputFileName The name of the decompressed output file.
void FileSizeDecompress(const string& inputFileName, const string& outputFileName) {
    auto inputSize = fs::file_size(inputFileName); ///< Gets the size of the compressed file in bytes.
    auto outputSize = fs::file_size(outputFileName); ///< Gets the size of the decompressed file in bytes.

    cout << "Decompression completed!" << endl;

    cout << "Compressed Size: " << inputSize << " bytes\n"; ///< Displays the compressed file size.
    cout << "Decompressed Size: " << outputSize << " bytes\n"; ///< Displays the decompressed file size.

    double decompressionIncreasePercent = 100.0 * ((double)outputSize / inputSize - 1); ///< Calculates the decompression increase percentage.
    cout << "Decompression Increase Percentage: " << decompressionIncreasePercent << "%\n"; ///< Displays the decompression increase percentage.
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
        string text = ReadFile(inputFileName); ///< Reads the input file.
        priority_queue<TreeNode*, vector<TreeNode*>, CompareNodes> pq; ///< Creates a priority queue for building the Huffman tree.
        BuildHuffmanTree(text, outputFileName, pq); ///< Builds the Huffman tree and encodes the file.
        FileSizeCompress(inputFileName, outputFileName); ///< Displays the file size before and after compression.
    }
    else if (action == "d") {
        DecodeFile(inputFileName, inputFileName + ".huff", outputFileName); ///< Decodes the file.
        FileSizeDecompress(inputFileName, outputFileName); ///< Displays the file size before and after decompression.
    }
    else {
        cerr << "Invalid action. Use 'c' for compress and 'd' for decompress." << endl; ///< Handles invalid actions.
        return 1; ///< Exits with an error code for invalid actions.
    }

    return 0; ///< Indicates successful execution.
}
