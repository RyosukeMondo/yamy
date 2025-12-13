#include "input_driver_win32.h"
#include "driver.h"
#include "misc.h"
#include "utf_conversion.h"
#include <tchar.h>

namespace yamy::platform {

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

void InputDriverWin32::manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void **moduleHandle)
{
#ifdef UNICODE
    std::wstring dllNameT = utf8_to_wstring(dllName);
    std::wstring dependDllNameT = utf8_to_wstring(dependDllName);
    const wchar_t* pDllName = dllNameT.c_str();
    const wchar_t* pDependName = dependDllNameT.c_str();
#else
    const char* pDllName = dllName.c_str();
    const char* pDependName = dependDllName.c_str();
#endif

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
            if (SearchPath(nullptr, pDependName, nullptr, 0, nullptr, nullptr) == 0) {
                // failed to find depend dll
            } else {
                *pTs4mayu = LoadLibrary(pDllName);
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

IInputDriver* createInputDriver() {
    return new InputDriverWin32();
}

} // namespace yamy::platform
