#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include <memory>

namespace icompress {

using Byte = uint8_t;
using ByteBuffer = std::vector<Byte>;

enum class Algorithm : uint8_t {
    Auto = 0,
    Huffman = 1,
    RLE = 2,
    BWT_RLE = 3,
    Arithmetic = 4,
    LZ77 = 5,
    LZ78 = 6,
    LZW = 7,
    Deflate = 8
};

inline std::string_view algorithm_name(Algorithm algo) {
    switch (algo) {
        case Algorithm::Auto: return "auto";
        case Algorithm::Huffman: return "huffman";
        case Algorithm::RLE: return "rle";
        case Algorithm::BWT_RLE: return "bwt";
        case Algorithm::Arithmetic: return "arithmetic";
        case Algorithm::LZ77: return "lz77";
        case Algorithm::LZ78: return "lz78";
        case Algorithm::LZW: return "lzw";
        case Algorithm::Deflate: return "deflate";
        default: return "unknown";
    }
}

inline Algorithm parse_algorithm(std::string_view name) {
    if (name == "auto" || name == "deflate") return Algorithm::Deflate;
    if (name == "huffman" || name == "huff") return Algorithm::Huffman;
    if (name == "rle") return Algorithm::RLE;
    if (name == "bwt" || name == "bzip") return Algorithm::BWT_RLE;
    if (name == "arithmetic" || name == "arith") return Algorithm::Arithmetic;
    if (name == "lz77") return Algorithm::LZ77;
    if (name == "lz78") return Algorithm::LZ78;
    if (name == "lzw") return Algorithm::LZW;
    throw std::runtime_error("Unknown compression algorithm: " + std::string(name));
}

struct CompressionStats {
    Algorithm algorithm{Algorithm::Auto};
    uint64_t original_size{0};
    uint64_t compressed_size{0};
    double ratio{1.0};
    double space_savings{0.0};
    double duration_ms{0.0};
    uint32_t crc32{0};
    bool verified{false};
};

} // namespace icompress
