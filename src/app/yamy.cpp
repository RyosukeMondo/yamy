//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy.cpp

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include "stringtool.h"
#include "mayurc.h"
#include <string>
#include "../platform/windows/utf_conversion.h" // For utf8_to_wstring

// Forward declaration
int appMain(const std::string& cmdLine);

/// main
extern "C" int WINAPI wWinMain(HINSTANCE i_hInstance, HINSTANCE /* i_hPrevInstance */,
                     LPWSTR i_lpszCmdLine, int /* i_nCmdShow */)
{
    // Convert command line to UTF-8
    std::string cmdLine = yamy::platform::wstring_to_utf8(i_lpszCmdLine);

    // Rest of application uses UTF-8 internally
    int result = appMain(cmdLine);

    return result;
}

int appMain(const std::string& /*cmdLine*/) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL result;
    std::string yamyPath;
    wchar_t exePath[GANA_MAX_PATH];
    wchar_t exeDrive[GANA_MAX_PATH];
    wchar_t exeDir[GANA_MAX_PATH];

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    GetModuleFileNameW(nullptr, exePath, GANA_MAX_PATH);
    _wsplitpath_s(exePath, exeDrive, GANA_MAX_PATH, exeDir, GANA_MAX_PATH, nullptr, 0, nullptr, 0);
    yamyPath = yamy::platform::wstring_to_utf8(exeDrive);
    yamyPath += yamy::platform::wstring_to_utf8(exeDir);

#ifdef _WIN64
    // If this launcher is 64-bit, the OS must be 64-bit.
    yamyPath += "yamy64.exe";
#else
    // If this launcher is 32-bit, check if we are running on 64-bit OS (WOW64).
    typedef BOOL (WINAPI* ISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
    BOOL isWow64 = FALSE;
    ISWOW64PROCESS pIsWow64Process;

    pIsWow64Process =
        (ISWOW64PROCESS)::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"),
                                         "IsWow64Process");
    if (pIsWow64Process) {
        if (pIsWow64Process(::GetCurrentProcess(), &isWow64) && isWow64) {
            yamyPath += "yamy64.exe";
        } else {
            yamyPath += "yamy32.exe";
        }
    } else {
        yamyPath += "yamy32.exe";
    }
#endif

    std::wstring wYamyPath = yamy::platform::utf8_to_wstring(yamyPath);
    result = CreateProcessW(wYamyPath.c_str(), nullptr, nullptr, nullptr, FALSE,
                           NORMAL_PRIORITY_CLASS, 0, nullptr, &si, &pi);

    if (result == FALSE) {
        wchar_t buf[1024];
        wchar_t text[1024];
        wchar_t title[1024];

        LoadStringW(GetModuleHandle(nullptr), IDS_cannotInvoke,
                   text, sizeof(text)/sizeof(text[0]));
        LoadStringW(GetModuleHandle(nullptr), IDS_mayu,
                   title, sizeof(title)/sizeof(title[0]));
        swprintf(buf, sizeof(buf)/sizeof(buf[0]),
                    text, wYamyPath.c_str(), GetLastError());
        MessageBoxW((HWND)nullptr, buf, title, MB_OK | MB_ICONSTOP);
    } else {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    return 0;
}
