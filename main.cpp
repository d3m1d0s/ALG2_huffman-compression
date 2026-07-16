#include <filesystem>
#include <iostream>
#include <string>

#include "huffman.h"

using namespace std;
namespace fs = std::filesystem;

/// Prints the size summary after compression.
void FileSizeCompress(const string& inputFileName, const string& outputFileName) {
    string tableFileName = outputFileName + ".huff";
    if (!fs::exists(inputFileName) || !fs::exists(outputFileName) || !fs::exists(tableFileName))
        return;

    auto inputSize = fs::file_size(inputFileName);
    auto outputSize = fs::file_size(outputFileName);
    auto tableSize = fs::file_size(tableFileName);
    // The code table is required for decompression, so honest numbers have to count it
    auto totalSize = outputSize + tableSize;

    cout << "Compression completed!" << endl;
    cout << "Original Size: " << inputSize << " bytes\n";
    cout << "Compressed Size: " << outputSize << " bytes (+ " << tableSize << " bytes code table)\n";

    if (inputSize > 0) {
        double compressionPercent = 100.0 * (1 - (double)totalSize / inputSize);
        cout << "Compression Percentage: " << compressionPercent << "%\n";
    }
}

/// Prints the size summary after decompression.
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

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <c|d> <input> <output>" << endl;
        return 1;
    }

    string action = argv[1];
    string inputFileName = argv[2];
    string outputFileName = argv[3];

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
        cerr << "Invalid action. Use 'c' for compress and 'd' for decompress." << endl;
        return 1;
    }

    return 0;
}
