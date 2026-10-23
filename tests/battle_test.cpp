#include "icompress/benchmark.hpp"
#include "icompress/crc32.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

using namespace icompress;

// Generates synthetic structured text (source code / documentation style)
static ByteBuffer generate_text_sample(size_t size) {
    std::string base = "int main(int argc, char* argv[]) {\n"
                       "    // iCompress high-performance data processing pipeline\n"
                       "    for (int i = 0; i < 100; ++i) {\n"
                       "        std::cout << \"Processing block index: \" << i << std::endl;\n"
                       "    }\n"
                       "    return 0;\n"
                       "}\n";
    ByteBuffer buffer;
    buffer.reserve(size);
    while (buffer.size() < size) {
        size_t take = std::min(base.size(), size - buffer.size());
        buffer.insert(buffer.end(), base.begin(), base.begin() + take);
    }
    return buffer;
}

// Generates synthetic 16-bit PCM audio stream (44.1 kHz multi-tone harmonic waveform)
static ByteBuffer generate_audio_sample(size_t num_samples) {
    ByteBuffer buffer;
    buffer.reserve(num_samples * 2);
    for (size_t i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / 44100.0;
        double sample = 0.6 * std::sin(2.0 * 3.14159265 * 440.0 * t) +
                        0.4 * std::sin(2.0 * 3.14159265 * 880.0 * t);
        int16_t pcm = static_cast<int16_t>(sample * 32767.0);
        buffer.push_back(static_cast<Byte>(pcm & 0xFF));
        buffer.push_back(static_cast<Byte>((pcm >> 8) & 0xFF));
    }
    return buffer;
}

// Generates synthetic RGB image raster (gradients and color runs)
static ByteBuffer generate_image_sample(size_t width, size_t height) {
    ByteBuffer buffer;
    buffer.reserve(width * height * 3);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            Byte r = static_cast<Byte>((x * 255) / width);
            Byte g = static_cast<Byte>((y * 255) / height);
            Byte b = static_cast<Byte>(((x + y) * 255) / (width + height));
            // Add repeated color blocks (typical in UI and flat images)
            if (x > width / 2 && y > height / 2) {
                r = 42; g = 128; b = 255;
            }
            buffer.push_back(r);
            buffer.push_back(g);
            buffer.push_back(b);
        }
    }
    return buffer;
}

// Generates synthetic raw video frames (consecutive frames with static background + moving sprite)
static ByteBuffer generate_video_sample(size_t frames, size_t frame_size) {
    ByteBuffer buffer;
    buffer.reserve(frames * frame_size);

    // Static background
    std::vector<Byte> background(frame_size, 128);
    for (size_t i = 0; i < frame_size; ++i) {
        background[i] = static_cast<Byte>(i % 256);
    }

    for (size_t f = 0; f < frames; ++f) {
        std::vector<Byte> frame = background;
        // Moving foreground block (temporal correlation)
        size_t sprite_pos = (f * 100) % (frame_size - 64);
        for (size_t k = 0; k < 64; ++k) {
            frame[sprite_pos + k] = 255;
        }
        buffer.insert(buffer.end(), frame.begin(), frame.end());
    }
    return buffer;
}

int main() {
    std::cout << "\n========================================================================\n";
    std::cout << "        iCompress Battle-Test Suite: Text, Audio, Image, Video           \n";
    std::cout << "========================================================================\n";

    // 1. Text payload (16 KB)
    std::cout << "\n>>> 1. Battle-Testing against TEXT payload (Source code / Docs)...";
    ByteBuffer text_data = generate_text_sample(16384);
    auto text_stats = Benchmark::run(text_data);
    Benchmark::print_report("synthetic_source.txt", text_data.size(), text_stats);

    // 2. Audio payload (44.1 kHz 16-bit PCM, 16 KB)
    std::cout << "\n>>> 2. Battle-Testing against AUDIO payload (16-bit PCM waveform)...";
    ByteBuffer audio_data = generate_audio_sample(8000);
    auto audio_stats = Benchmark::run(audio_data);
    Benchmark::print_report("synthetic_tone.pcm", audio_data.size(), audio_stats);

    // 3. Image payload (80x60 RGB raster, 14.4 KB)
    std::cout << "\n>>> 3. Battle-Testing against IMAGE payload (RGB raster bitmap)...";
    ByteBuffer image_data = generate_image_sample(80, 60);
    auto image_stats = Benchmark::run(image_data);
    Benchmark::print_report("synthetic_raster.rgb", image_data.size(), image_stats);

    // 4. Video payload (4 frames of 4000 bytes raw frames, 16 KB)
    std::cout << "\n>>> 4. Battle-Testing against VIDEO payload (Consecutive YUV frames)...";
    ByteBuffer video_data = generate_video_sample(4, 4000);
    auto video_stats = Benchmark::run(video_data);
    Benchmark::print_report("synthetic_video.yuv", video_data.size(), video_stats);

    // Verify all tests passed without corruption
    bool all_passed = true;
    for (const auto& s : text_stats) if (!s.verified) all_passed = false;
    for (const auto& s : audio_stats) if (!s.verified) all_passed = false;
    for (const auto& s : image_stats) if (!s.verified) all_passed = false;
    for (const auto& s : video_stats) if (!s.verified) all_passed = false;

    if (all_passed) {
        std::cout << "\n>>> [VERIFIED] All 4 data modalities passed 100% bit-exact lossless verification!\n\n";
        return 0;
    } else {
        std::cerr << "\n>>> [FAILURE] One or more algorithms failed integrity verification!\n\n";
        return 1;
    }
}
