#include "icompress/benchmark.hpp"
#include "icompress/codec.hpp"
#include "icompress/crc32.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace icompress {

std::vector<CompressionStats> Benchmark::run(const ByteBuffer& data) {
    std::vector<Algorithm> algos = {
        Algorithm::Deflate,
        Algorithm::BWT_RLE,
        Algorithm::LZ77,
        Algorithm::LZ78,
        Algorithm::LZW,
        Algorithm::Arithmetic,
        Algorithm::Huffman,
        Algorithm::RLE
    };

    uint32_t original_crc = CRC32::calculate(data);
    std::vector<CompressionStats> results;

    for (Algorithm algo : algos) {
        CompressionStats stats;
        stats.algorithm = algo;
        stats.original_size = data.size();
        stats.crc32 = original_crc;

        try {
            auto codec = ICodec::create(algo);

            auto start = std::chrono::high_resolution_clock::now();
            ByteBuffer compressed = codec->compress(data);
            auto end = std::chrono::high_resolution_clock::now();

            stats.compressed_size = compressed.size();
            stats.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (data.size() > 0 && compressed.size() > 0) {
                stats.ratio = static_cast<double>(data.size()) / static_cast<double>(compressed.size());
                stats.space_savings = (1.0 - static_cast<double>(compressed.size()) / static_cast<double>(data.size())) * 100.0;
            } else {
                stats.ratio = 1.0;
                stats.space_savings = 0.0;
            }

            // Verify decompression
            ByteBuffer decompressed = codec->decompress(compressed, data.size());
            uint32_t decomp_crc = CRC32::calculate(decompressed);
            stats.verified = (decomp_crc == original_crc);
        } catch (const std::exception& e) {
            stats.compressed_size = 0;
            stats.ratio = 0.0;
            stats.space_savings = 0.0;
            stats.duration_ms = 0.0;
            stats.verified = false;
        }

        results.push_back(stats);
    }

    return results;
}

static std::string format_size(uint64_t bytes) {
    std::ostringstream ss;
    if (bytes < 1024) {
        ss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        ss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
    } else {
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    return ss.str();
}

void Benchmark::print_report(const std::string& filename, size_t file_size,
                             const std::vector<CompressionStats>& stats) {
    std::cout << "\n========================================================================================\n";
    std::cout << "  iCompress Battle Benchmark: " << filename << " (" << format_size(file_size) << ")\n";
    std::cout << "========================================================================================\n";
    std::cout << std::left
              << std::setw(14) << "Algorithm"
              << std::setw(16) << "Compressed"
              << std::setw(12) << "Ratio"
              << std::setw(12) << "Savings"
              << std::setw(14) << "Time (ms)"
              << std::setw(12) << "Integrity"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    const CompressionStats* best = nullptr;

    for (const auto& s : stats) {
        std::cout << std::left
                  << std::setw(14) << algorithm_name(s.algorithm)
                  << std::setw(16) << format_size(s.compressed_size)
                  << std::setw(12) << (std::to_string(s.ratio).substr(0, 4) + "x")
                  << std::setw(12) << (std::to_string(s.space_savings).substr(0, 5) + "%")
                  << std::setw(14) << std::fixed << std::setprecision(2) << s.duration_ms
                  << std::setw(12) << (s.verified ? "PASS (CRC)" : "FAIL")
                  << "\n";

        if (s.verified && (!best || s.compressed_size < best->compressed_size)) {
            best = &s;
        }
    }

    std::cout << "----------------------------------------------------------------------------------------\n";
    if (best) {
        std::cout << "  Optimal algorithm: " << algorithm_name(best->algorithm)
                  << " (" << format_size(best->compressed_size)
                  << ", " << std::fixed << std::setprecision(1) << best->space_savings << "% reduction)\n";
    }
    std::cout << "========================================================================================\n\n";
}

} // namespace icompress
