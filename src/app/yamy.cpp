//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy.cpp

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include "stringtool.h"
#include "mayurc.h"

/// main
extern "C" int WINAPI _tWinMain(HINSTANCE i_hInstance, HINSTANCE /* i_hPrevInstance */,
                     LPTSTR /* i_lpszCmdLine */, int /* i_nCmdShow */)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL result;
    tstring yamyPath;
    _TCHAR exePath[GANA_MAX_PATH];
    _TCHAR exeDrive[GANA_MAX_PATH];
    _TCHAR exeDir[GANA_MAX_PATH];

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    GetModuleFileName(nullptr, exePath, GANA_MAX_PATH);
    _tsplitpath_s(exePath, exeDrive, GANA_MAX_PATH, exeDir, GANA_MAX_PATH, nullptr, 0, nullptr, 0);
    yamyPath = exeDrive;
    yamyPath += exeDir;

#ifdef _WIN64
    // If this launcher is 64-bit, the OS must be 64-bit.
    yamyPath += _T("yamy64.exe");
#else
    // If this launcher is 32-bit, check if we are running on 64-bit OS (WOW64).
    typedef BOOL (WINAPI* ISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
    BOOL isWow64 = FALSE;
    ISWOW64PROCESS pIsWow64Process;

    pIsWow64Process =
        (ISWOW64PROCESS)::GetProcAddress(::GetModuleHandle(_T("kernel32.dll")),
                                         "IsWow64Process");
    if (pIsWow64Process) {
        if (pIsWow64Process(::GetCurrentProcess(), &isWow64) && isWow64) {
            yamyPath += _T("yamy64.exe");
        } else {
            yamyPath += _T("yamy32.exe");
        }
    } else {
        yamyPath += _T("yamy32.exe");
    }
#endif

    result = CreateProcess(yamyPath.c_str(), nullptr, nullptr, nullptr, FALSE,
                           NORMAL_PRIORITY_CLASS, 0, nullptr, &si, &pi);

    if (result == FALSE) {
        TCHAR buf[1024];
        TCHAR text[1024];
        TCHAR title[1024];

        LoadString(i_hInstance, IDS_cannotInvoke,
                   text, sizeof(text)/sizeof(text[0]));
        LoadString(i_hInstance, IDS_mayu,
                   title, sizeof(title)/sizeof(title[0]));
        _stprintf_s(buf, sizeof(buf)/sizeof(buf[0]),
                    text, yamyPath.c_str(), GetLastError());
        MessageBox((HWND)nullptr, buf, title, MB_OK | MB_ICONSTOP);
    } else {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    return 0;
}
