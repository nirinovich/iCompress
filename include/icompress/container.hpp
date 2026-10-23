#pragma once

#include "icompress/types.hpp"
#include <string>

namespace icompress {

constexpr uint32_t MAGIC_HEADER = 0x504D4349; // 'ICMP' in little-endian
constexpr uint8_t FORMAT_VERSION = 1;
constexpr size_t HEADER_SIZE = 20;

struct ArchiveHeader {
    uint32_t magic{MAGIC_HEADER};
    uint8_t version{FORMAT_VERSION};
    Algorithm algorithm{Algorithm::Auto};
    uint16_t flags{0};
    uint64_t original_size{0};
    uint32_t original_crc32{0};
};

class Container {
public:
    static ByteBuffer pack(const ArchiveHeader& header, const ByteBuffer& payload);
    static ArchiveHeader unpack_header(const ByteBuffer& data);
    static ArchiveHeader unpack_header(const Byte* data, size_t size);
    static ByteBuffer extract_payload(const ByteBuffer& data);

    static void write_file(const std::string& path, const ByteBuffer& data);
    static ByteBuffer read_file(const std::string& path);
};

} // namespace icompress
