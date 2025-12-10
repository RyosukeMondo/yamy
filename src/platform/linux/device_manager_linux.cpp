//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// device_manager_linux.cpp - Device enumeration (Track 9 Phase 2-3)

#include "device_manager_linux.h"
#include <libudev.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

// Helper macros for bit manipulation
#define NBITS(x) ((((x)-1)/8)+1)
#define test_bit(bit, array) ((array)[(bit)/8] & (1<<((bit)%8)))

namespace yamy::platform {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DeviceManager Implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DeviceManager::DeviceManager()
    : m_udev(nullptr)
{
    // Create udev context
    m_udev = udev_new();
    if (!m_udev) {
        std::cerr << "[DeviceManager] Failed to create udev context" << std::endl;
    }
}

DeviceManager::~DeviceManager()
{
    if (m_udev) {
        udev_unref(m_udev);
        m_udev = nullptr;
    }
}

std::vector<InputDeviceInfo> DeviceManager::enumerateDevices()
{
    std::vector<InputDeviceInfo> devices;

    if (!m_udev) {
        std::cerr << "[DeviceManager] udev context not initialized" << std::endl;
        return devices;
    }

    // Create enumerate object
    struct udev_enumerate* enumerate = udev_enumerate_new(m_udev);
    if (!enumerate) {
        std::cerr << "[DeviceManager] Failed to create udev enumerate" << std::endl;
        return devices;
    }

    // Filter for input subsystem
    udev_enumerate_add_match_subsystem(enumerate, "input");

    // Scan devices
    udev_enumerate_scan_devices(enumerate);

    // Get list of devices
    struct udev_list_entry* devices_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices_list) {
        const char* path = udev_list_entry_get_name(entry);

        // Create device object
        struct udev_device* dev = udev_device_new_from_syspath(m_udev, path);
        if (!dev) continue;

        // Get device node (e.g. /dev/input/event0)
        const char* devNode = udev_device_get_devnode(dev);
        if (!devNode) {
            udev_device_unref(dev);
            continue;
        }

        // Only process eventX devices
        std::string devNodeStr(devNode);
        if (devNodeStr.find("/dev/input/event") != 0) {
            udev_device_unref(dev);
            continue;
        }

        // Get device properties
        InputDeviceInfo info;
        info.devNode = devNodeStr;
        info.sysPath = path;

        // Get device name
        const char* name = udev_device_get_sysattr_value(dev, "name");
        if (name) {
            info.name = name;
        } else {
            // Fallback: get name from parent
            struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(
                dev, "input", NULL);
            if (parent) {
                const char* parent_name = udev_device_get_sysattr_value(parent, "name");
                if (parent_name) {
                    info.name = parent_name;
                }
            }
        }

        // Get vendor/product IDs from parent
        struct udev_device* usb_dev = udev_device_get_parent_with_subsystem_devtype(
            dev, "usb", "usb_device");
        if (usb_dev) {
            const char* vendor = udev_device_get_sysattr_value(usb_dev, "idVendor");
            const char* product = udev_device_get_sysattr_value(usb_dev, "idProduct");
            if (vendor) info.vendor = std::stoi(vendor, nullptr, 16);
            if (product) info.product = std::stoi(product, nullptr, 16);
        }

        // Check capabilities
        info.isKeyboard = isKeyboardDevice(devNodeStr);
        // TODO: Add mouse detection if needed
        info.isMouse = false;

        devices.push_back(info);

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);

    return devices;
}

std::vector<InputDeviceInfo> DeviceManager::enumerateKeyboards()
{
    std::vector<InputDeviceInfo> allDevices = enumerateDevices();
    std::vector<InputDeviceInfo> keyboards;

    for (const auto& dev : allDevices) {
        if (dev.isKeyboard) {
            keyboards.push_back(dev);
        }
    }

    return keyboards;
}

bool DeviceManager::isKeyboardDevice(const std::string& devNode)
{
    // Open device
    int fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        // Permission denied or doesn't exist
        return false;
    }

    // Check for EV_KEY capability
    unsigned long evBits[NBITS(EV_MAX)] = {0};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) {
        close(fd);
        return false;
    }

    bool hasKeys = test_bit(EV_KEY, evBits);

    if (hasKeys) {
        // Check for actual keyboard keys (not just mouse buttons)
        unsigned long keyBits[NBITS(KEY_MAX)] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) >= 0) {
            // Check for typical keyboard keys
            bool hasKeyboardKeys =
                test_bit(KEY_A, keyBits) ||
                test_bit(KEY_Z, keyBits) ||
                test_bit(KEY_ENTER, keyBits) ||
                test_bit(KEY_SPACE, keyBits);

            close(fd);
            return hasKeyboardKeys;
        }
    }

    close(fd);
    return false;
}

std::string DeviceManager::getDeviceName(const std::string& devNode)
{
    int fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return "";
    }

    char name[256] = "Unknown";
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        close(fd);
        return "";
    }

    close(fd);
    return std::string(name);
}

int DeviceManager::openDevice(const std::string& devNode, bool nonBlock)
{
    int flags = O_RDONLY;
    if (nonBlock) {
        flags |= O_NONBLOCK;
    }

    int fd = open(devNode.c_str(), flags);
    if (fd < 0) {
        std::cerr << "[DeviceManager] Failed to open " << devNode << ": "
                  << strerror(errno) << std::endl;
        return -1;
    }

    return fd;
}

bool DeviceManager::grabDevice(int fd, bool grab)
{
    if (fd < 0) return false;

    int grabFlag = grab ? 1 : 0;
    if (ioctl(fd, EVIOCGRAB, grabFlag) < 0) {
        std::cerr << "[DeviceManager] Failed to " << (grab ? "grab" : "ungrab")
                  << " device: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

void DeviceManager::closeDevice(int fd)
{
    if (fd >= 0) {
        // Ungrab before closing
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
}

} // namespace yamy::platform
