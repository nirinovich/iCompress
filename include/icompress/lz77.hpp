#pragma once

#include "icompress/codec.hpp"

namespace icompress {

struct LZ77Token {
    uint16_t distance{0};
    uint16_t length{0};
    Byte next_char{0};
};

class LZ77Codec : public ICodec {
public:
    explicit LZ77Codec(uint16_t window_size = 4096, uint16_t lookahead_size = 258);

    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::LZ77; }

    // Standalone token generation for testing and analysis
    std::vector<LZ77Token> tokenize(const ByteBuffer& input) const;

private:
    uint16_t window_size_;
    uint16_t lookahead_size_;
};

} // namespace icompress
