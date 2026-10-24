cmake_minimum_required(VERSION 3.16)
project(iCompress VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

include_directories(include)

set(LIB_SOURCES
    src/crc32.cpp
    src/bitstream.cpp
    src/container.cpp
    src/codec.cpp
    src/huffman.cpp
    src/rle.cpp
    src/bwt.cpp
    src/arithmetic.cpp
    src/lz77.cpp
    src/lz78.cpp
    src/lzw.cpp
    src/deflate.cpp
    src/benchmark.cpp
)

add_library(icompress_lib STATIC ${LIB_SOURCES})

add_executable(icompress src/main.cpp)
target_link_libraries(icompress PRIVATE icompress_lib)

if(WIN32)
    add_executable(icompress-gui WIN32 src/gui_main.cpp)
    target_link_libraries(icompress-gui PRIVATE shell32)
endif()

enable_testing()

add_executable(test_codecs tests/test_codecs.cpp)
target_link_libraries(test_codecs PRIVATE icompress_lib)
add_test(NAME test_codecs COMMAND test_codecs)

add_executable(battle_test tests/battle_test.cpp)
target_link_libraries(battle_test PRIVATE icompress_lib)
add_test(NAME battle_test COMMAND battle_test)

# Installation targets
install(TARGETS icompress RUNTIME DESTINATION bin)
if(WIN32)
    install(TARGETS icompress-gui RUNTIME DESTINATION bin)
endif()

