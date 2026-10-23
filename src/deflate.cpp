#include "icompress/deflate.hpp"
#include "icompress/huffman.hpp"
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace icompress {

DeflateCodec::DeflateCodec(uint16_t window_size, uint16_t max_match)
    : window_size_(window_size), max_match_(max_match) {}

ByteBuffer DeflateCodec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    const size_t n = input.size();
    ByteBuffer intermediate;
    intermediate.reserve(n);

    // Hash table for 3-byte prefixes
    constexpr size_t HASH_SIZE = 65536;
    std::vector<int> head(HASH_SIZE, -1);
    std::vector<int> prev(n, -1);

    auto hash3 = [](Byte a, Byte b, Byte c) -> size_t {
        return ((static_cast<size_t>(a) << 8) ^ (static_cast<size_t>(b) << 4) ^ static_cast<size_t>(c)) % HASH_SIZE;
    };

    size_t cursor = 0;
    while (cursor < n) {
        size_t best_dist = 0;
        size_t best_len = 0;

        if (cursor + 3 <= n) {
            size_t h = hash3(input[cursor], input[cursor + 1], input[cursor + 2]);
            int candidate = head[h];
            head[h] = static_cast<int>(cursor);

            // Search back along the hash chain
            size_t search_limit = (cursor > window_size_) ? cursor - window_size_ : 0;
            int chain_depth = 0;

            while (candidate != -1 && static_cast<size_t>(candidate) >= search_limit && chain_depth < 64) {
                size_t s = static_cast<size_t>(candidate);
                size_t match_len = 0;
                size_t max_len = std::min(static_cast<size_t>(max_match_), n - cursor);

                while (match_len < max_len && input[s + match_len] == input[cursor + match_len]) {
                    match_len++;
                }

                if (match_len > best_len) {
                    best_len = match_len;
                    best_dist = cursor - s;
                    if (best_len == max_len) break;
                }

                candidate = prev[candidate];
                chain_depth++;
            }
        }

        // Minimum match threshold is 3 bytes
        if (best_len >= 3) {
            // Match token: 1 (flag), 2 bytes distance, 2 bytes length
            intermediate.push_back(1);
            intermediate.push_back(static_cast<Byte>(best_dist & 0xFF));
            intermediate.push_back(static_cast<Byte>((best_dist >> 8) & 0xFF));
            intermediate.push_back(static_cast<Byte>(best_len & 0xFF));
            intermediate.push_back(static_cast<Byte>((best_len >> 8) & 0xFF));

            // Update hash chain for consumed positions
            for (size_t k = 1; k < best_len && (cursor + k + 2 < n); ++k) {
                size_t hk = hash3(input[cursor + k], input[cursor + k + 1], input[cursor + k + 2]);
                prev[cursor + k] = head[hk];
                head[hk] = static_cast<int>(cursor + k);
            }
            cursor += best_len;
        } else {
            // Literal token: 0 (flag), byte value
            intermediate.push_back(0);
            intermediate.push_back(input[cursor]);
            cursor += 1;
        }
    }

    // Step 2: Huffman entropy encode the intermediate token stream
    HuffmanCodec huffman;
    ByteBuffer compressed = huffman.compress(intermediate);

    // Prefix with intermediate uncompressed size (4 bytes)
    ByteBuffer output;
    uint32_t inter_size = static_cast<uint32_t>(intermediate.size());
    for (int i = 0; i < 4; ++i) {
        output.push_back(static_cast<Byte>((inter_size >> (i * 8)) & 0xFF));
    }
    output.insert(output.end(), compressed.begin(), compressed.end());
    return output;
}

ByteBuffer DeflateCodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    if (input.size() < 4) {
        throw std::runtime_error("Corrupted DEFLATE stream header");
    }

    uint32_t inter_size = static_cast<uint32_t>(input[0]) |
                         (static_cast<uint32_t>(input[1]) << 8) |
                         (static_cast<uint32_t>(input[2]) << 16) |
                         (static_cast<uint32_t>(input[3]) << 24);

    ByteBuffer huff_payload(input.begin() + 4, input.end());

    // Step 1: Invert Huffman entropy layer
    HuffmanCodec huffman;
    ByteBuffer intermediate = huffman.decompress(huff_payload, inter_size);

    // Step 2: Replay LZ77 tokens
    ByteBuffer output;
    output.reserve(original_size);

    size_t cursor = 0;
    while (cursor < intermediate.size()) {
        Byte flag = intermediate[cursor++];
        if (flag == 0) {
            if (cursor >= intermediate.size()) {
                throw std::runtime_error("Truncated DEFLATE literal token");
            }
            output.push_back(intermediate[cursor++]);
        } else if (flag == 1) {
            if (cursor + 4 > intermediate.size()) {
                throw std::runtime_error("Truncated DEFLATE match token");
            }
            uint16_t dist = static_cast<uint16_t>(intermediate[cursor]) |
                           (static_cast<uint16_t>(intermediate[cursor + 1]) << 8);
            uint16_t len = static_cast<uint16_t>(intermediate[cursor + 2]) |
                          (static_cast<uint16_t>(intermediate[cursor + 3]) << 8);
            cursor += 4;

            if (dist == 0 || dist > output.size()) {
                throw std::runtime_error("Corrupted DEFLATE distance out of bounds");
            }

            size_t copy_pos = output.size() - dist;
            for (uint16_t k = 0; k < len; ++k) {
                output.push_back(output[copy_pos + k]);
            }
        } else {
            throw std::runtime_error("Invalid DEFLATE token flag");
        }
    }

    if (output.size() != original_size) {
        throw std::runtime_error("DEFLATE decompressed size mismatch: expected " +
                                 std::to_string(original_size) + " but got " +
                                 std::to_string(output.size()));
    }

    return output;
}

} // namespace icompress
