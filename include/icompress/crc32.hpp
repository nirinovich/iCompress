#pragma once

#include "icompress/types.hpp"

namespace icompress {

class CRC32 {
public:
    static uint32_t calculate(const Byte* data, size_t length);
    static uint32_t calculate(const ByteBuffer& buffer);
    static uint32_t update(uint32_t crc, const Byte* data, size_t length);

private:
    static const uint32_t Table[256];
};

} // namespace icompress
