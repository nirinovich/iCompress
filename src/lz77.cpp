#include "icompress/lz77.hpp"
#include <algorithm>
#include <stdexcept>

namespace icompress {

LZ77Codec::LZ77Codec(uint16_t window_size, uint16_t lookahead_size)
    : window_size_(window_size), lookahead_size_(lookahead_size) {}

std::vector<LZ77Token> LZ77Codec::tokenize(const ByteBuffer& input) const {
    std::vector<LZ77Token> tokens;
    const size_t n = input.size();
    size_t cursor = 0;

    while (cursor < n) {
        size_t search_start = (cursor > window_size_) ? cursor - window_size_ : 0;
        size_t max_match_len = std::min(static_cast<size_t>(lookahead_size_), n - cursor);

        size_t best_dist = 0;
        size_t best_len = 0;

        // Search for longest match in sliding search buffer (preferring closest match on tie)
        for (size_t s = search_start; s < cursor; ++s) {
            size_t match_len = 0;
            while (match_len < max_match_len && input[s + match_len] == input[cursor + match_len]) {
                match_len++;
            }

            size_t dist = cursor - s;
            if (match_len > best_len || (match_len == best_len && match_len > 0 && dist < best_dist)) {
                best_len = match_len;
                best_dist = dist;
            }
        }

        // If best match covers the remainder of the file exactly
        if (best_len > 0 && cursor + best_len == n) {
            LZ77Token token;
            token.distance = static_cast<uint16_t>(best_dist);
            token.length = static_cast<uint16_t>(best_len);
            token.next_char = 0; // No next char
            tokens.push_back(token);
            cursor += best_len;
            break;
        }

        if (best_len > 0) {
            LZ77Token token;
            token.distance = static_cast<uint16_t>(best_dist);
            token.length = static_cast<uint16_t>(best_len);
            token.next_char = input[cursor + best_len];
            tokens.push_back(token);
            cursor += best_len + 1;
        } else {
            LZ77Token token;
            token.distance = 0;
            token.length = 0;
            token.next_char = input[cursor];
            tokens.push_back(token);
            cursor += 1;
        }
    }

    return tokens;
}

ByteBuffer LZ77Codec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    auto tokens = tokenize(input);
    ByteBuffer output;
    output.reserve(tokens.size() * 5);

    // Number of tokens: 4 bytes
    uint32_t num_tokens = static_cast<uint32_t>(tokens.size());
    for (int i = 0; i < 4; ++i) {
        output.push_back(static_cast<Byte>((num_tokens >> (i * 8)) & 0xFF));
    }

    // Write tokens: 2 bytes distance, 2 bytes length, 1 byte next_char
    for (const auto& t : tokens) {
        output.push_back(static_cast<Byte>(t.distance & 0xFF));
        output.push_back(static_cast<Byte>((t.distance >> 8) & 0xFF));
        output.push_back(static_cast<Byte>(t.length & 0xFF));
        output.push_back(static_cast<Byte>((t.length >> 8) & 0xFF));
        output.push_back(t.next_char);
    }

    return output;
}

ByteBuffer LZ77Codec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    if (input.size() < 4) {
        throw std::runtime_error("Corrupted LZ77 stream header");
    }

    uint32_t num_tokens = static_cast<uint32_t>(input[0]) |
                         (static_cast<uint32_t>(input[1]) << 8) |
                         (static_cast<uint32_t>(input[2]) << 16) |
                         (static_cast<uint32_t>(input[3]) << 24);

    if (input.size() < 4 + num_tokens * 5) {
        throw std::runtime_error("Truncated LZ77 stream");
    }

    ByteBuffer output;
    output.reserve(original_size);

    size_t offset = 4;
    for (uint32_t i = 0; i < num_tokens; ++i) {
        uint16_t dist = static_cast<uint16_t>(input[offset]) |
                       (static_cast<uint16_t>(input[offset + 1]) << 8);
        uint16_t len = static_cast<uint16_t>(input[offset + 2]) |
                      (static_cast<uint16_t>(input[offset + 3]) << 8);
        Byte next_ch = input[offset + 4];
        offset += 5;

        if (len > 0) {
            if (dist == 0 || dist > output.size()) {
                throw std::runtime_error("Invalid LZ77 backward distance");
            }
            size_t copy_pos = output.size() - dist;
            for (uint16_t k = 0; k < len; ++k) {
                output.push_back(output[copy_pos + k]);
            }
        }

        if (output.size() < original_size) {
            output.push_back(next_ch);
        }
    }

    if (output.size() != original_size) {
        throw std::runtime_error("LZ77 decompressed size mismatch");
    }

    return output;
}

} // namespace icompress
