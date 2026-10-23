#include "icompress/arithmetic.hpp"
#include "icompress/bitstream.hpp"
#include <array>
#include <stdexcept>
#include <algorithm>

namespace icompress {

constexpr size_t ARITH_BLOCK_SIZE = 65536; // 64 KB block size to guarantee precision
constexpr uint64_t ARITH_TOP = 0xFFFFFFFFULL;
constexpr uint64_t ARITH_FIRST_QTR = 0x40000000ULL;
constexpr uint64_t ARITH_HALF = 0x80000000ULL;
constexpr uint64_t ARITH_THIRD_QTR = 0xC0000000ULL;

static void compress_block(const Byte* data, size_t length, BitWriter& writer) {
    if (length == 0) return;

    // 1. Calculate frequencies and cumulative frequencies
    std::array<uint32_t, 256> freqs{};
    for (size_t i = 0; i < length; ++i) {
        freqs[data[i]]++;
    }

    uint16_t unique_count = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) unique_count++;
    }

    // Write header: unique symbol count + (symbol, freq)
    writer.write_byte(static_cast<Byte>(unique_count & 0xFF));
    writer.write_byte(static_cast<Byte>((unique_count >> 8) & 0xFF));

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            writer.write_byte(static_cast<Byte>(i));
            uint32_t f = freqs[i];
            writer.write_byte(static_cast<Byte>(f & 0xFF));
            writer.write_byte(static_cast<Byte>((f >> 8) & 0xFF));
            writer.write_byte(static_cast<Byte>((f >> 16) & 0xFF));
            writer.write_byte(static_cast<Byte>((f >> 24) & 0xFF));
        }
    }

    std::array<uint64_t, 257> cum{};
    cum[0] = 0;
    for (int i = 0; i < 256; ++i) {
        cum[i + 1] = cum[i] + freqs[i];
    }
    uint64_t total = cum[256];

    uint64_t low = 0;
    uint64_t high = ARITH_TOP;
    uint32_t pending_bits = 0;

    auto output_bits = [&writer, &pending_bits](bool bit) {
        writer.write_bit(bit);
        while (pending_bits > 0) {
            writer.write_bit(!bit);
            pending_bits--;
        }
    };

    for (size_t i = 0; i < length; ++i) {
        Byte c = data[i];
        uint64_t range = high - low + 1;
        high = low + (range * cum[c + 1]) / total - 1;
        low = low + (range * cum[c]) / total;

        for (;;) {
            if (high < ARITH_HALF) {
                output_bits(false);
            } else if (low >= ARITH_HALF) {
                output_bits(true);
                low -= ARITH_HALF;
                high -= ARITH_HALF;
            } else if (low >= ARITH_FIRST_QTR && high < ARITH_THIRD_QTR) {
                pending_bits++;
                low -= ARITH_FIRST_QTR;
                high -= ARITH_FIRST_QTR;
            } else {
                break;
            }
            low = (low << 1) & ARITH_TOP;
            high = ((high << 1) | 1) & ARITH_TOP;
        }
    }

    // Flush remaining state
    pending_bits++;
    if (low < ARITH_FIRST_QTR) {
        output_bits(false);
    } else {
        output_bits(true);
    }
}

static void decompress_block(BitReader& reader, size_t length, ByteBuffer& output) {
    if (length == 0) return;

    // Read unique symbol count
    uint16_t unique_count = static_cast<uint16_t>(reader.read_byte()) |
                           (static_cast<uint16_t>(reader.read_byte()) << 8);

    std::array<uint32_t, 256> freqs{};
    for (uint16_t i = 0; i < unique_count; ++i) {
        Byte sym = reader.read_byte();
        uint32_t f = static_cast<uint32_t>(reader.read_byte()) |
                    (static_cast<uint32_t>(reader.read_byte()) << 8) |
                    (static_cast<uint32_t>(reader.read_byte()) << 16) |
                    (static_cast<uint32_t>(reader.read_byte()) << 24);
        freqs[sym] = f;
    }

    std::array<uint64_t, 257> cum{};
    cum[0] = 0;
    for (int i = 0; i < 256; ++i) {
        cum[i + 1] = cum[i] + freqs[i];
    }
    uint64_t total = cum[256];

    if (total == 0) return;

    // Read initial 32 bits into value
    uint64_t value = 0;
    for (int i = 0; i < 32; ++i) {
        value = (value << 1) | (reader.has_more() ? (reader.read_bit() ? 1 : 0) : 0);
    }

    uint64_t low = 0;
    uint64_t high = ARITH_TOP;

    for (size_t i = 0; i < length; ++i) {
        uint64_t range = high - low + 1;
        uint64_t scaled = (((value - low + 1) * total) - 1) / range;

        // Binary search for symbol
        int sym = 0;
        int l = 0, r = 255;
        while (l <= r) {
            int mid = l + (r - l) / 2;
            if (cum[mid + 1] <= scaled) {
                l = mid + 1;
            } else if (cum[mid] > scaled) {
                r = mid - 1;
            } else {
                sym = mid;
                break;
            }
        }

        output.push_back(static_cast<Byte>(sym));

        high = low + (range * cum[sym + 1]) / total - 1;
        low = low + (range * cum[sym]) / total;

        for (;;) {
            if (high < ARITH_HALF) {
                // do nothing
            } else if (low >= ARITH_HALF) {
                value -= ARITH_HALF;
                low -= ARITH_HALF;
                high -= ARITH_HALF;
            } else if (low >= ARITH_FIRST_QTR && high < ARITH_THIRD_QTR) {
                value -= ARITH_FIRST_QTR;
                low -= ARITH_FIRST_QTR;
                high -= ARITH_FIRST_QTR;
            } else {
                break;
            }
            low = (low << 1) & ARITH_TOP;
            high = ((high << 1) | 1) & ARITH_TOP;
            value = ((value << 1) | (reader.has_more() ? (reader.read_bit() ? 1 : 0) : 0)) & ARITH_TOP;
        }
    }
}

ByteBuffer ArithmeticCodec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    BitWriter writer;
    size_t offset = 0;
    while (offset < input.size()) {
        size_t chunk_len = std::min(ARITH_BLOCK_SIZE, input.size() - offset);
        compress_block(input.data() + offset, chunk_len, writer);
        offset += chunk_len;
    }

    return writer.take_buffer();
}

ByteBuffer ArithmeticCodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    BitReader reader(input);
    ByteBuffer output;
    output.reserve(original_size);

    size_t remaining = original_size;
    while (remaining > 0) {
        size_t chunk_len = std::min(ARITH_BLOCK_SIZE, remaining);
        decompress_block(reader, chunk_len, output);
        remaining -= chunk_len;
    }

    return output;
}

} // namespace icompress
