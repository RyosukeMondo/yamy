// Simple tool to test modal modifier behavior
// Usage: test_modal_modifier <modifier_scancode> <test_key_scancode>
// Example: test_modal_modifier 48 17  (Hold B=48, press W=17)

#include <iostream>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <cstring>

void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    if (write(fd, &ie, sizeof(ie)) < 0) {
        std::cerr << "Error writing event: " << strerror(errno) << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <modifier_key> <test_key>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 48 17  (Hold B=48, press W=17)" << std::endl;
        return 1;
    }

    int modifier_key = std::stoi(argv[1]);
    int test_key = std::stoi(argv[2]);

    // Open uinput device
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/uinput: " << strerror(errno) << std::endl;
        return 1;
    }

    // Enable key events
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < 256; i++) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    // Create virtual device
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "YAMY Test Device");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0x5678;
    uidev.id.version = 1;

    if (write(fd, &uidev, sizeof(uidev)) < 0) {
        std::cerr << "Error writing device info" << std::endl;
        close(fd);
        return 1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        std::cerr << "Error creating device" << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "Simulating: Hold key " << modifier_key << ", press key " << test_key << std::endl;

    sleep(1);  // Give YAMY time to see the device

    // Press modifier
    emit(fd, EV_KEY, modifier_key, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(50000);  // 50ms

    // Press test key
    emit(fd, EV_KEY, test_key, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(50000);

    // Release test key
    emit(fd, EV_KEY, test_key, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(50000);

    // Release modifier
    emit(fd, EV_KEY, modifier_key, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);

    std::cout << "Test sequence complete. Check YAMY output." << std::endl;

    // Cleanup
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}
