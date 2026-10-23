#include "icompress/huffman.hpp"
#include "icompress/bitstream.hpp"
#include <queue>
#include <map>

namespace icompress {

struct HuffmanBuilderNode {
    int symbol{-1}; // -1 for internal
    uint64_t frequency{0};
    uint32_t creation_id{0};
    std::shared_ptr<HuffmanBuilderNode> left{nullptr};
    std::shared_ptr<HuffmanBuilderNode> right{nullptr};

    bool is_leaf() const { return left == nullptr && right == nullptr; }
};

struct NodeComparator {
    bool operator()(const std::shared_ptr<HuffmanBuilderNode>& a,
                    const std::shared_ptr<HuffmanBuilderNode>& b) const {
        if (a->frequency != b->frequency) {
            return a->frequency > b->frequency; // lowest frequency first
        }
        return a->creation_id > b->creation_id; // deterministic tie-break
    }
};

void build_code_table(const std::shared_ptr<HuffmanBuilderNode>& node,
                      const std::string& prefix,
                      std::array<std::string, 256>& table) {
    if (!node) return;
    if (node->is_leaf()) {
        table[static_cast<Byte>(node->symbol)] = prefix.empty() ? "0" : prefix;
        return;
    }
    build_code_table(node->left, prefix + "0", table);
    build_code_table(node->right, prefix + "1", table);
}

ByteBuffer HuffmanCodec::compress(const ByteBuffer& input) {
    if (input.empty()) {
        return {};
    }

    // 1. Calculate frequencies
    std::array<uint32_t, 256> freqs{};
    for (Byte b : input) {
        freqs[b]++;
    }

    // 2. Build Huffman tree with deterministic priority queue
    uint32_t node_counter = 0;
    std::priority_queue<std::shared_ptr<HuffmanBuilderNode>,
                        std::vector<std::shared_ptr<HuffmanBuilderNode>>,
                        NodeComparator> pq;

    uint16_t unique_symbols = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            auto node = std::make_shared<HuffmanBuilderNode>();
            node->symbol = i;
            node->frequency = freqs[i];
            node->creation_id = node_counter++;
            pq.push(node);
            unique_symbols++;
        }
    }

    if (pq.empty()) {
        return {};
    }

    std::shared_ptr<HuffmanBuilderNode> root;
    if (pq.size() == 1) {
        root = pq.top();
    } else {
        while (pq.size() > 1) {
            auto left = pq.top(); pq.pop();
            auto right = pq.top(); pq.pop();

            auto parent = std::make_shared<HuffmanBuilderNode>();
            parent->symbol = -1;
            parent->frequency = left->frequency + right->frequency;
            parent->creation_id = node_counter++;
            parent->left = left;
            parent->right = right;
            pq.push(parent);
        }
        root = pq.top();
    }

    // 3. Generate codes
    std::array<std::string, 256> table;
    build_code_table(root, "", table);

    // 4. Serialize header (frequency table) + bitstream
    BitWriter writer;

    // Store number of unique symbols: 2 bytes
    writer.write_byte(static_cast<Byte>(unique_symbols & 0xFF));
    writer.write_byte(static_cast<Byte>((unique_symbols >> 8) & 0xFF));

    // For each unique symbol: 1 byte for symbol, 4 bytes for frequency
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

    // Encode data
    for (Byte b : input) {
        const std::string& code = table[b];
        for (char c : code) {
            writer.write_bit(c == '1');
        }
    }

    return writer.take_buffer();
}

ByteBuffer HuffmanCodec::decompress(const ByteBuffer& input, size_t original_size) {
    if (original_size == 0 || input.empty()) {
        return {};
    }

    BitReader reader(input);

    // Read unique symbols count
    uint16_t unique_symbols = static_cast<uint16_t>(reader.read_byte()) |
                             (static_cast<uint16_t>(reader.read_byte()) << 8);

    std::array<uint32_t, 256> freqs{};
    for (uint16_t i = 0; i < unique_symbols; ++i) {
        Byte sym = reader.read_byte();
        uint32_t f = static_cast<uint32_t>(reader.read_byte()) |
                    (static_cast<uint32_t>(reader.read_byte()) << 8) |
                    (static_cast<uint32_t>(reader.read_byte()) << 16) |
                    (static_cast<uint32_t>(reader.read_byte()) << 24);
        freqs[sym] = f;
    }

    // Reconstruct tree
    uint32_t node_counter = 0;
    std::priority_queue<std::shared_ptr<HuffmanBuilderNode>,
                        std::vector<std::shared_ptr<HuffmanBuilderNode>>,
                        NodeComparator> pq;

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            auto node = std::make_shared<HuffmanBuilderNode>();
            node->symbol = i;
            node->frequency = freqs[i];
            node->creation_id = node_counter++;
            pq.push(node);
        }
    }

    if (pq.empty()) {
        return {};
    }

    std::shared_ptr<HuffmanBuilderNode> root;
    if (pq.size() == 1) {
        root = pq.top();
    } else {
        while (pq.size() > 1) {
            auto left = pq.top(); pq.pop();
            auto right = pq.top(); pq.pop();

            auto parent = std::make_shared<HuffmanBuilderNode>();
            parent->symbol = -1;
            parent->frequency = left->frequency + right->frequency;
            parent->creation_id = node_counter++;
            parent->left = left;
            parent->right = right;
            pq.push(parent);
        }
        root = pq.top();
    }

    // Decode bitstream
    ByteBuffer output;
    output.reserve(original_size);

    if (root->is_leaf()) {
        // All symbols are identical
        output.assign(original_size, static_cast<Byte>(root->symbol));
        return output;
    }

    auto current = root;
    while (output.size() < original_size) {
        bool bit = reader.read_bit();
        current = bit ? current->right : current->left;
        if (!current) {
            throw std::runtime_error("Corrupted Huffman bitstream: invalid tree traversal");
        }
        if (current->is_leaf()) {
            output.push_back(static_cast<Byte>(current->symbol));
            current = root;
        }
    }

    return output;
}

} // namespace icompress
