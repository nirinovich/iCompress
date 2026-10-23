#include "icompress/lzw.hpp"
#include <map>
#include <stdexcept>

namespace icompress {

ByteBuffer LZWCodec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    // Map: (prefix_code, next_byte) -> code
    std::map<std::pair<uint16_t, Byte>, uint16_t> dict;
    uint16_t next_code = 256;

    std::vector<uint16_t> codes;
    uint16_t current = input[0];

    for (size_t i = 1; i < input.size(); ++i) {
        Byte b = input[i];
        auto key = std::make_pair(current, b);
        auto it = dict.find(key);

        if (it != dict.end()) {
            current = it->second;
        } else {
            codes.push_back(current);
            if (next_code < MAX_DICT_ENTRIES) {
                dict[key] = next_code++;
            }
            current = b;
        }
    }
    codes.push_back(current);

    // Serialize codes as 16-bit little-endian
    ByteBuffer output;
    output.reserve(codes.size() * 2);
    for (uint16_t c : codes) {
        output.push_back(static_cast<Byte>(c & 0xFF));
        output.push_back(static_cast<Byte>((c >> 8) & 0xFF));
    }

    return output;
}

ByteBuffer LZWCodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    if (input.size() % 2 != 0) {
        throw std::runtime_error("Corrupted LZW stream: odd byte count");
    }

    size_t num_codes = input.size() / 2;
    std::vector<uint16_t> codes(num_codes);
    for (size_t i = 0; i < num_codes; ++i) {
        codes[i] = static_cast<uint16_t>(input[i * 2]) |
                  (static_cast<uint16_t>(input[i * 2 + 1]) << 8);
    }

    // Initialize dictionary with 256 single-byte entries
    std::vector<ByteBuffer> dict(256);
    for (int i = 0; i < 256; ++i) {
        dict[i] = {static_cast<Byte>(i)};
    }

    ByteBuffer output;
    output.reserve(original_size);

    uint16_t prev_code = codes[0];
    if (prev_code >= dict.size()) {
        throw std::runtime_error("Invalid initial LZW code");
    }

    output.insert(output.end(), dict[prev_code].begin(), dict[prev_code].end());

    for (size_t i = 1; i < num_codes; ++i) {
        uint16_t curr_code = codes[i];
        ByteBuffer entry;

        if (curr_code < dict.size()) {
            entry = dict[curr_code];
        } else if (curr_code == dict.size()) {
            // Special KwKwK case
            entry = dict[prev_code];
            entry.push_back(dict[prev_code][0]);
        } else {
            throw std::runtime_error("Corrupted LZW code out of dictionary bounds");
        }

        output.insert(output.end(), entry.begin(), entry.end());

        if (dict.size() < MAX_DICT_ENTRIES) {
            ByteBuffer new_entry = dict[prev_code];
            new_entry.push_back(entry[0]);
            dict.push_back(new_entry);
        }

        prev_code = curr_code;
    }

    if (output.size() != original_size) {
        throw std::runtime_error("LZW decompressed size mismatch");
    }

    return output;
}

} // namespace icompress
