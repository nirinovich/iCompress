CXX ?= g++
RC ?= windres
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
RES_OBJ = build/resources.o

TARGET = bin/icompress.exe
TARGET_GUI = bin/icompress-gui.exe
TEST_CODECS = bin/test_codecs.exe
BATTLE_TEST = bin/battle_test.exe

.PHONY: all clean test battle gui install uninstall

all: $(TARGET) $(TARGET_GUI)

$(RES_OBJ): src/resources.rc
	@if not exist build mkdir build
	$(RC) src/resources.rc -O coff -o $@

build/%.o: src/%.cpp
	@if not exist build mkdir build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(RES_OBJ) src/main.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) src/main.cpp $(OBJS) $(RES_OBJ) -o $@ $(LDFLAGS)

$(TARGET_GUI): $(RES_OBJ) src/gui_main.cpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) -mwindows src/gui_main.cpp $(RES_OBJ) -o $@ $(LDFLAGS) -lshell32

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

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

install: all
	@mkdir -p $(DESTDIR)$(BINDIR) 2>nul || mkdir $(DESTDIR)$(BINDIR) 2>nul || true
	@copy /y bin\icompress.exe "$(DESTDIR)$(BINDIR)\" 2>nul || cp -f $(TARGET) $(DESTDIR)$(BINDIR)/ 2>nul || true
	@echo Installed to $(DESTDIR)$(BINDIR)

uninstall:
	@del /f /q "$(DESTDIR)$(BINDIR)\icompress.exe" 2>nul || rm -f $(DESTDIR)$(BINDIR)/icompress 2>nul || true

