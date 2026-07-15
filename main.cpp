#include <filesystem>
#include <iostream>
#include <string>

#include "huffman.h"

using namespace std;
namespace fs = std::filesystem;

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
        if (!huffman::Compress(inputFileName, outputFileName))
            return 1;
        FileSizeCompress(inputFileName, outputFileName);
    }
    else if (action == "d") {
        if (!huffman::Decompress(inputFileName, outputFileName))
            return 1;
        FileSizeDecompress(inputFileName, outputFileName);
    }
    else {
        cerr << "Invalid action. Use 'c' for compress and 'd' for decompress." << endl; ///< Handles invalid actions.
        return 1; ///< Exits with an error code for invalid actions.
    }

    return 0; ///< Indicates successful execution.
}
