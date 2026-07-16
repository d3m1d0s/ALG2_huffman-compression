# Huffman Compressor

## Overview
This is a command-line file compressor based on Huffman coding, made as a
semester project for the Algorithms II course at VSB-TUO. It compresses a
file into a binary file plus a text file with the Huffman codes, and
decompresses them back into the original.

## Build
To build the program, ensure CMake 3.16 or newer and a C++20 compiler are
installed and run:

    mkdir build && cd build
    cmake ..
    make

This produces an executable named `HuffmanCompressor`. The project can also
be opened directly in CLion.

## Usage
Run the program from the project root:

    ./build/HuffmanCompressor <c|d> <input> <output>

- `c` - compress `<input>` into `<output>`
- `d` - decompress `<input>` into `<output>`

Compression also creates `<output>.huff` with the code of every byte.
Decompression looks for `<input>.huff` next to the compressed file, so
keep the two files together. After each run the program prints the sizes
before and after.

Example:

    ./build/HuffmanCompressor c input.txt compressed.bin
    ./build/HuffmanCompressor d compressed.bin output.txt

The included `input.txt` is "The Gold-Bug" by Edgar Allan Poe. To make sure
the decompressed file matches the original, compare them with
`cmp input.txt output.txt` on Linux or `fc /b input.txt output.txt` on
Windows.

## Documentation
Run `doxygen` in the project root to generate the documentation into
`docs/`. It is also available at
https://d3m1d0s.github.io/ALG2_huffman-compression/.

## License
This project is authored by Demid Ostiakov. All rights reserved.

## Acknowledgments
Thanks to doc. Mgr. Jiří Dvorský, Ph.D. from VSB-TUO, whose Algorithms
lectures gave me the understanding this project is built on.
