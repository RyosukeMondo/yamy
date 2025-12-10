#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// input_hook_linux.h - evdev input capture (Track 9 Phase 4-5)

#include "../../core/platform/input_hook_interface.h"
#include "device_manager_linux.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

namespace yamy::platform {

/// Event reader thread for a single device
class EventReaderThread {
public:
    EventReaderThread(int fd, const std::string& devNode, KeyCallback callback);
    ~EventReaderThread();

    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    const std::string& getDevNode() const { return m_devNode; }

private:
    void run();

    int m_fd;
    std::string m_devNode;
    KeyCallback m_callback;
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_stopRequested;
};

/// Linux input hook implementation using evdev
class InputHookLinux : public IInputHook {
public:
    InputHookLinux();
    ~InputHookLinux() override;

    bool install(KeyCallback keyCallback, MouseCallback mouseCallback) override;
    void uninstall() override;
    bool isInstalled() const override { return m_isInstalled; }

private:
    void cleanup();

    KeyCallback m_keyCallback;
    MouseCallback m_mouseCallback;
    bool m_isInstalled;

    DeviceManager m_deviceManager;
    std::vector<OpenDevice> m_openDevices;
    std::vector<std::unique_ptr<EventReaderThread>> m_readerThreads;
    std::mutex m_readerThreadsMutex;
};

} // namespace yamy::platform
