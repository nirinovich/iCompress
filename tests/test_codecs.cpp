#include "icompress/codec.hpp"
#include "icompress/huffman.hpp"
#include "icompress/rle.hpp"
#include "icompress/bwt.hpp"
#include "icompress/arithmetic.hpp"
#include "icompress/lz77.hpp"
#include "icompress/lz78.hpp"
#include "icompress/lzw.hpp"
#include "icompress/deflate.hpp"
#include "icompress/crc32.hpp"
#include <iostream>
#include <cassert>
#include <string>

using namespace icompress;

static void test_huffman() {
    std::cout << "[TEST] Huffman Coding... ";
    std::string text = "ABRACADABRA";
    ByteBuffer input(text.begin(), text.end());

    HuffmanCodec codec;
    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());

    assert(decompressed == input);
    assert(CRC32::calculate(decompressed) == CRC32::calculate(input));

    // Test numbers example from PDF
    std::string num_seq = "5 7 5 8 8 7 5 7 8 5";
    ByteBuffer input2(num_seq.begin(), num_seq.end());
    ByteBuffer comp2 = codec.compress(input2);
    ByteBuffer decomp2 = codec.decompress(comp2, input2.size());
    assert(decomp2 == input2);

    std::cout << "PASSED\n";
}

static void test_rle() {
    std::cout << "[TEST] Run-Length Encoding (RLE)... ";
    std::string text = "AAAABBBCCDAA";
    ByteBuffer input(text.begin(), text.end());

    RLECodec codec;
    ByteBuffer compressed = codec.compress(input);

    // PDF expected: 4A 3B 2C 1D 2A
    assert(compressed.size() == 10);
    assert(compressed[0] == 4 && compressed[1] == 'A');
    assert(compressed[2] == 3 && compressed[3] == 'B');
    assert(compressed[4] == 2 && compressed[5] == 'C');
    assert(compressed[6] == 1 && compressed[7] == 'D');
    assert(compressed[8] == 2 && compressed[9] == 'A');

    ByteBuffer decompressed = codec.decompress(compressed, input.size());
    assert(decompressed == input);
    std::cout << "PASSED\n";
}

static void test_bwt() {
    std::cout << "[TEST] Burrows-Wheeler Transform (BWT)... ";
    std::string text = "BANANA";
    ByteBuffer input(text.begin(), text.end());

    auto [transformed, primary_idx] = BWTCodec::forward_transform(input);
    std::string last_col(transformed.begin(), transformed.end());

    // PDF expected: "NNBAAA", primary index 3 (0-indexed)
    assert(last_col == "NNBAAA");
    assert(primary_idx == 3);

    ByteBuffer inverted = BWTCodec::inverse_transform(transformed, primary_idx);
    assert(inverted == input);

    // End-to-end BWT codec roundtrip
    BWTCodec codec;
    ByteBuffer comp = codec.compress(input);
    ByteBuffer decomp = codec.decompress(comp, input.size());
    assert(decomp == input);

    std::cout << "PASSED\n";
}

static void test_arithmetic() {
    std::cout << "[TEST] Arithmetic Coding... ";
    std::string text = "CALCUL";
    ByteBuffer input(text.begin(), text.end());

    ArithmeticCodec codec;
    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());

    assert(decompressed == input);
    assert(CRC32::calculate(decompressed) == CRC32::calculate(input));
    std::cout << "PASSED\n";
}

static void test_lz77() {
    std::cout << "[TEST] LZ77 Sliding Window... ";
    std::string text = "CABRACADABRARRARRAD";
    ByteBuffer input(text.begin(), text.end());

    LZ77Codec codec;
    auto tokens = codec.tokenize(input);

    // Verify token sequence matches PDF worked example
    // (0,0,'C'), (0,0,'A'), (0,0,'B'), (0,0,'R'), (3,1,'C'), (2,1,'D'), (7,4,'R'), (3,5,'D')
    assert(!tokens.empty());
    assert(tokens[0].distance == 0 && tokens[0].length == 0 && tokens[0].next_char == 'C');
    assert(tokens[1].distance == 0 && tokens[1].length == 0 && tokens[1].next_char == 'A');
    assert(tokens[2].distance == 0 && tokens[2].length == 0 && tokens[2].next_char == 'B');
    assert(tokens[3].distance == 0 && tokens[3].length == 0 && tokens[3].next_char == 'R');
    assert(tokens[4].distance == 3 && tokens[4].length == 1 && tokens[4].next_char == 'C');
    assert(tokens[5].distance == 2 && tokens[5].length == 1 && tokens[5].next_char == 'D');
    assert(tokens[6].distance == 7 && tokens[6].length == 4 && tokens[6].next_char == 'R');
    assert(tokens[7].distance == 3 && tokens[7].length == 5 && tokens[7].next_char == 'D');

    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());
    assert(decompressed == input);
    std::cout << "PASSED\n";
}

static void test_lz78() {
    std::cout << "[TEST] LZ78 Dictionary Coding... ";
    std::string text = "WABBATWABBATWABBATWOOTWOOTWO";
    ByteBuffer input(text.begin(), text.end());

    LZ78Codec codec;
    auto tokens = codec.tokenize(input);

    // Check first tokens from PDF:
    // (0, 'W'), (0, 'A'), (0, 'B'), (3, 'A'), (0, 'T'), (1, 'A'), (3, 'B')...
    assert(tokens[0].dict_index == 0 && tokens[0].next_char == 'W');
    assert(tokens[1].dict_index == 0 && tokens[1].next_char == 'A');
    assert(tokens[2].dict_index == 0 && tokens[2].next_char == 'B');
    assert(tokens[3].dict_index == 3 && tokens[3].next_char == 'A');
    assert(tokens[4].dict_index == 0 && tokens[4].next_char == 'T');
    assert(tokens[5].dict_index == 1 && tokens[5].next_char == 'A');
    assert(tokens[6].dict_index == 3 && tokens[6].next_char == 'B');

    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());
    assert(decompressed == input);
    std::cout << "PASSED\n";
}

static void test_lzw() {
    std::cout << "[TEST] LZW Dictionary Coding... ";
    std::string text = "ABABBABCABABBA";
    ByteBuffer input(text.begin(), text.end());

    LZWCodec codec;
    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());

    assert(decompressed == input);
    assert(CRC32::calculate(decompressed) == CRC32::calculate(input));
    std::cout << "PASSED\n";
}

static void test_deflate() {
    std::cout << "[TEST] DEFLATE Hybrid Engine... ";
    std::string text = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG! "
                       "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG! "
                       "REPEATED PATTERN OF TEXT FOR COMPRESSION ANALYSIS!";
    ByteBuffer input(text.begin(), text.end());

    DeflateCodec codec;
    ByteBuffer compressed = codec.compress(input);
    ByteBuffer decompressed = codec.decompress(compressed, input.size());

    assert(decompressed == input);
    assert(CRC32::calculate(decompressed) == CRC32::calculate(input));
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\n=== Running iCompress Reference Algorithm Unit Tests ===\n";
    test_huffman();
    test_rle();
    test_bwt();
    test_arithmetic();
    test_lz77();
    test_lz78();
    test_lzw();
    test_deflate();
    std::cout << "=== ALL 8 REFERENCE ALGORITHMS VERIFIED SUCCESSFULLY ===\n\n";
    return 0;
}
