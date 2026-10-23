# iCompress

> A lightweight, zero-dependency lossless data archiver and multi-codec compression suite written in modern C++17.

`iCompress` implements canonical compression algorithms from ground up, providing a unified command-line tool, an interactive battle-test benchmarking engine, and a desktop studio interface.

---

## Highlights

- **Pure C++17 Architecture**: Zero external dependencies. Self-contained bitstreams, sliding windows, and range coders.
- **8 Reference Codecs**:
  - `Huffman`: Variable-length prefix entropy coding
  - `RLE`: Consecutive identical-symbol run-length packing
  - `BWT`: Burrows-Wheeler block permutation + inversion mapping + RLE
  - `Arithmetic`: 32-bit range subdivision integer arithmetic coding
  - `LZ77`: Sliding-window dictionary matching with configurable lookahead
  - `LZ78`: Dynamic dictionary phrase graph matching
  - `LZW`: Pre-seeded alphabet dictionary codebook
  - `DEFLATE`: Hybrid LZ77 sliding window + Huffman entropy layer (`--auto` default)
- **Lossless Bit-Exact Guarantee**: Every archive encapsulates an IEEE 802.3 standard CRC32 checksum, verified on extraction.
- **Battle-Tested Across Media Modalities**: Evaluated against Text, Audio (PCM waveforms), Image (RGB rasters), and Video (temporal frame sequences).
- **Dual Interface**: Fast POSIX CLI with short tags and subcommands, plus a minimalist Apple-inspired desktop studio (`icompress ui`).

---

## Battle-Test Benchmarks

The suite was battle-tested across four distinct data modalities:

| Modality | Reference Payload | Optimal Codec | Compression Ratio | Space Reduction | Checksum |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Text** | `source.txt` (16.0 KB) | **DEFLATE** | **26.5x** | **96.2%** | `PASS (CRC32)` |
| **Audio** | `tone.pcm` (15.6 KB 16-bit PCM) | **DEFLATE** | **2.31x** | **56.8%** | `PASS (CRC32)` |
| **Image** | `raster.rgb` (14.1 KB RGB bitmap) | **Arithmetic** | **1.11x** | **10.2%** | `PASS (CRC32)` |
| **Video** | `video.yuv` (15.6 KB consecutive frames)| **BWT + RLE** | **14.7x** | **93.2%** | `PASS (CRC32)` |

---

## Container Specification (`ICMP`)

Every `.ic` archive encapsulates a 20-byte binary header followed by the compressed payload:

```
+---------------+---------------+---------------+-----------------------+-----------------------+
|  Magic Bytes  | Version (1B)  | Algorithm(1B) | Original Size (8B)    | Original CRC32 (4B)   |
| "ICMP" (4B)   | 0x01          | 0x01 .. 0x08  | uint64_t (LE)         | uint32_t (LE)         |
+---------------+---------------+---------------+-----------------------+-----------------------+
|                              Compressed Payload Stream ...                                    |
+-----------------------------------------------------------------------------------------------+
```

---

## Building from Source

### Prerequisites
- C++17 compliant compiler (`g++` >= 8, `clang++` >= 7, or MSVC >= 2019)
- CMake 3.16+ or standard `make`

### Build with CMake
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Build with Make
```bash
make
```

Binaries will be placed in `bin/`:
- `bin/icompress` (Main CLI archiver)
- `bin/test_codecs` (Algorithm verification suite)
- `bin/battle_test` (Text/Audio/Image/Video benchmark suite)

---

## Quickstart & CLI Usage

### 1. Compress a File
```bash
# Default (DEFLATE / auto)
icompress compress file.txt -o file.ic

# Select a specific algorithm
icompress -c video.yuv -m bwt -o video.ic
icompress -c audio.pcm -m arithmetic -o audio.ic
```

### 2. Extract an Archive
```bash
icompress extract file.ic -o restored.txt
# or short tag
icompress -d file.ic
```

### 3. Verify Archive Integrity
```bash
icompress test file.ic
```

### 4. Inspect Archive Details
```bash
icompress list file.ic
```

### 5. Run the Multi-Algorithm Battle Benchmark
```bash
icompress benchmark sample.dat
```

### 6. Launch Desktop Interface
```bash
icompress ui
```

---

## Running Automated Tests

```bash
# Verify all 8 reference codecs against worked exam proofs
./bin/test_codecs

# Run the 4-modality media battle tests
./bin/battle_test
```

---

## License

MIT License. See [LICENSE](LICENSE) for details.
