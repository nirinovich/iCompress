#include "icompress/rle.hpp"

namespace icompress {

ByteBuffer RLECodec::compress(const ByteBuffer& input) {
    if (input.empty()) {
        return {};
    }

    ByteBuffer output;
    output.reserve(input.size());

    size_t i = 0;
    while (i < input.size()) {
        Byte current = input[i];
        size_t run_length = 1;
        while (i + run_length < input.size() && input[i + run_length] == current && run_length < 255) {
            run_length++;
        }

        output.push_back(static_cast<Byte>(run_length));
        output.push_back(current);

        i += run_length;
    }

    return output;
}

ByteBuffer RLECodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) {
        return {};
    }

    if (input.size() % 2 != 0) {
        throw std::runtime_error("Corrupted RLE stream: odd number of bytes");
    }

    ByteBuffer output;
    output.reserve(original_size);

    for (size_t i = 0; i < input.size(); i += 2) {
        Byte count = input[i];
        Byte value = input[i + 1];
        output.insert(output.end(), count, value);
    }

    if (output.size() != original_size) {
        throw std::runtime_error("RLE decompressed size mismatch: expected " +
                                 std::to_string(original_size) + " but got " +
                                 std::to_string(output.size()));
    }

    return output;
}

} // namespace icompress
