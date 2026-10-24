#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::wstring find_browser() {
    std::vector<std::wstring> candidates;

    // Check Program Files for Chrome
    wchar_t progFiles[MAX_PATH];
    if (GetEnvironmentVariableW(L"ProgramFiles", progFiles, MAX_PATH)) {
        candidates.push_back(std::wstring(progFiles) + L"\\Google\\Chrome\\Application\\chrome.exe");
        candidates.push_back(std::wstring(progFiles) + L"\\Microsoft\\Edge\\Application\\msedge.exe");
    }

    wchar_t progFilesX86[MAX_PATH];
    if (GetEnvironmentVariableW(L"ProgramFiles(x86)", progFilesX86, MAX_PATH)) {
        candidates.push_back(std::wstring(progFilesX86) + L"\\Google\\Chrome\\Application\\chrome.exe");
        candidates.push_back(std::wstring(progFilesX86) + L"\\Microsoft\\Edge\\Application\\msedge.exe");
    }

    wchar_t localAppData[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH)) {
        candidates.push_back(std::wstring(localAppData) + L"\\Google\\Chrome\\Application\\chrome.exe");
        candidates.push_back(std::wstring(localAppData) + L"\\Microsoft\\Edge\\Application\\msedge.exe");
    }

    for (const auto& path : candidates) {
        DWORD attrs = GetFileAttributesW(path.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return path;
        }
    }

    return L"";
}

static std::wstring find_ui_file() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    fs::path exeDir = fs::path(exePath).parent_path();
    
    // Check if in bin/ -> look for ../ui/index.html
    fs::path candidate1 = exeDir.parent_path() / "ui" / "index.html";
    if (fs::exists(candidate1)) {
        return candidate1.wstring();
    }

    // Check if in root -> look for ./ui/index.html
    fs::path candidate2 = exeDir / "ui" / "index.html";
    if (fs::exists(candidate2)) {
        return candidate2.wstring();
    }

    return L"";
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::wstring uiFile = find_ui_file();
    if (uiFile.empty()) {
        MessageBoxW(nullptr, 
            L"Could not locate ui/index.html. Please ensure the 'ui' directory exists in the iCompress root.",
            L"iCompress Studio Error", MB_ICONERROR);
        return 1;
    }

    std::wstring browser = find_browser();
    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring profileDir = std::wstring(tempDir) + L"icompress_studio_profile";

    // Format file URL: convert backslashes to forward slashes
    std::wstring fileUrl = L"file:///";
    for (wchar_t c : uiFile) {
        if (c == L'\\') fileUrl += L'/';
        else fileUrl += c;
    }

    // Check if an input file was passed via command line (e.g. double click or context menu)
    int numArgs = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &numArgs);
    if (argvW && numArgs > 1) {
        std::wstring openArg = argvW[1];
        fileUrl += L"?open=";
        for (wchar_t c : openArg) {
            if (c == L'\\') fileUrl += L'/';
            else if (c == L' ') fileUrl += L"%20";
            else fileUrl += c;
        }
        LocalFree(argvW);
    }

    if (!browser.empty()) {
        // Launch isolated desktop application window with full native app disguise
        std::wstring args = L"--app=\"" + fileUrl + L"\" "
                            L"--window-size=1240,820 "
                            L"--user-data-dir=\"" + profileDir + L"\" "
                            L"--no-first-run "
                            L"--no-default-browser-check "
                            L"--disable-features=Translate,OptimizationHints,MediaRouter "
                            L"--disable-extensions "
                            L"--disable-background-networking "
                            L"--disable-sync "
                            L"--disable-default-apps "
                            L"--app-id=icompress-archiver";

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        std::wstring cmdLine = L"\"" + browser + L"\" " + args;
        std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back(0);

        if (CreateProcessW(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
    }

    // Fallback: system shell execute default viewer
    ShellExecuteW(nullptr, L"open", uiFile.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return 0;
}
