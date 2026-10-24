@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   iCompress C++17 Lossless Suite - Windows Build
echo ===================================================

:: 1. Locate MinGW C++ Toolchain
where g++ >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "CXX=g++"
    set "RC=windres"
) else (
    if exist "C:\mingw64\bin\g++.exe" (
        set "PATH=C:\mingw64\bin;%PATH%"
        set "CXX=g++"
        set "RC=windres"
        echo [i] Using MinGW-w64 from C:\mingw64\bin
    ) else (
        echo [ERROR] Could not find g++ compiler in PATH or C:\mingw64\bin!
        echo Please ensure MinGW-w64 is installed or in your system PATH.
        exit /b 1
    )
)

:: 2. Ensure directories exist
if not exist build mkdir build
if not exist bin mkdir bin

:: 3. Compile Windows Icon Resource
echo [1/5] Compiling application icon resource...
%RC% src\resources.rc -O coff -o build\resources.o
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile icon resource.
    exit /b 1
)

:: 4. Source files
set "COMMON_SRCS=src\crc32.cpp src\bitstream.cpp src\container.cpp src\codec.cpp src\huffman.cpp src\rle.cpp src\bwt.cpp src\arithmetic.cpp src\lz77.cpp src\lz78.cpp src\lzw.cpp src\deflate.cpp src\benchmark.cpp"
set "FLAGS=-std=c++17 -O3 -Wall -Wextra -Iinclude -static -static-libgcc -static-libstdc++"

:: 5. Compile CLI Archiver
echo [2/5] Building bin\icompress.exe (CLI Archiver)...
%CXX% %FLAGS% src\main.cpp %COMMON_SRCS% build\resources.o -o bin\icompress.exe -lshell32 -ladvapi32
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to build bin\icompress.exe
    exit /b 1
)

:: 6. Compile Desktop GUI Application
echo [3/5] Building bin\icompress-gui.exe (Desktop GUI Application)...
%CXX% %FLAGS% -mwindows src\gui_main.cpp build\resources.o -o bin\icompress-gui.exe -lshell32 -ladvapi32
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to build bin\icompress-gui.exe
    exit /b 1
)

:: 7. Compile Unit Test Suite
echo [4/5] Building bin\test_codecs.exe (Algorithm Verification Suite)...
%CXX% %FLAGS% tests\test_codecs.cpp %COMMON_SRCS% -o bin\test_codecs.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to build bin\test_codecs.exe
    exit /b 1
)

:: 8. Compile Battle-Test Benchmark Suite
echo [5/5] Building bin\battle_test.exe (Empirical Modality Suite)...
%CXX% %FLAGS% tests\battle_test.cpp %COMMON_SRCS% -o bin\battle_test.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to build bin\battle_test.exe
    exit /b 1
)

echo.
echo [SUCCESS] All binaries compiled successfully in bin\
echo   - bin\icompress.exe       (Command-line archiver)
echo   - bin\icompress-gui.exe   (Desktop GUI application)
echo   - bin\test_codecs.exe     (8 reference codec verification)
echo   - bin\battle_test.exe     (Text, Audio, Image, Video benchmark)
echo ===================================================
exit /b 0
