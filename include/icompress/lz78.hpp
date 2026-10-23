#pragma once

#include "icompress/codec.hpp"

namespace icompress {

struct LZ78Token {
    uint32_t dict_index{0};
    Byte next_char{0};
    bool has_next{true};
};

class LZ78Codec : public ICodec {
public:
    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::LZ78; }

    std::vector<LZ78Token> tokenize(const ByteBuffer& input) const;
};

} // namespace icompress
