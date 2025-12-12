// platform_paths_win32.cpp - Windows implementation of path utilities

#include "../../core/platform/platform_paths.h"
#include "windowstool.h"
#include "misc.h"
#include <windows.h>

namespace yamy {
namespace platform {

std::string getExecutableDirectory()
{
    char buf[GANA_MAX_PATH];
    if (GetModuleFileName(GetModuleHandle(nullptr), buf, NUMBER_OF(buf))) {
        return pathRemoveFileSpec(buf);
    }
    return "";
}

} // namespace platform
} // namespace yamy
