#include "input_driver_win32.h"
#include "driver.h"
#include "misc.h"
#include "windowstool.h"
#include "../../utils/stringtool.h"
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

    m_hDevice = CreateFile(MAYU_DEVICE_FILE_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           nullptr);

    if (m_hDevice == INVALID_HANDLE_VALUE)
        return false;

    m_ol.hEvent = (HANDLE)readEvent;
    
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
    const TCHAR *ts4mayuDllName = (const TCHAR *)dllName;
    const TCHAR *dependDllNameT = (const TCHAR *)dependDllName;
    HMODULE *pTs4mayu = (HMODULE *)moduleHandle;

    if (load == false) {
        if (*pTs4mayu) {
            bool (WINAPI *pTs4mayuTerm)();

            pTs4mayuTerm = (bool (WINAPI*)())GetProcAddress(*pTs4mayu, "ts4mayuTerm");
            if (pTs4mayuTerm && pTs4mayuTerm() == true)
                FreeLibrary(*pTs4mayu);
            *pTs4mayu = nullptr;
        }
    } else {
        if (*pTs4mayu) {
            // already loaded
        } else {
            if (SearchPath(nullptr, dependDllNameT, nullptr, 0, nullptr, nullptr) == 0) {
                // failed to find depend dll
            } else {
                *pTs4mayu = LoadLibrary(ts4mayuDllName);
                if (*pTs4mayu == nullptr) {
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

// Implementation of IInputDriver interface

bool InputDriverWin32::initialize() {
    return open(nullptr);
}

void InputDriverWin32::shutdown() {
    close();
}

void InputDriverWin32::processEvents() {
    // No-op for now
}

bool InputDriverWin32::isKeyPressed(uint32_t key) const {
    return (GetAsyncKeyState((int)key) & 0x8000) != 0;
}

void InputDriverWin32::manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void** moduleHandle) {
    manageExtension((const void*)to_tstring(dllName).c_str(), (const void*)to_tstring(dependDllName).c_str(), load, moduleHandle);
}

namespace yamy::platform {
    IInputDriver* createInputDriver() {
        return new InputDriverWin32();
    }
}
