#pragma once

#include "icompress/codec.hpp"

namespace icompress {

class ArithmeticCodec : public ICodec {
public:
    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::Arithmetic; }
};

} // namespace icompress
