#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// device_manager_linux.h - Device enumeration and management (Track 9 Phase 2-3)

#include <string>
#include <vector>
#include <cstdint>

namespace yamy::platform {

/// Information about an input device
struct InputDeviceInfo {
    std::string devNode;        // e.g. "/dev/input/event0"
    std::string name;           // Device name
    std::string sysPath;        // sysfs path
    uint16_t vendor;            // Vendor ID
    uint16_t product;           // Product ID
    bool isKeyboard;            // Has keyboard capabilities
    bool isMouse;               // Has mouse capabilities
};

/// Represents an opened input device
struct OpenDevice {
    int fd;                     // File descriptor
    std::string devNode;        // Device path
    std::string name;           // Device name
    bool grabbed;               // Whether we have exclusive access
};

/// Manages input device enumeration and monitoring
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    /// Enumerate all input devices
    /// @return List of input devices found
    std::vector<InputDeviceInfo> enumerateDevices();

    /// Enumerate only keyboard devices
    /// @return List of keyboard devices
    std::vector<InputDeviceInfo> enumerateKeyboards();

    /// Open device for reading
    /// @param devNode Device path (e.g. "/dev/input/event0")
    /// @param nonBlock Open in non-blocking mode
    /// @return File descriptor or -1 on error
    static int openDevice(const std::string& devNode, bool nonBlock = true);

    /// Grab device for exclusive access
    /// @param fd File descriptor from openDevice()
    /// @param grab true to grab, false to ungrab
    /// @return true on success
    static bool grabDevice(int fd, bool grab = true);

    /// Close device
    /// @param fd File descriptor
    static void closeDevice(int fd);

    /// Check if device has keyboard capabilities
    /// @param devNode Device node path (e.g. "/dev/input/event0")
    /// @return true if device has EV_KEY capability
    static bool isKeyboardDevice(const std::string& devNode);

    /// Get device name
    /// @param devNode Device node path
    /// @return Device name or empty string on error
    static std::string getDeviceName(const std::string& devNode);

private:
    struct udev* m_udev;
};

} // namespace yamy::platform
