#pragma once

#include "icompress/codec.hpp"

namespace icompress {

class BWTCodec : public ICodec {
public:
    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::BWT_RLE; }

    // Standalone block transform functions
    static std::pair<ByteBuffer, uint32_t> forward_transform(const ByteBuffer& block);
    static ByteBuffer inverse_transform(const ByteBuffer& last_column, uint32_t primary_index);

private:
    static constexpr size_t BLOCK_SIZE = 8192; // 8 KB block size for optimal throughput
};

} // namespace icompress
