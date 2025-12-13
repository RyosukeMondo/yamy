// windowstool.h - Linux stub for Windows utility functions

#pragma once

#include <string>

// Stub for loadString - on Windows this loads a string from resources
// On Linux, we just return the application name
inline std::string loadString(unsigned int) {
    return "YAMY";
}
