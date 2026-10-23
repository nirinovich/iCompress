#include "icompress/codec.hpp"
#include "icompress/huffman.hpp"
#include "icompress/rle.hpp"
#include "icompress/bwt.hpp"
#include "icompress/arithmetic.hpp"
#include "icompress/lz77.hpp"
#include "icompress/lz78.hpp"
#include "icompress/lzw.hpp"
#include "icompress/deflate.hpp"

namespace icompress {

std::unique_ptr<ICodec> ICodec::create(Algorithm algo) {
    switch (algo) {
        case Algorithm::Huffman:
            return std::make_unique<HuffmanCodec>();
        case Algorithm::RLE:
            return std::make_unique<RLECodec>();
        case Algorithm::BWT_RLE:
            return std::make_unique<BWTCodec>();
        case Algorithm::Arithmetic:
            return std::make_unique<ArithmeticCodec>();
        case Algorithm::LZ77:
            return std::make_unique<LZ77Codec>();
        case Algorithm::LZ78:
            return std::make_unique<LZ78Codec>();
        case Algorithm::LZW:
            return std::make_unique<LZWCodec>();
        case Algorithm::Auto:
        case Algorithm::Deflate:
            return std::make_unique<DeflateCodec>();
        default:
            throw std::runtime_error("Unsupported algorithm");
    }
}

} // namespace icompress
