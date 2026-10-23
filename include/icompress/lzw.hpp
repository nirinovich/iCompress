#pragma once

#include "icompress/codec.hpp"

namespace icompress {

class LZWCodec : public ICodec {
public:
    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::LZW; }

    static constexpr uint16_t MAX_DICT_ENTRIES = 65535;
    static constexpr uint16_t CLEAR_CODE = 256;
};

} // namespace icompress
