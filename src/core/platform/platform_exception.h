#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// platform_exception.h - Platform-specific exception classes

#include <stdexcept>
#include <string>

namespace yamy::platform {

/// Base exception for all platform-related errors
class PlatformException : public std::runtime_error {
public:
    explicit PlatformException(const std::string& message)
        : std::runtime_error(message) {}
};

/// Exception for display/window system connection failures
class DisplayConnectionException : public PlatformException {
public:
    explicit DisplayConnectionException(const std::string& displayName = "")
        : PlatformException(buildMessage(displayName))
        , m_displayName(displayName) {}

    const std::string& displayName() const { return m_displayName; }

private:
    std::string m_displayName;

    static std::string buildMessage(const std::string& displayName) {
        std::string msg = "Failed to connect to display server";
        if (!displayName.empty()) {
            msg += " (display: " + displayName + ")";
        }
        msg += ". Please check:\n";
        msg += "  1. The DISPLAY environment variable is set correctly\n";
        msg += "  2. The X11 server is running\n";
        msg += "  3. You have permission to connect to the display";
        return msg;
    }
};

/// Exception for X11 extension unavailability
class ExtensionUnavailableException : public PlatformException {
public:
    explicit ExtensionUnavailableException(const std::string& extensionName,
                                           const std::string& suggestion = "")
        : PlatformException(buildMessage(extensionName, suggestion))
        , m_extensionName(extensionName) {}

    const std::string& extensionName() const { return m_extensionName; }

private:
    std::string m_extensionName;

    static std::string buildMessage(const std::string& extensionName,
                                    const std::string& suggestion) {
        std::string msg = "Required X11 extension '" + extensionName + "' is not available";
        if (!suggestion.empty()) {
            msg += ". " + suggestion;
        }
        return msg;
    }
};

/// Exception for X11 protocol errors
class X11ProtocolException : public PlatformException {
public:
    X11ProtocolException(int errorCode, const std::string& errorText)
        : PlatformException(buildMessage(errorCode, errorText))
        , m_errorCode(errorCode) {}

    int errorCode() const { return m_errorCode; }

private:
    int m_errorCode;

    static std::string buildMessage(int errorCode, const std::string& errorText) {
        return "X11 protocol error " + std::to_string(errorCode) + ": " + errorText;
    }
};

/// Exception for device access errors (e.g., /dev/uinput, /dev/input/*)
class DeviceAccessException : public PlatformException {
public:
    DeviceAccessException(const std::string& devicePath, int errorCode,
                          const std::string& errorText)
        : PlatformException(buildMessage(devicePath, errorCode, errorText))
        , m_devicePath(devicePath)
        , m_errorCode(errorCode) {}

    const std::string& devicePath() const { return m_devicePath; }
    int errorCode() const { return m_errorCode; }

private:
    std::string m_devicePath;
    int m_errorCode;

    static std::string buildMessage(const std::string& devicePath,
                                    int errorCode, const std::string& errorText) {
        std::string msg = "Failed to access device '" + devicePath + "': " + errorText;
        msg += " (errno " + std::to_string(errorCode) + ")";
        msg += "\nPlease check:\n";
        msg += "  1. The device exists\n";
        msg += "  2. You have permission to access the device (try adding user to 'input' group)";
        return msg;
    }
};

/// Exception for uinput subsystem unavailability
class UinputUnavailableException : public PlatformException {
public:
    explicit UinputUnavailableException(int errorCode, const std::string& errorText)
        : PlatformException(buildMessage(errorCode, errorText))
        , m_errorCode(errorCode) {}

    int errorCode() const { return m_errorCode; }

private:
    int m_errorCode;

    static std::string buildMessage(int errorCode, const std::string& errorText) {
        std::string msg = "Failed to access /dev/uinput: " + errorText;
        msg += " (errno " + std::to_string(errorCode) + ")";
        msg += "\nThe uinput kernel module is required for input injection.";
        msg += "\nPlease check:\n";
        msg += "  1. Load uinput module: sudo modprobe uinput\n";
        msg += "  2. Create udev rule for persistent access:\n";
        msg += "     echo 'KERNEL==\"uinput\", MODE=\"0660\", GROUP=\"input\"' | sudo tee /etc/udev/rules.d/99-uinput.rules\n";
        msg += "  3. Add your user to input group: sudo usermod -a -G input $USER\n";
        msg += "  4. Reload udev: sudo udevadm control --reload-rules && sudo udevadm trigger\n";
        msg += "  5. Log out and back in for group changes to take effect";
        return msg;
    }
};

/// Exception for evdev subsystem unavailability (no keyboard devices found)
class EvdevUnavailableException : public PlatformException {
public:
    explicit EvdevUnavailableException(const std::string& reason = "")
        : PlatformException(buildMessage(reason)) {}

private:
    static std::string buildMessage(const std::string& reason) {
        std::string msg = "No keyboard devices found or accessible";
        if (!reason.empty()) {
            msg += ": " + reason;
        }
        msg += "\nThe evdev subsystem is required for input capture.";
        msg += "\nPlease check:\n";
        msg += "  1. Add your user to input group: sudo usermod -a -G input $USER\n";
        msg += "  2. Log out and back in for group changes to take effect\n";
        msg += "  3. Verify /dev/input/event* devices exist\n";
        msg += "  4. Check permissions: ls -la /dev/input/event*";
        return msg;
    }
};

/// Exception for failed device grab (exclusive access)
class DeviceGrabException : public PlatformException {
public:
    DeviceGrabException(const std::string& devicePath, int errorCode,
                        const std::string& errorText)
        : PlatformException(buildMessage(devicePath, errorCode, errorText))
        , m_devicePath(devicePath)
        , m_errorCode(errorCode) {}

    const std::string& devicePath() const { return m_devicePath; }
    int errorCode() const { return m_errorCode; }

private:
    std::string m_devicePath;
    int m_errorCode;

    static std::string buildMessage(const std::string& devicePath,
                                    int errorCode, const std::string& errorText) {
        std::string msg = "Failed to grab exclusive access to '" + devicePath + "': " + errorText;
        msg += " (errno " + std::to_string(errorCode) + ")";
        msg += "\nAnother application may have exclusive access to the device.";
        msg += "\nPlease check:\n";
        msg += "  1. No other keyboard remapping software is running\n";
        msg += "  2. Close applications that might grab keyboards (e.g., VMs, remote desktop)";
        return msg;
    }
};

} // namespace yamy::platform
