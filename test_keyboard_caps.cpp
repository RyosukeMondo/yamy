#include <iostream>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>

#define NBITS(x) ((((x)-1)/8)+1)
#define test_bit(bit, array) ((array)[(bit)/8] & (1<<((bit)%8)))

int main() {
    const char* devNode = "/dev/input/event16";

    int fd = open(devNode, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Cannot open " << devNode << ": " << strerror(errno) << std::endl;
        return 1;
    }

    // Get name
    char name[256] = "Unknown";
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    std::cout << "Device: " << name << std::endl;

    // Check EV_KEY capability
    unsigned long evBits[NBITS(EV_MAX)] = {0};
    int ret = ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits);
    std::cout << "EVIOCGBIT(0) returned: " << ret << std::endl;
    std::cout << "Has EV_KEY: " << test_bit(EV_KEY, evBits) << std::endl;

    // Check specific keys
    unsigned long keyBits[NBITS(KEY_MAX)] = {0};
    ret = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);
    std::cout << "EVIOCGBIT(EV_KEY) returned: " << ret << std::endl;

    std::cout << "\nChecking specific keys:" << std::endl;
    std::cout << "  KEY_A (" << KEY_A << "): " << test_bit(KEY_A, keyBits) << std::endl;
    std::cout << "  KEY_Z (" << KEY_Z << "): " << test_bit(KEY_Z, keyBits) << std::endl;
    std::cout << "  KEY_ENTER (" << KEY_ENTER << "): " << test_bit(KEY_ENTER, keyBits) << std::endl;
    std::cout << "  KEY_SPACE (" << KEY_SPACE << "): " << test_bit(KEY_SPACE, keyBits) << std::endl;
    std::cout << "  KEY_ESC (" << KEY_ESC << "): " << test_bit(KEY_ESC, keyBits) << std::endl;
    std::cout << "  KEY_1 (" << KEY_1 << "): " << test_bit(KEY_1, keyBits) << std::endl;

    close(fd);
    return 0;
}
