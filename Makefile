CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Iinclude -static -static-libgcc -static-libstdc++
LDFLAGS ?= 

SRCS = src/crc32.cpp \
       src/bitstream.cpp \
       src/container.cpp \
       src/codec.cpp \
       src/huffman.cpp \
       src/rle.cpp \
       src/bwt.cpp \
       src/arithmetic.cpp \
       src/lz77.cpp \
       src/lz78.cpp \
       src/lzw.cpp \
       src/deflate.cpp \
       src/benchmark.cpp

OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))

TARGET = bin/icompress.exe
TARGET_GUI = bin/icompress-gui.exe
TEST_CODECS = bin/test_codecs.exe
BATTLE_TEST = bin/battle_test.exe

.PHONY: all clean test battle gui

all: $(TARGET) $(TARGET_GUI)

build/%.o: src/%.cpp
	@if not exist build mkdir build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) src/main.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) src/main.cpp $(OBJS) -o $@ $(LDFLAGS)

$(TARGET_GUI): src/gui_main.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) -mwindows src/gui_main.cpp -o $@ $(LDFLAGS) -lshell32

$(TEST_CODECS): $(OBJS) tests/test_codecs.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) tests/test_codecs.cpp $(OBJS) -o $@ $(LDFLAGS)

$(BATTLE_TEST): $(OBJS) tests/battle_test.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) tests/battle_test.cpp $(OBJS) -o $@ $(LDFLAGS)

test: $(TEST_CODECS)
	@$(TEST_CODECS)

battle: $(BATTLE_TEST)
	@$(BATTLE_TEST)

clean:
	@if exist build rmdir /s /q build
	@if exist bin rmdir /s /q bin
