// platform_paths_linux.cpp - Linux implementation of path utilities

#include "platform_paths.h"
#include "misc.h"
#include <unistd.h>
#include <cstring>

namespace yamy {
namespace platform {

std::string getExecutableDirectory()
{
    char buf[GANA_MAX_PATH];
    ssize_t count = readlink("/proc/self/exe", buf, NUMBER_OF(buf) - 1);
    if (count > 0) {
        buf[count] = '\0';
        // Remove filename to get directory
        char *lastSlash = strrchr(buf, '/');
        if (lastSlash) {
            *lastSlash = '\0';
            return std::string(buf);
        }
    }
    return "";
}

} // namespace platform
} // namespace yamy
