#include "icompress/lz78.hpp"
#include <map>
#include <stdexcept>

namespace icompress {

std::vector<LZ78Token> LZ78Codec::tokenize(const ByteBuffer& input) const {
    std::vector<LZ78Token> tokens;
    if (input.empty()) return tokens;

    // Map: (parent_node, byte) -> child_node_id
    std::map<std::pair<uint32_t, Byte>, uint32_t> trie;
    uint32_t next_id = 1;
    uint32_t current_node = 0;

    for (size_t i = 0; i < input.size(); ++i) {
        Byte b = input[i];
        auto key = std::make_pair(current_node, b);
        auto it = trie.find(key);

        if (it != trie.end()) {
            current_node = it->second;
        } else {
            trie[key] = next_id++;
            LZ78Token token;
            token.dict_index = current_node;
            token.next_char = b;
            token.has_next = true;
            tokens.push_back(token);
            current_node = 0;
        }
    }

    if (current_node != 0) {
        LZ78Token token;
        token.dict_index = current_node;
        token.next_char = 0;
        token.has_next = false;
        tokens.push_back(token);
    }

    return tokens;
}

ByteBuffer LZ78Codec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    auto tokens = tokenize(input);
    ByteBuffer output;
    output.reserve(tokens.size() * 6);

    // Number of tokens: 4 bytes
    uint32_t count = static_cast<uint32_t>(tokens.size());
    for (int i = 0; i < 4; ++i) {
        output.push_back(static_cast<Byte>((count >> (i * 8)) & 0xFF));
    }

    // Each token: 4 bytes dict_index, 1 byte flags (0x01 = has_next), 1 byte next_char
    for (const auto& t : tokens) {
        for (int i = 0; i < 4; ++i) {
            output.push_back(static_cast<Byte>((t.dict_index >> (i * 8)) & 0xFF));
        }
        output.push_back(t.has_next ? 1 : 0);
        output.push_back(t.next_char);
    }

    return output;
}

ByteBuffer LZ78Codec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    if (input.size() < 4) {
        throw std::runtime_error("Corrupted LZ78 stream: missing header");
    }

    uint32_t count = static_cast<uint32_t>(input[0]) |
                    (static_cast<uint32_t>(input[1]) << 8) |
                    (static_cast<uint32_t>(input[2]) << 16) |
                    (static_cast<uint32_t>(input[3]) << 24);

    if (input.size() < 4 + count * 6) {
        throw std::runtime_error("Truncated LZ78 stream");
    }

    std::vector<ByteBuffer> dictionary;
    dictionary.push_back({}); // Index 0 is empty string

    ByteBuffer output;
    output.reserve(original_size);

    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t idx = static_cast<uint32_t>(input[offset]) |
                      (static_cast<uint32_t>(input[offset + 1]) << 8) |
                      (static_cast<uint32_t>(input[offset + 2]) << 16) |
                      (static_cast<uint32_t>(input[offset + 3]) << 24);
        bool has_next = (input[offset + 4] != 0);
        Byte next_ch = input[offset + 5];
        offset += 6;

        if (idx >= dictionary.size()) {
            throw std::runtime_error("Invalid LZ78 dictionary index reference");
        }

        ByteBuffer phrase = dictionary[idx];
        if (has_next) {
            phrase.push_back(next_ch);
            dictionary.push_back(phrase);
        }

        output.insert(output.end(), phrase.begin(), phrase.end());
    }

    if (output.size() != original_size) {
        throw std::runtime_error("LZ78 decompressed size mismatch");
    }

    return output;
}

} // namespace icompress
