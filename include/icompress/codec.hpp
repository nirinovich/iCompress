#pragma once

#include "icompress/types.hpp"
#include <memory>

namespace icompress {

class ICodec {
public:
    virtual ~ICodec() = default;

    virtual ByteBuffer compress(const ByteBuffer& input) = 0;
    virtual ByteBuffer decompress(const ByteBuffer& input, size_t original_size) = 0;
    virtual Algorithm algorithm() const = 0;

    static std::unique_ptr<ICodec> create(Algorithm algo);
};

} // namespace icompress
