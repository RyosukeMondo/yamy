#include <iostream>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstring>

#define NBITS(x) ((((x)-1)/8)+1)
#define test_bit(bit, array) ((array)[(bit)/8] & (1<<((bit)%8)))

int main() {
    std::cout << "Enumerating /dev/input/event* devices:\n" << std::endl;

    DIR* dir = opendir("/dev/input");
    if (!dir) {
        std::cerr << "Failed to open /dev/input" << std::endl;
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        std::string devNode = "/dev/input/" + std::string(entry->d_name);

        // Try to open
        int fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cout << devNode << " - CANNOT OPEN: " << strerror(errno) << std::endl;
            continue;
        }

        // Get name
        char name[256] = "Unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);

        // Check for keyboard keys
        unsigned long evBits[NBITS(EV_MAX)] = {0};
        ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits);
        bool hasKeys = test_bit(EV_KEY, evBits);

        bool isKeyboard = false;
        if (hasKeys) {
            unsigned long keyBits[NBITS(KEY_MAX)] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) >= 0) {
                isKeyboard =
                    test_bit(KEY_A, keyBits) ||
                    test_bit(KEY_Z, keyBits) ||
                    test_bit(KEY_ENTER, keyBits) ||
                    test_bit(KEY_SPACE, keyBits);
            }
        }

        std::cout << devNode << " - \"" << name << "\" - "
                  << (isKeyboard ? "KEYBOARD" : "NOT KEYBOARD") << std::endl;

        close(fd);
    }

    closedir(dir);
    return 0;
}
