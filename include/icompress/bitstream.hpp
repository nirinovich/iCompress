#pragma once

#include "icompress/types.hpp"

namespace icompress {

class BitWriter {
public:
    BitWriter() = default;

    void write_bit(bool bit);
    void write_bits(uint64_t value, size_t num_bits);
    void write_byte(Byte byte);
    void write_bytes(const Byte* data, size_t length);
    void write_bytes(const ByteBuffer& buffer);

    void flush();
    const ByteBuffer& buffer() const { return buffer_; }
    ByteBuffer take_buffer() { flush(); return std::move(buffer_); }
    size_t bit_count() const { return buffer_.size() * 8 - (8 - bit_pos_) % 8; }

private:
    ByteBuffer buffer_;
    Byte current_byte_{0};
    uint8_t bit_pos_{0}; // 0..7 bits written into current_byte_ (MSB first)
};

class BitReader {
public:
    explicit BitReader(const ByteBuffer& buffer);
    BitReader(const Byte* data, size_t size);

    bool read_bit();
    uint64_t read_bits(size_t num_bits);
    Byte read_byte();
    void read_bytes(Byte* dest, size_t length);
    ByteBuffer read_bytes(size_t length);

    void align_to_byte();
    bool has_more() const;
    size_t remaining_bits() const;
    size_t byte_position() const { return byte_pos_; }

private:
    const Byte* data_{nullptr};
    size_t size_{0};
    size_t byte_pos_{0};
    uint8_t bit_pos_{0}; // 0..7 bit index in current byte (MSB first)
};

} // namespace icompress
