#include "icompress/bwt.hpp"
#include "icompress/rle.hpp"
#include <algorithm>
#include <numeric>
#include <array>
#include <stdexcept>

namespace icompress {

std::pair<ByteBuffer, uint32_t> BWTCodec::forward_transform(const ByteBuffer& block) {
    const size_t n = block.size();
    if (n == 0) return {{}, 0};
    if (n == 1) return {block, 0};

    // Vector of rotation indices
    std::vector<uint32_t> indices(n);
    std::iota(indices.begin(), indices.end(), 0);

    // Sort rotations lexicographically
    std::sort(indices.begin(), indices.end(), [&block, n](uint32_t a, uint32_t b) {
        if (a == b) return false;
        for (size_t k = 0; k < n; ++k) {
            Byte ca = block[(a + k) % n];
            Byte cb = block[(b + k) % n];
            if (ca != cb) {
                return ca < cb;
            }
        }
        return a < b;
    });

    ByteBuffer last_column(n);
    uint32_t primary_index = 0;

    for (size_t i = 0; i < n; ++i) {
        if (indices[i] == 0) {
            primary_index = static_cast<uint32_t>(i);
        }
        last_column[i] = block[(indices[i] + n - 1) % n];
    }

    return {last_column, primary_index};
}

ByteBuffer BWTCodec::inverse_transform(const ByteBuffer& last_column, uint32_t primary_index) {
    const size_t n = last_column.size();
    if (n == 0) return {};
    if (n == 1) return last_column;
    if (primary_index >= n) {
        throw std::runtime_error("Corrupted BWT primary index out of bounds");
    }

    // 1. Calculate symbol frequencies in L
    std::array<size_t, 256> count{};
    for (Byte b : last_column) {
        count[b]++;
    }

    // 2. Calculate starting index of each symbol in F (first_pos)
    std::array<size_t, 256> first_pos{};
    size_t total = 0;
    for (int i = 0; i < 256; ++i) {
        first_pos[i] = total;
        total += count[i];
    }

    // 3. Compute LF-mapping table
    // LF[i] = first_pos[L[i]] + occurrence_count(L[i] up to i)
    std::vector<uint32_t> lf(n);
    std::array<size_t, 256> occ{};
    for (size_t i = 0; i < n; ++i) {
        Byte c = last_column[i];
        lf[i] = static_cast<uint32_t>(first_pos[c] + occ[c]);
        occ[c]++;
    }

    // 4. Reconstruct original block backwards using LF-mapping
    ByteBuffer original(n);
    uint32_t current = primary_index;

    for (size_t step = 0; step < n; ++step) {
        size_t idx = n - 1 - step;
        original[idx] = last_column[current];
        current = lf[current];
    }

    return original;
}

ByteBuffer BWTCodec::compress(const ByteBuffer& input) {
    if (input.empty()) return {};

    ByteBuffer output;
    RLECodec rle;

    size_t offset = 0;
    while (offset < input.size()) {
        size_t chunk_len = std::min(BLOCK_SIZE, input.size() - offset);
        ByteBuffer block(input.begin() + offset, input.begin() + offset + chunk_len);

        auto [bwt_column, primary_idx] = forward_transform(block);
        ByteBuffer rle_packed = rle.compress(bwt_column);

        // Header for chunk:
        // Chunk uncompressed size: 4 bytes
        // Primary index: 4 bytes
        // RLE compressed size: 4 bytes
        uint32_t u_size = static_cast<uint32_t>(chunk_len);
        uint32_t rle_size = static_cast<uint32_t>(rle_packed.size());

        for (int i = 0; i < 4; ++i) output.push_back(static_cast<Byte>((u_size >> (i * 8)) & 0xFF));
        for (int i = 0; i < 4; ++i) output.push_back(static_cast<Byte>((primary_idx >> (i * 8)) & 0xFF));
        for (int i = 0; i < 4; ++i) output.push_back(static_cast<Byte>((rle_size >> (i * 8)) & 0xFF));

        output.insert(output.end(), rle_packed.begin(), rle_packed.end());
        offset += chunk_len;
    }

    return output;
}

ByteBuffer BWTCodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) return {};

    ByteBuffer output;
    output.reserve(original_size);
    RLECodec rle;

    size_t offset = 0;
    while (offset < input.size()) {
        if (offset + 12 > input.size()) {
            throw std::runtime_error("Corrupted BWT chunk header");
        }

        uint32_t u_size = 0;
        uint32_t primary_idx = 0;
        uint32_t rle_size = 0;

        for (int i = 0; i < 4; ++i) u_size |= (static_cast<uint32_t>(input[offset + i]) << (i * 8));
        for (int i = 0; i < 4; ++i) primary_idx |= (static_cast<uint32_t>(input[offset + 4 + i]) << (i * 8));
        for (int i = 0; i < 4; ++i) rle_size |= (static_cast<uint32_t>(input[offset + 8 + i]) << (i * 8));

        offset += 12;

        if (offset + rle_size > input.size()) {
            throw std::runtime_error("Corrupted BWT chunk payload truncated");
        }

        ByteBuffer rle_packed(input.begin() + offset, input.begin() + offset + rle_size);
        offset += rle_size;

        ByteBuffer bwt_column = rle.decompress(rle_packed, u_size);
        ByteBuffer block = inverse_transform(bwt_column, primary_idx);

        output.insert(output.end(), block.begin(), block.end());
    }

    if (output.size() != original_size) {
        throw std::runtime_error("BWT decompressed size mismatch");
    }

    return output;
}

} // namespace icompress
