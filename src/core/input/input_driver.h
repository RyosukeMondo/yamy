#pragma once
#ifndef _INPUT_DRIVER_H
#define _INPUT_DRIVER_H

/**
 * @file input_driver.h
 * @brief Platform-neutral interface for kernel driver communication.
 *
 * Defines the InputDriver abstract interface for communicating with
 * platform-specific kernel-mode input drivers or hooks.
 */

/**
 * @brief Platform-neutral interface for kernel driver communication.
 *
 * This interface abstracts kernel-mode driver operations required for
 * low-level keyboard input interception and injection. Platform-specific
 * implementations handle the actual driver communication (e.g., Windows IOCTL,
 * Linux uinput).
 *
 * @note The driver must be opened before use and properly closed on shutdown.
 *
 * @pre Driver must be installed on the system
 * @pre Caller must have appropriate permissions for driver access
 *
 * @code
 * InputDriver* driver = createPlatformDriver();
 * void* event = createEvent();
 * if (driver->open(event)) {
 *     // Driver ready for use
 *     driver->close();
 * }
 * @endcode
 */
class InputDriver
{
public:
    virtual ~InputDriver() { }

    /**
     * @brief Open the driver device for communication.
     *
     * @param readEvent Handle to an event object that will be signaled when
     *                  input is available for reading. Platform-specific type
     *                  (e.g., HANDLE on Windows, file descriptor on Linux).
     * @return true if successful, false on failure.
     *
     * @pre Driver must be installed and accessible
     * @pre readEvent must be a valid event handle
     * @post Driver is ready for read/write operations
     *
     * @note This method must be called before any other driver operations.
     */
    virtual bool open(void *readEvent) = 0;

    /**
     * @brief Close the driver device.
     *
     * @pre Driver must have been opened with open()
     * @post Driver is closed and cannot be used until reopened
     *
     * @note Always call this before application shutdown.
     */
    virtual void close() = 0;

    /**
     * @brief Load or unload a kernel extension.
     *
     * Manages kernel-mode extensions such as sts4mayu.dll for ThumbSense support.
     *
     * @param dllName Path to the kernel extension DLL
     * @param dependDllName Path to a dependency DLL (or nullptr)
     * @param load true to load the extension, false to unload
     * @param moduleHandle Receives the module handle on load, or provides it for unload
     *
     * @pre Driver must be opened
     * @pre Caller must have kernel module loading permissions
     * @post Extension is loaded/unloaded as requested
     *
     * @note Extension loading typically requires administrator privileges.
     */
    virtual void manageExtension(const void *dllName, const void *dependDllName, bool load, void **moduleHandle) = 0;
};

#endif // !_INPUT_DRIVER_H
