#include "icompress/bitstream.hpp"

namespace icompress {

void BitWriter::write_bit(bool bit) {
    if (bit) {
        current_byte_ |= static_cast<Byte>(1 << (7 - bit_pos_));
    }
    bit_pos_++;
    if (bit_pos_ == 8) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bit_pos_ = 0;
    }
}

void BitWriter::write_bits(uint64_t value, size_t num_bits) {
    for (size_t i = 0; i < num_bits; ++i) {
        size_t shift = num_bits - 1 - i;
        bool bit = ((value >> shift) & 1) != 0;
        write_bit(bit);
    }
}

void BitWriter::write_byte(Byte byte) {
    if (bit_pos_ == 0) {
        buffer_.push_back(byte);
    } else {
        write_bits(byte, 8);
    }
}

void BitWriter::write_bytes(const Byte* data, size_t length) {
    if (bit_pos_ == 0) {
        buffer_.insert(buffer_.end(), data, data + length);
    } else {
        for (size_t i = 0; i < length; ++i) {
            write_bits(data[i], 8);
        }
    }
}

void BitWriter::write_bytes(const ByteBuffer& buffer) {
    write_bytes(buffer.data(), buffer.size());
}

void BitWriter::flush() {
    if (bit_pos_ > 0) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bit_pos_ = 0;
    }
}

BitReader::BitReader(const ByteBuffer& buffer)
    : data_(buffer.data()), size_(buffer.size()) {}

BitReader::BitReader(const Byte* data, size_t size)
    : data_(data), size_(size) {}

bool BitReader::read_bit() {
    if (!has_more()) {
        throw std::runtime_error("Unexpected end of bitstream");
    }
    bool bit = ((data_[byte_pos_] >> (7 - bit_pos_)) & 1) != 0;
    bit_pos_++;
    if (bit_pos_ == 8) {
        byte_pos_++;
        bit_pos_ = 0;
    }
    return bit;
}

uint64_t BitReader::read_bits(size_t num_bits) {
    uint64_t result = 0;
    for (size_t i = 0; i < num_bits; ++i) {
        result = (result << 1) | (read_bit() ? 1 : 0);
    }
    return result;
}

Byte BitReader::read_byte() {
    if (bit_pos_ == 0) {
        if (byte_pos_ >= size_) {
            throw std::runtime_error("Unexpected end of byte stream");
        }
        return data_[byte_pos_++];
    }
    return static_cast<Byte>(read_bits(8));
}

void BitReader::read_bytes(Byte* dest, size_t length) {
    if (bit_pos_ == 0) {
        if (byte_pos_ + length > size_) {
            throw std::runtime_error("Buffer underflow reading bytes");
        }
        std::copy(data_ + byte_pos_, data_ + byte_pos_ + length, dest);
        byte_pos_ += length;
    } else {
        for (size_t i = 0; i < length; ++i) {
            dest[i] = read_byte();
        }
    }
}

ByteBuffer BitReader::read_bytes(size_t length) {
    ByteBuffer result(length);
    read_bytes(result.data(), length);
    return result;
}

void BitReader::align_to_byte() {
    if (bit_pos_ > 0) {
        byte_pos_++;
        bit_pos_ = 0;
    }
}

bool BitReader::has_more() const {
    return byte_pos_ < size_;
}

size_t BitReader::remaining_bits() const {
    if (byte_pos_ >= size_) return 0;
    return (size_ - byte_pos_) * 8 - bit_pos_;
}

} // namespace icompress
