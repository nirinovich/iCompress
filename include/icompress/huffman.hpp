#pragma once

#include "icompress/codec.hpp"
#include <array>
#include <queue>
#include <memory>

namespace icompress {

struct HuffmanNode {
    Byte symbol{0};
    uint64_t frequency{0};
    std::shared_ptr<HuffmanNode> left{nullptr};
    std::shared_ptr<HuffmanNode> right{nullptr};

    bool is_leaf() const { return !left && !right; }
};

struct CompareHuffmanNode {
    bool operator()(const std::shared_ptr<HuffmanNode>& a, const std::shared_ptr<HuffmanNode>& b) const {
        return a->frequency > b->frequency;
    }
};

class HuffmanCodec : public ICodec {
public:
    ByteBuffer compress(const ByteBuffer& input) override;
    ByteBuffer decompress(const ByteBuffer& input, size_t original_size) override;
    Algorithm algorithm() const override { return Algorithm::Huffman; }

private:
    static void build_codes(const std::shared_ptr<HuffmanNode>& node, const std::string& current_code,
                            std::array<std::string, 256>& code_table);
};

} // namespace icompress
