#pragma once

#include "icompress/types.hpp"
#include <vector>
#include <string>

namespace icompress {

class Benchmark {
public:
    static std::vector<CompressionStats> run(const ByteBuffer& data);
    static void print_report(const std::string& filename, size_t file_size,
                            const std::vector<CompressionStats>& stats);
};

} // namespace icompress
