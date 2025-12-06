#ifndef _INPUT_DRIVER_WIN32_H
#define _INPUT_DRIVER_WIN32_H

#include "../../core/input/input_driver.h"
#include <windows.h>

class InputDriverWin32 : public InputDriver
{
public:
	InputDriverWin32();
	virtual ~InputDriverWin32();

	virtual bool open(void *readEvent);
	virtual void close();
	virtual void manageExtension(const void *dllName, const void *dependDllName, bool load, void **moduleHandle);

private:
	HANDLE m_hDevice;
	OVERLAPPED m_ol;
};

#endif // !_INPUT_DRIVER_WIN32_H
