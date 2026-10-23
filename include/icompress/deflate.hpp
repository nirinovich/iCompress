#pragma once

#include "icompress/codec.hpp"

namespace icompress {

class DeflateCodec : public ICodec {
public:
    explicit DeflateCodec(uint16_t window_size = 32768, uint16_t max_match = 258);

    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::Deflate; }

private:
    uint16_t window_size_;
    uint16_t max_match_;
};

} // namespace icompress
