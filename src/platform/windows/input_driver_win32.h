#pragma once
#ifndef _INPUT_DRIVER_WIN32_H
#define _INPUT_DRIVER_WIN32_H

#include "../../core/platform/input_driver_interface.h"
#include <windows.h>
#include <string>

namespace yamy::platform {

class InputDriverWin32 : public IInputDriver
{
public:
    InputDriverWin32();
    virtual ~InputDriverWin32();

    virtual bool open(void *readEvent) override;
    virtual void close() override;
    virtual void manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void **moduleHandle) override;

private:
    HANDLE m_hDevice;
    OVERLAPPED m_ol;
};

} // namespace yamy::platform

#endif // !_INPUT_DRIVER_WIN32_H
