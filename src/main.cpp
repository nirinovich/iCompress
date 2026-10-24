#include "icompress/container.hpp"
#include "icompress/codec.hpp"
#include "icompress/crc32.hpp"
#include "icompress/benchmark.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace fs = std::filesystem;
using namespace icompress;

#ifdef _WIN32
static bool set_reg_str(HKEY root, const std::wstring& subkey, const std::wstring& name, const std::wstring& val) {
    HKEY hKey;
    if (RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, name.empty() ? nullptr : name.c_str(), 0, REG_SZ, 
                       reinterpret_cast<const BYTE*>(val.c_str()), static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

static int do_register_shell() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path binDir = fs::path(exePath).parent_path();
    fs::path guiPath = binDir / "icompress-gui.exe";
    if (!fs::exists(guiPath)) {
        guiPath = fs::path(exePath);
    }

    std::wstring cliCmd = L"\"" + std::wstring(exePath) + L"\" compress \"%1\"";
    std::wstring guiCmd = L"\"" + guiPath.wstring() + L"\" \"%1\"";
    std::wstring iconVal = L"\"" + guiPath.wstring() + L"\",0";

    // 1. Context Menu for all files
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\iCompress", L"", L"Compress with iCompress");
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\iCompress", L"Icon", iconVal);
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\iCompress\\command", L"", cliCmd);

    // 2. Context Menu for directories
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\iCompress", L"", L"Compress with iCompress");
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\iCompress", L"Icon", iconVal);
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\iCompress\\command", L"", cliCmd);

    // 3. File Association for .ic
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\.ic", L"", L"iCompress.Archive");
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive", L"", L"iCompress Lossless Archive");
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive\\DefaultIcon", L"", iconVal);
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive\\shell\\open\\command", L"", guiCmd);
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive\\shell\\extract", L"", L"Extract Here");
    set_reg_str(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive\\shell\\extract\\command", L"", 
                L"\"" + std::wstring(exePath) + L"\" extract \"%1\"");

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    std::cout << "[SUCCESS] Registered iCompress in Windows Explorer context menu & .ic file association.\n";
    std::cout << "          - Right-click any file/folder -> 'Compress with iCompress'\n";
    std::cout << "          - Double-click any .ic file -> Opens in iCompress Desktop Studio\n";
    return 0;
}

static int do_unregister_shell() {
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\iCompress");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\iCompress");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.ic");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\iCompress.Archive");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    std::cout << "[SUCCESS] Unregistered iCompress Windows Explorer integrations.\n";
    return 0;
}
#endif

static void print_usage(const char* prog) {
    std::cout << "iCompress — High-efficiency lossless data archiver\n\n"
              << "Usage:\n"
              << "  " << prog << " [command] [options] <file>\n\n"
              << "Commands:\n"
              << "  compress, -c <file>     Compress file into an .ic archive\n"
              << "  extract,  -d <file.ic>  Extract an .ic archive\n"
              << "  list,     -l <file.ic>  Display archive metadata and header details\n"
              << "  test,     -t <file.ic>  Verify archive data integrity (CRC32)\n"
              << "  benchmark,-b <file>     Run battle benchmark across all 8 reference codecs\n"
              << "  ui,       --gui         Launch the interactive desktop interface\n"
              << "  register, --register    Register Windows Explorer context menu & .ic file association\n"
              << "  unregister, --unregister Remove Windows Explorer integrations\n\n"
              << "Options:\n"
              << "  -o <path>               Output file path\n"
              << "  -m <method>             Algorithm: auto (default), deflate, bwt, arithmetic,\n"
              << "                          lz77, lz78, lzw, huffman, rle\n"
              << "  -v, --verbose           Display detailed execution metrics\n"
              << "  -h, --help              Show help information\n"
              << "  -V, --version           Display version information\n\n"
              << "Examples:\n"
              << "  " << prog << " compress video.yuv -o video.ic -m deflate\n"
              << "  " << prog << " extract video.ic -o restored.yuv\n"
              << "  " << prog << " benchmark audio.wav\n";
}

static void print_version() {
    std::cout << "iCompress version 1.0.0 (x86_64-pc-windows)\n"
              << "Copyright (C) 2026. Built with modern C++17.\n";
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

static int do_compress(const std::string& input_path, const std::string& custom_out,
                       Algorithm algo, bool verbose) {
    std::string out_path = custom_out;
    if (out_path.empty()) {
        out_path = input_path + ".ic";
    }

    ByteBuffer raw_data = Container::read_file(input_path);
    uint32_t raw_crc = CRC32::calculate(raw_data);

    if (verbose) {
        std::cout << "[iCompress] Compressing '" << input_path << "' using "
                  << algorithm_name(algo) << " (" << format_size(raw_data.size()) << ")...\n";
    }

    auto codec = ICodec::create(algo);
    auto start = std::chrono::high_resolution_clock::now();
    ByteBuffer payload = codec->compress(raw_data);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    ArchiveHeader header;
    header.magic = MAGIC_HEADER;
    header.version = FORMAT_VERSION;
    header.algorithm = algo;
    header.original_size = raw_data.size();
    header.original_crc32 = raw_crc;

    ByteBuffer archive = Container::pack(header, payload);
    Container::write_file(out_path, archive);

    double ratio = (payload.size() > 0) ? (static_cast<double>(raw_data.size()) / payload.size()) : 1.0;
    double savings = (raw_data.size() > 0) ? (1.0 - static_cast<double>(payload.size()) / raw_data.size()) * 100.0 : 0.0;

    std::cout << "[iCompress] Compressed successfully: '" << out_path << "'\n"
              << "  Original:   " << format_size(raw_data.size()) << "\n"
              << "  Compressed: " << format_size(archive.size())
              << " (" << std::fixed << std::setprecision(1) << savings << "% savings, "
              << std::setprecision(2) << ratio << "x ratio)\n"
              << "  Time:       " << std::fixed << std::setprecision(2) << elapsed_ms << " ms\n"
              << "  Checksum:   0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << raw_crc << "\n";

    return 0;
}

static int do_extract(const std::string& input_path, const std::string& custom_out, bool verbose) {
    ByteBuffer archive = Container::read_file(input_path);
    ArchiveHeader header = Container::unpack_header(archive);
    ByteBuffer payload = Container::extract_payload(archive);

    std::string out_path = custom_out;
    if (out_path.empty()) {
        if (input_path.size() > 3 && input_path.substr(input_path.size() - 3) == ".ic") {
            out_path = input_path.substr(0, input_path.size() - 3);
        } else {
            out_path = input_path + ".extracted";
        }
    }

    if (verbose) {
        std::cout << "[iCompress] Extracting archive '" << input_path << "' using "
                  << algorithm_name(header.algorithm) << "...\n";
    }

    auto codec = ICodec::create(header.algorithm);
    auto start = std::chrono::high_resolution_clock::now();
    ByteBuffer restored = codec->decompress(payload, header.original_size);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    uint32_t restored_crc = CRC32::calculate(restored);
    if (restored_crc != header.original_crc32) {
        std::cerr << "[iCompress] ERROR: CRC32 checksum mismatch!\n"
                  << "  Expected: 0x" << std::hex << std::uppercase << header.original_crc32 << "\n"
                  << "  Actual:   0x" << std::hex << std::uppercase << restored_crc << "\n";
        return 1;
    }

    Container::write_file(out_path, restored);

    std::cout << "[iCompress] Extracted successfully: '" << out_path << "'\n"
              << "  Restored:   " << format_size(restored.size()) << "\n"
              << "  Time:       " << std::fixed << std::setprecision(2) << elapsed_ms << " ms\n"
              << "  Integrity:  0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << restored_crc << " (MATCH)\n";

    return 0;
}

static int do_list(const std::string& input_path) {
    ByteBuffer archive = Container::read_file(input_path);
    ArchiveHeader header = Container::unpack_header(archive);
    ByteBuffer payload = Container::extract_payload(archive);

    double ratio = (payload.size() > 0) ? (static_cast<double>(header.original_size) / payload.size()) : 1.0;
    double savings = (header.original_size > 0) ? (1.0 - static_cast<double>(payload.size()) / header.original_size) * 100.0 : 0.0;

    std::cout << "\n=== iCompress Archive Details ===\n"
              << "  Archive Path:    " << input_path << "\n"
              << "  Format Version:  " << static_cast<int>(header.version) << "\n"
              << "  Codec Algorithm: " << algorithm_name(header.algorithm) << "\n"
              << "  Original Size:   " << format_size(header.original_size) << " (" << header.original_size << " bytes)\n"
              << "  Compressed Size: " << format_size(archive.size()) << " (" << archive.size() << " bytes)\n"
              << "  Payload Size:    " << format_size(payload.size()) << "\n"
              << "  Space Savings:   " << std::fixed << std::setprecision(1) << savings << "%\n"
              << "  Ratio:           " << std::setprecision(2) << ratio << "x\n"
              << "  Stored CRC32:    0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << header.original_crc32 << "\n\n";

    return 0;
}

static int do_test(const std::string& input_path) {
    ByteBuffer archive = Container::read_file(input_path);
    ArchiveHeader header = Container::unpack_header(archive);
    ByteBuffer payload = Container::extract_payload(archive);

    std::cout << "[iCompress] Testing archive integrity: '" << input_path << "'... ";
    auto codec = ICodec::create(header.algorithm);
    ByteBuffer restored = codec->decompress(payload, header.original_size);
    uint32_t calc_crc = CRC32::calculate(restored);

    if (calc_crc == header.original_crc32) {
        std::cout << "OK (CRC32 0x" << std::hex << std::uppercase << calc_crc << " MATCH)\n";
        return 0;
    } else {
        std::cout << "FAILED (CRC32 mismatch)\n";
        return 1;
    }
}

static int do_benchmark(const std::string& input_path) {
    ByteBuffer data = Container::read_file(input_path);
    auto stats = Benchmark::run(data);
    Benchmark::print_report(input_path, data.size(), stats);
    return 0;
}

static int launch_ui() {
    std::cout << "[iCompress] Launching desktop studio GUI...\n";
#if defined(_WIN32)
    system("start bin\\icompress-gui.exe");
#elif defined(__APPLE__)
    system("open ui/index.html");
#else
    system("xdg-open ui/index.html");
#endif
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage("icompress");
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") {
        print_usage("icompress");
        return 0;
    }
    if (cmd == "-V" || cmd == "--version") {
        print_version();
        return 0;
    }
    if (cmd == "ui" || cmd == "--gui") {
        return launch_ui();
    }
    if (cmd == "register" || cmd == "--register") {
#ifdef _WIN32
        return do_register_shell();
#else
        std::cout << "Shell registration is supported on Windows.\n";
        return 0;
#endif
    }
    if (cmd == "unregister" || cmd == "--unregister") {
#ifdef _WIN32
        return do_unregister_shell();
#else
        std::cout << "Shell unregistration is supported on Windows.\n";
        return 0;
#endif
    }

    std::string input_path;
    std::string output_path;
    Algorithm algo = Algorithm::Auto;
    bool verbose = false;

    // Parse command or short tag
    bool is_compress = (cmd == "compress" || cmd == "-c");
    bool is_extract = (cmd == "extract" || cmd == "-d" || cmd == "-x");
    bool is_list = (cmd == "list" || cmd == "-l");
    bool is_test = (cmd == "test" || cmd == "-t");
    bool is_bench = (cmd == "benchmark" || cmd == "-b");

    if (!is_compress && !is_extract && !is_list && !is_test && !is_bench) {
        // Fallback: check if first arg was a file path
        std::cerr << "Unknown command: " << cmd << "\n";
        print_usage("icompress");
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "-m" && i + 1 < argc) {
            algo = parse_algorithm(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage("icompress");
            return 0;
        } else if (arg[0] != '-') {
            input_path = arg;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: No input file specified.\n\n";
        print_usage("icompress");
        return 1;
    }

    try {
        if (is_compress) {
            return do_compress(input_path, output_path, algo, verbose);
        } else if (is_extract) {
            return do_extract(input_path, output_path, verbose);
        } else if (is_list) {
            return do_list(input_path);
        } else if (is_test) {
            return do_test(input_path);
        } else if (is_bench) {
            return do_benchmark(input_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "[iCompress] Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
