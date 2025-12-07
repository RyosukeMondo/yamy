#include "input_driver_win32.h"
#include "driver.h"
#include "misc.h"
#include "windowstool.h" // For addSessionId?
#include <tchar.h>

InputDriverWin32::InputDriverWin32()
    : m_hDevice(INVALID_HANDLE_VALUE)
{
    ZeroMemory(&m_ol, sizeof(m_ol));
}

InputDriverWin32::~InputDriverWin32()
{
    close();
}

bool InputDriverWin32::open(void *readEvent)
{
    if (m_hDevice != INVALID_HANDLE_VALUE)
        return true;

    // Note: original Engine::open() logic seems to be missing in current codebase scan,
    // but based on standard Mayu driver usage:
    m_hDevice = CreateFile(MAYU_DEVICE_FILE_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL);

    if (m_hDevice == INVALID_HANDLE_VALUE)
        return false;

    m_ol.hEvent = (HANDLE)readEvent;
    
    // Start async read if needed...
    // Historically, Mayu reads from the driver to get keyboard input.
    // But currently we use hooks. 
    // If we want to support the driver, we would issue a ReadFile here.
    
    return true;
}

void InputDriverWin32::close()
{
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        CancelIo(m_hDevice);
        CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
    }
}

void InputDriverWin32::manageExtension(const void *dllName, const void *dependDllName, bool load, void **moduleHandle)
{
    // Logic from Engine::manageTs4mayu
    const TCHAR *ts4mayuDllName = (const TCHAR *)dllName;
    const TCHAR *dependDllNameT = (const TCHAR *)dependDllName;
    HMODULE *pTs4mayu = (HMODULE *)moduleHandle;

    if (load == false) {
        if (*pTs4mayu) {
            bool (WINAPI *pTs4mayuTerm)();

            pTs4mayuTerm = (bool (WINAPI*)())GetProcAddress(*pTs4mayu, "ts4mayuTerm");
            if (pTs4mayuTerm && pTs4mayuTerm() == true)
                FreeLibrary(*pTs4mayu);
            *pTs4mayu = NULL;
        }
    } else {
        if (*pTs4mayu) {
            // already loaded
        } else {
            if (SearchPath(NULL, dependDllNameT, NULL, 0, NULL, NULL) == 0) {
                // failed to find depend dll
            } else {
                *pTs4mayu = LoadLibrary(ts4mayuDllName);
                if (*pTs4mayu == NULL) {
                    // failed to load
                } else {
                    bool (WINAPI *pTs4mayuInit)(UINT);
                    pTs4mayuInit = (bool (WINAPI*)(UINT))GetProcAddress(*pTs4mayu, "ts4mayuInit");
                    if (!pTs4mayuInit || pTs4mayuInit(GetCurrentThreadId()) != true) {
                        // failed init
                    }
                }
            }
        }
    }
}
