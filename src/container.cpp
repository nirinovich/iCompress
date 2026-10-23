#include "icompress/container.hpp"
#include <fstream>
#include <cstring>

namespace icompress {

ByteBuffer Container::pack(const ArchiveHeader& header, const ByteBuffer& payload) {
    ByteBuffer archive;
    archive.reserve(HEADER_SIZE + payload.size());

    // Magic: 4 bytes
    archive.push_back(static_cast<Byte>(header.magic & 0xFF));
    archive.push_back(static_cast<Byte>((header.magic >> 8) & 0xFF));
    archive.push_back(static_cast<Byte>((header.magic >> 16) & 0xFF));
    archive.push_back(static_cast<Byte>((header.magic >> 24) & 0xFF));

    // Version: 1 byte
    archive.push_back(header.version);

    // Algorithm: 1 byte
    archive.push_back(static_cast<Byte>(header.algorithm));

    // Flags: 2 bytes
    archive.push_back(static_cast<Byte>(header.flags & 0xFF));
    archive.push_back(static_cast<Byte>((header.flags >> 8) & 0xFF));

    // Original Size: 8 bytes (little endian)
    for (int i = 0; i < 8; ++i) {
        archive.push_back(static_cast<Byte>((header.original_size >> (i * 8)) & 0xFF));
    }

    // CRC32: 4 bytes (little endian)
    for (int i = 0; i < 4; ++i) {
        archive.push_back(static_cast<Byte>((header.original_crc32 >> (i * 8)) & 0xFF));
    }

    // Payload
    archive.insert(archive.end(), payload.begin(), payload.end());
    return archive;
}

ArchiveHeader Container::unpack_header(const Byte* data, size_t size) {
    if (size < HEADER_SIZE) {
        throw std::runtime_error("File too small to be a valid iCompress archive");
    }

    ArchiveHeader header;
    header.magic = static_cast<uint32_t>(data[0]) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) |
                   (static_cast<uint32_t>(data[3]) << 24);

    if (header.magic != MAGIC_HEADER) {
        throw std::runtime_error("Invalid file format: missing ICMP magic header");
    }

    header.version = data[4];
    if (header.version != FORMAT_VERSION) {
        throw std::runtime_error("Unsupported archive version: " + std::to_string(header.version));
    }

    header.algorithm = static_cast<Algorithm>(data[5]);
    header.flags = static_cast<uint16_t>(data[6]) |
                   (static_cast<uint16_t>(data[7]) << 8);

    header.original_size = 0;
    for (int i = 0; i < 8; ++i) {
        header.original_size |= (static_cast<uint64_t>(data[8 + i]) << (i * 8));
    }

    header.original_crc32 = 0;
    for (int i = 0; i < 4; ++i) {
        header.original_crc32 |= (static_cast<uint32_t>(data[16 + i]) << (i * 8));
    }

    return header;
}

ArchiveHeader Container::unpack_header(const ByteBuffer& data) {
    return unpack_header(data.data(), data.size());
}

ByteBuffer Container::extract_payload(const ByteBuffer& data) {
    if (data.size() < HEADER_SIZE) {
        throw std::runtime_error("Archive size less than header size");
    }
    return ByteBuffer(data.begin() + HEADER_SIZE, data.end());
}

void Container::write_file(const std::string& path, const ByteBuffer& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) {
        throw std::runtime_error("Failed to write data to file: " + path);
    }
}

ByteBuffer Container::read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + path);
    }
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    ByteBuffer buffer(static_cast<size_t>(size));
    if (size > 0) {
        if (!in.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Failed to read file: " + path);
        }
    }
    return buffer;
}

} // namespace icompress
