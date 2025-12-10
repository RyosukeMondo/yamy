//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// input_hook_linux.cpp - evdev input capture (Track 9 Phase 4-5)

#include "input_hook_linux.h"
#include "keycode_mapping.h"
#include "core/platform/platform_exception.h"
#include "../../utils/platform_logger.h"
#include <linux/input.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
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
        PLATFORM_LOG_ERROR("input", "Failed to create thread for %s", m_devNode.c_str());
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
    PLATFORM_LOG_INFO("input", "Started reading from %s", m_devNode.c_str());

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
                PLATFORM_LOG_WARN("input", "Device %s disconnected", m_devNode.c_str());
                break;
            }
            // Other error
            PLATFORM_LOG_ERROR("input", "Read error on %s: %s", m_devNode.c_str(), strerror(errno));
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

        // Log key event (scancode only, no sensitive info)
        PLATFORM_LOG_DEBUG("input", "Key event: scancode=0x%04x %s",
                           yamyCode, event.isKeyDown ? "DOWN" : "UP");

        // Call callback
        if (m_callback) {
            try {
                m_callback(event);
            } catch (const std::exception& e) {
                PLATFORM_LOG_ERROR("input", "Callback exception: %s", e.what());
            }
        }
    }

    PLATFORM_LOG_INFO("input", "Stopped reading from %s", m_devNode.c_str());
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
        PLATFORM_LOG_WARN("input", "Input hook already installed");
        return true;
    }

    PLATFORM_LOG_INFO("input", "Installing input hook...");

    // First check if /dev/input directory exists
    struct stat st;
    if (stat("/dev/input", &st) != 0 || !S_ISDIR(st.st_mode)) {
        PLATFORM_LOG_ERROR("input", "/dev/input directory not found");
        throw EvdevUnavailableException("/dev/input directory not found");
    }

    // Check if any event devices exist
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        int err = errno;
        PLATFORM_LOG_ERROR("input", "Cannot open /dev/input: %s", std::strerror(err));
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
        PLATFORM_LOG_ERROR("input", "No event devices found in /dev/input");
        throw EvdevUnavailableException("No event devices found in /dev/input");
    }

    m_keyCallback = keyCallback;
    m_mouseCallback = mouseCallback;

    // Enumerate keyboard devices
    std::vector<InputDeviceInfo> keyboards = m_deviceManager.enumerateKeyboards();

    if (keyboards.empty()) {
        PLATFORM_LOG_ERROR("input", "No keyboard devices found");
        PLATFORM_LOG_ERROR("input", "Event devices exist but none have keyboard capabilities");
        throw EvdevUnavailableException("Event devices exist but no keyboards found. Check permissions (input group)");
    }

    PLATFORM_LOG_INFO("input", "Found %zu keyboard device(s)", keyboards.size());

    // Track grab failures for better error reporting
    int openFailures = 0;
    int grabFailures = 0;
    std::string lastGrabError;

    // Open and grab each keyboard
    for (const auto& kbInfo : keyboards) {
        PLATFORM_LOG_INFO("input", "Opening: %s (%s)", kbInfo.devNode.c_str(), kbInfo.name.c_str());

        // Open device
        int fd = DeviceManager::openDevice(kbInfo.devNode, false); // Blocking mode for read
        if (fd < 0) {
            PLATFORM_LOG_WARN("input", "Failed to open %s", kbInfo.devNode.c_str());
            openFailures++;
            continue;
        }

        // Grab device
        if (!DeviceManager::grabDevice(fd, true)) {
            int err = errno;
            PLATFORM_LOG_WARN("input", "Failed to grab %s: %s", kbInfo.devNode.c_str(), std::strerror(err));
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
            PLATFORM_LOG_WARN("input", "Failed to start reader thread for %s", kbInfo.devNode.c_str());
            delete reader;
            continue;
        }

        m_readerThreads.push_back(reader);
        PLATFORM_LOG_INFO("input", "Successfully hooked %s", kbInfo.devNode.c_str());
    }

    if (m_readerThreads.empty()) {
        PLATFORM_LOG_ERROR("input", "Failed to hook any keyboard devices");
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
    PLATFORM_LOG_INFO("input", "Input hook installed successfully (%zu device(s) active)", m_readerThreads.size());

    return true;
}

void InputHookLinux::uninstall()
{
    if (!m_isInstalled) return;

    PLATFORM_LOG_INFO("input", "Uninstalling input hook...");

    cleanup();

    m_keyCallback = nullptr;
    m_mouseCallback = nullptr;
    m_isInstalled = false;

    PLATFORM_LOG_INFO("input", "Input hook uninstalled");
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
        PLATFORM_LOG_DEBUG("input", "Closing %s", dev.devNode.c_str());
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
