#pragma once
#ifndef _INPUT_DRIVER_H
#define _INPUT_DRIVER_H

/// Platform-neutral interface for kernel driver communication
class InputDriver
{
public:
    virtual ~InputDriver() { }

    /**
     * Open the driver device.
     * @param readEvent Handle to an event object that will be signaled when input is available.
     * @return true if successful.
     */
    virtual bool open(void *readEvent) = 0;

    /**
     * Close the driver device.
     */
    virtual void close() = 0;

    /**
     * Load/Unload kernel extension (e.g. sts4mayu.dll for ThumbSense)
     */
    virtual void manageExtension(const void *dllName, const void *dependDllName, bool load, void **moduleHandle) = 0;
};

#endif // !_INPUT_DRIVER_H
