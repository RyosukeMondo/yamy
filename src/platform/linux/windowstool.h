// windowstool.h - Linux stub for Windows utility functions

#ifndef _LINUX_WINDOWSTOOL_H
#define _LINUX_WINDOWSTOOL_H

#include <string>

// Stub for loadString - on Windows this loads a string from resources
// On Linux, we just return the application name
inline std::string loadString(unsigned int) {
    return "YAMY";
}

#endif // _LINUX_WINDOWSTOOL_H
