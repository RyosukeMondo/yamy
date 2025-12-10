//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// input_hook_linux.cpp - evdev input capture (Track 9 Phase 4-5)

#include "input_hook_linux.h"
#include "keycode_mapping.h"
#include "core/platform/platform_exception.h"
#include <linux/input.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

namespace yamy::platform {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// EventReaderThread Implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EventReaderThread::EventReaderThread(int fd, const std::string& devNode, KeyCallback callback)
    : m_fd(fd)
    , m_devNode(devNode)
    , m_callback(callback)
    , m_thread(0)
    , m_running(false)
    , m_stopRequested(false)
{
}

EventReaderThread::~EventReaderThread()
{
    stop();
}

bool EventReaderThread::start()
{
    if (m_running) return true;

    m_stopRequested = false;
    if (pthread_create(&m_thread, nullptr, threadFunc, this) != 0) {
        std::cerr << "[EventReader] Failed to create thread for " << m_devNode << std::endl;
        return false;
    }

    m_running = true;
    return true;
}

void EventReaderThread::stop()
{
    if (!m_running) return;

    m_stopRequested = true;

    // Wait for thread to finish
    pthread_join(m_thread, nullptr);
    m_running = false;
}

void* EventReaderThread::threadFunc(void* arg)
{
    EventReaderThread* self = static_cast<EventReaderThread*>(arg);
    self->run();
    return nullptr;
}

void EventReaderThread::run()
{
    std::cout << "[EventReader] Started reading from " << m_devNode << std::endl;

    struct input_event ev;

    while (!m_stopRequested) {
        ssize_t bytes = read(m_fd, &ev, sizeof(ev));

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, sleep briefly
                usleep(10000); // 10ms
                continue;
            }
            if (errno == ENODEV) {
                // Device disconnected
                std::cerr << "[EventReader] Device " << m_devNode << " disconnected" << std::endl;
                break;
            }
            // Other error
            std::cerr << "[EventReader] Read error on " << m_devNode << ": "
                      << strerror(errno) << std::endl;
            break;
        }

        if (bytes != sizeof(ev)) {
            // Incomplete read
            continue;
        }

        // Process only key events
        if (ev.type != EV_KEY) {
            continue;
        }

        // Filter out buttons (mouse buttons are also EV_KEY)
        // Mouse buttons are BTN_LEFT (0x110), BTN_RIGHT (0x111), etc.
        // Keyboard keys are KEY_ESC (1), KEY_A (30), etc.
        if (ev.code >= BTN_MISC && ev.code < KEY_OK) {
            // This is a button, not a keyboard key
            continue;
        }

        // Convert evdev code to YAMY code
        uint16_t yamyCode = evdevToYamyKeyCode(ev.code);
        if (yamyCode == 0) {
            // Unknown key, skip
            continue;
        }

        // Create KeyEvent
        KeyEvent event;
        event.key = KeyCode::Unknown; // We use scanCode primarily
        event.scanCode = yamyCode;
        event.isKeyDown = (ev.value == 1 || ev.value == 2); // 1=press, 2=repeat, 0=release
        event.isExtended = false; // evdev doesn't use extended scancodes
        event.timestamp = ev.time.tv_sec * 1000 + ev.time.tv_usec / 1000; // Convert to ms
        event.flags = 0;
        if (ev.value == 0) {
            event.flags |= 1; // Mark as key up
        }
        event.extraInfo = 0;

        // Call callback
        if (m_callback) {
            try {
                m_callback(event);
            } catch (const std::exception& e) {
                std::cerr << "[EventReader] Callback exception: " << e.what() << std::endl;
            }
        }
    }

    std::cout << "[EventReader] Stopped reading from " << m_devNode << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InputHookLinux Implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

InputHookLinux::InputHookLinux()
    : m_isInstalled(false)
{
}

InputHookLinux::~InputHookLinux()
{
    uninstall();
}

bool InputHookLinux::install(KeyCallback keyCallback, MouseCallback mouseCallback)
{
    if (m_isInstalled) {
        std::cerr << "[InputHook] Already installed" << std::endl;
        return true;
    }

    std::cout << "[InputHook] Installing input hook..." << std::endl;

    // First check if /dev/input directory exists
    struct stat st;
    if (stat("/dev/input", &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "[InputHook] /dev/input directory not found" << std::endl;
        throw EvdevUnavailableException("/dev/input directory not found");
    }

    // Check if any event devices exist
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        int err = errno;
        std::cerr << "[InputHook] Cannot open /dev/input: " << std::strerror(err) << std::endl;
        throw EvdevUnavailableException("Cannot open /dev/input: " + std::string(std::strerror(err)));
    }

    bool hasEventDevices = false;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            hasEventDevices = true;
            break;
        }
    }
    closedir(dir);

    if (!hasEventDevices) {
        std::cerr << "[InputHook] No event devices found in /dev/input" << std::endl;
        throw EvdevUnavailableException("No event devices found in /dev/input");
    }

    m_keyCallback = keyCallback;
    m_mouseCallback = mouseCallback;

    // Enumerate keyboard devices
    std::vector<InputDeviceInfo> keyboards = m_deviceManager.enumerateKeyboards();

    if (keyboards.empty()) {
        std::cerr << "[InputHook] No keyboard devices found!" << std::endl;
        std::cerr << "[InputHook] Event devices exist but none have keyboard capabilities." << std::endl;
        throw EvdevUnavailableException("Event devices exist but no keyboards found. Check permissions (input group)");
    }

    std::cout << "[InputHook] Found " << keyboards.size() << " keyboard device(s)" << std::endl;

    // Track grab failures for better error reporting
    int openFailures = 0;
    int grabFailures = 0;
    std::string lastGrabError;

    // Open and grab each keyboard
    for (const auto& kbInfo : keyboards) {
        std::cout << "[InputHook]   Opening: " << kbInfo.devNode << " (" << kbInfo.name << ")" << std::endl;

        // Open device
        int fd = DeviceManager::openDevice(kbInfo.devNode, false); // Blocking mode for read
        if (fd < 0) {
            std::cerr << "[InputHook]   Failed to open " << kbInfo.devNode << std::endl;
            openFailures++;
            continue;
        }

        // Grab device
        if (!DeviceManager::grabDevice(fd, true)) {
            int err = errno;
            std::cerr << "[InputHook]   Failed to grab " << kbInfo.devNode << ": " << std::strerror(err) << std::endl;
            lastGrabError = std::strerror(err);
            DeviceManager::closeDevice(fd);
            grabFailures++;
            continue;
        }

        // Store device
        OpenDevice dev;
        dev.fd = fd;
        dev.devNode = kbInfo.devNode;
        dev.name = kbInfo.name;
        dev.grabbed = true;
        m_openDevices.push_back(dev);

        // Create reader thread
        EventReaderThread* reader = new EventReaderThread(fd, kbInfo.devNode, m_keyCallback);
        if (!reader->start()) {
            std::cerr << "[InputHook]   Failed to start reader thread" << std::endl;
            delete reader;
            continue;
        }

        m_readerThreads.push_back(reader);
        std::cout << "[InputHook]   Successfully hooked " << kbInfo.devNode << std::endl;
    }

    if (m_readerThreads.empty()) {
        std::cerr << "[InputHook] Failed to hook any keyboard devices!" << std::endl;
        cleanup();

        // Provide specific error based on failure type
        if (openFailures == static_cast<int>(keyboards.size())) {
            throw DeviceAccessException(keyboards[0].devNode, EACCES,
                "Cannot open keyboard devices - check permissions (input group)");
        } else if (grabFailures > 0) {
            throw DeviceGrabException(keyboards[0].devNode, EBUSY, lastGrabError);
        } else {
            throw EvdevUnavailableException("Failed to hook keyboard devices");
        }
    }

    m_isInstalled = true;
    std::cout << "[InputHook] Input hook installed successfully ("
              << m_readerThreads.size() << " device(s) active)" << std::endl;

    return true;
}

void InputHookLinux::uninstall()
{
    if (!m_isInstalled) return;

    std::cout << "[InputHook] Uninstalling input hook..." << std::endl;

    cleanup();

    m_keyCallback = nullptr;
    m_mouseCallback = nullptr;
    m_isInstalled = false;

    std::cout << "[InputHook] Input hook uninstalled" << std::endl;
}

void InputHookLinux::cleanup()
{
    // Stop all reader threads
    for (EventReaderThread* reader : m_readerThreads) {
        reader->stop();
        delete reader;
    }
    m_readerThreads.clear();

    // Close all devices
    for (const OpenDevice& dev : m_openDevices) {
        std::cout << "[InputHook]   Closing " << dev.devNode << std::endl;
        DeviceManager::closeDevice(dev.fd);
    }
    m_openDevices.clear();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Factory function
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

IInputHook* createInputHook() {
    return new InputHookLinux();
}

} // namespace yamy::platform
