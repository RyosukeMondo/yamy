#include "core/platform/input_injector_interface.h"
#include "core/platform/platform_exception.h"
#include "core/input/input_event.h"
#include "keycode_mapping.h"
#include "../../utils/logger.h"
#include "../../utils/metrics.h"
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <cerrno>
#include <chrono>

namespace yamy::platform {

class InputInjectorLinux : public IInputInjector {
public:
    InputInjectorLinux(IWindowSystem* windowSystem)
        : m_windowSystem(windowSystem), m_fd(-1), m_wheelAccumulator(0) {
        initializeUinput();
    }

    ~InputInjectorLinux() override {
        if (m_fd >= 0) {
            ioctl(m_fd, UI_DEV_DESTROY);
            close(m_fd);
            LOG_INFO("[injector] Destroyed uinput virtual device");
        }
    }

    // Keyboard
    void keyDown(KeyCode key) override {
        sendKeyEvent(key, 1);
    }

    void keyUp(KeyCode key) override {
        sendKeyEvent(key, 0);
    }

    // Mouse
    void mouseMove(int32_t dx, int32_t dy) override {
        if (m_fd < 0) return;

        if (dx != 0) writeEvent(EV_REL, REL_X, dx);
        if (dy != 0) writeEvent(EV_REL, REL_Y, dy);
        if (dx != 0 || dy != 0) writeEvent(EV_SYN, SYN_REPORT, 0);
    }

    void mouseButton(MouseButton button, bool down) override {
        if (m_fd < 0) return;

        uint16_t btnCode = 0;
        switch (button) {
            case MouseButton::Left:   btnCode = BTN_LEFT; break;
            case MouseButton::Right:  btnCode = BTN_RIGHT; break;
            case MouseButton::Middle: btnCode = BTN_MIDDLE; break;
            case MouseButton::X1:     btnCode = BTN_SIDE; break;
            case MouseButton::X2:     btnCode = BTN_EXTRA; break;
            default: return;
        }

        writeEvent(EV_KEY, btnCode, down ? 1 : 0);
        writeEvent(EV_SYN, SYN_REPORT, 0);
    }

    void mouseWheel(int32_t delta) override {
        if (m_fd < 0) return;

        // Windows uses 120 per notch. Linux uses 1.
        m_wheelAccumulator += delta;

        int32_t steps = m_wheelAccumulator / 120;
        if (steps != 0) {
            writeEvent(EV_REL, REL_WHEEL, steps);
            writeEvent(EV_SYN, SYN_REPORT, 0);

            // Keep the remainder for future accumulation
            m_wheelAccumulator %= 120;
        }
    }

    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) override {
        (void)rawData;
        (void)ctx;

        if (!data || m_fd < 0) {
            return;
        }

        auto injectStart = std::chrono::high_resolution_clock::now();

        // Check for mouse events (E1 flag indicates mouse event)
        if (data->Flags & KEYBOARD_INPUT_DATA::E1) {
            bool isKeyUp = data->Flags & KEYBOARD_INPUT_DATA::BREAK;

            switch (data->MakeCode) {
                case 1: // Left button
                    mouseButton(MouseButton::Left, !isKeyUp);
                    break;
                case 2: // Right button
                    mouseButton(MouseButton::Right, !isKeyUp);
                    break;
                case 3: // Middle button
                    mouseButton(MouseButton::Middle, !isKeyUp);
                    break;
                case 4: // Wheel up
                    if (!isKeyUp) mouseWheel(120);
                    break;
                case 5: // Wheel down
                    if (!isKeyUp) mouseWheel(-120);
                    break;
                case 6: // X1 button
                    mouseButton(MouseButton::X1, !isKeyUp);
                    break;
                case 7: // X2 button
                    mouseButton(MouseButton::X2, !isKeyUp);
                    break;
                case 8: // HWheel right (treat as vertical wheel for now)
                    if (!isKeyUp) mouseWheel(120);
                    break;
                case 9: // HWheel left
                    if (!isKeyUp) mouseWheel(-120);
                    break;
                case 10: // Generic wheel with delta in ExtraInformation
                    if (!isKeyUp) mouseWheel(static_cast<int32_t>(data->ExtraInformation));
                    break;
                default:
                    break;
            }
        } else {
            // Keyboard event
            // MakeCode contains VK code (YAMY key code) on Linux path
            static bool debug_logging = (std::getenv("YAMY_DEBUG_KEYCODE") != nullptr);

            if (debug_logging) {
                LOG_INFO("[injector] [LAYER2:OUT] Engine output code = 0x{:04X} ({})",
                         data->MakeCode, data->MakeCode);
            }

            uint16_t evdevCode = yamyToEvdevKeyCode(data->MakeCode);

            if (evdevCode == 0 && data->MakeCode != 0) {
                // Unknown key code, skip
                LOG_WARN("[injector] Unknown MakeCode={} cannot convert to evdev", data->MakeCode);
                return;
            }

            bool isKeyUp = data->Flags & KEYBOARD_INPUT_DATA::BREAK;
            int value = isKeyUp ? 0 : 1;

            writeEvent(EV_KEY, evdevCode, value);
            writeEvent(EV_SYN, SYN_REPORT, 0);
        }

        // Record injection latency
        auto injectEnd = std::chrono::high_resolution_clock::now();
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            injectEnd - injectStart).count();
        yamy::metrics::PerformanceMetrics::instance().recordLatency(
            yamy::metrics::Operations::INPUT_INJECTION, static_cast<uint64_t>(durationNs));
    }

private:
    IWindowSystem* m_windowSystem;
    int m_fd;
    int32_t m_wheelAccumulator;

    /// Check if uinput is available on this system
    static bool checkUinputAvailable() {
        struct stat st;
        if (stat("/dev/uinput", &st) != 0) {
            return false;
        }
        // Check if we can open it (write access)
        int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            return false;
        }
        close(fd);
        return true;
    }

    void initializeUinput() {
        // Check if /dev/uinput exists
        struct stat st;
        if (stat("/dev/uinput", &st) != 0) {
            int err = errno;
            LOG_ERROR("[injector] /dev/uinput not found: {}", std::strerror(err));
            throw UinputUnavailableException(err, std::strerror(err));
        }

        m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (m_fd < 0) {
            int err = errno;
            LOG_ERROR("[injector] Failed to open /dev/uinput: {}", std::strerror(err));
            throw UinputUnavailableException(err, std::strerror(err));
        }

        // Enable Key Events
        if (ioctl(m_fd, UI_SET_EVBIT, EV_KEY) < 0) {
            int err = errno;
            LOG_ERROR("[injector] Failed to set EV_KEY: {}", std::strerror(err));
            close(m_fd);
            m_fd = -1;
            throw UinputUnavailableException(err, "ioctl UI_SET_EVBIT failed: " + std::string(std::strerror(err)));
        }

        // Enable mapped keys
        // We enable a standard range covering common keys and extended keys
        for (int i = 0; i < 256; ++i) {
             ioctl(m_fd, UI_SET_KEYBIT, i);
        }
        for (int i = 0x100; i < 0x200; ++i) { // Extended keys
             ioctl(m_fd, UI_SET_KEYBIT, i);
        }
        // Mouse buttons
        ioctl(m_fd, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(m_fd, UI_SET_KEYBIT, BTN_RIGHT);
        ioctl(m_fd, UI_SET_KEYBIT, BTN_MIDDLE);
        ioctl(m_fd, UI_SET_KEYBIT, BTN_SIDE);
        ioctl(m_fd, UI_SET_KEYBIT, BTN_EXTRA);

        // Enable Relative Events (Mouse)
        ioctl(m_fd, UI_SET_EVBIT, EV_REL);
        ioctl(m_fd, UI_SET_RELBIT, REL_X);
        ioctl(m_fd, UI_SET_RELBIT, REL_Y);
        ioctl(m_fd, UI_SET_RELBIT, REL_WHEEL);

        // Enable Sync Events
        ioctl(m_fd, UI_SET_EVBIT, EV_SYN);

        struct uinput_user_dev uidev;
        memset(&uidev, 0, sizeof(uidev));

        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Yamy Virtual Input Device");
        uidev.id.bustype = BUS_USB;
        uidev.id.vendor  = 0x1;
        uidev.id.product = 0x1;
        uidev.id.version = 1;

        if (write(m_fd, &uidev, sizeof(uidev)) < 0) {
            int err = errno;
            LOG_ERROR("[injector] Failed to write uinput device config: {}", std::strerror(err));
            close(m_fd);
            m_fd = -1;
            throw UinputUnavailableException(err, "Failed to write uinput device config: " + std::string(std::strerror(err)));
        }

        if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
            int err = errno;
            LOG_ERROR("[injector] Failed to create uinput device: {}", std::strerror(err));
            close(m_fd);
            m_fd = -1;
            throw UinputUnavailableException(err, "Failed to create uinput device: " + std::string(std::strerror(err)));
        }

        LOG_INFO("[injector] Virtual input device created successfully");
    }

    void sendKeyEvent(KeyCode key, int value) {
        if (m_fd < 0) return;

        // Convert Yamy KeyCode to Linux Evdev code
        uint16_t evdevCode = yamyToEvdevKeyCode(static_cast<uint16_t>(key));

        if (evdevCode == 0 && static_cast<uint32_t>(key) != 0) {
            return;
        }

        writeEvent(EV_KEY, evdevCode, value);
        writeEvent(EV_SYN, SYN_REPORT, 0);
    }

    void writeEvent(uint16_t type, uint16_t code, int32_t value) {
        struct input_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = type;
        ev.code = code;
        ev.value = value;

        if (write(m_fd, &ev, sizeof(ev)) < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("[injector] Failed to write event (type={} code={}): {}",
                          type, code, std::strerror(errno));
            }
        }
    }
};

IInputInjector* createInputInjector(IWindowSystem* windowSystem) {
    return new InputInjectorLinux(windowSystem);
}

} // namespace yamy::platform
