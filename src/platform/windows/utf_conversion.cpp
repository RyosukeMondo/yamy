#include "utf_conversion.h"
#include <windows.h>
#include <vector>

namespace yamy::platform {

std::wstring utf8_to_wstring(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    if (size_needed <= 0) {
        return std::wstring();
    }

    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring utf8_to_wstring(const char* utf8) {
    if (!utf8) return std::wstring();
    return utf8_to_wstring(std::string(utf8));
}

std::string wstring_to_utf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) {
        return std::string();
    }

    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string wstring_to_utf8(const wchar_t* wide) {
    if (!wide) return std::string();
    return wstring_to_utf8(std::wstring(wide));
}

} // namespace yamy::platform
