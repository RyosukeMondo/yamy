#include "utf_conversion.h"
#include <windows.h>

namespace yamy::platform {

std::wstring utf8_to_wstring(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), nullptr, 0);
    if (size_needed <= 0) {
        return std::wstring();
    }

    std::wstring result(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), &result[0], size_needed);
    return result;
}

std::wstring utf8_to_wstring(const char* utf8) {
    if (!utf8 || *utf8 == '\0') {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (size_needed <= 1) { // 1 because it includes null terminator
        return std::wstring();
    }

    // size_needed includes null terminator.
    std::wstring result(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &result[0], size_needed);
    result.resize(size_needed - 1); // Remove the null terminator
    return result;
}

std::string wstring_to_utf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.length(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
        return std::string();
    }

    std::string result(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.length(), &result[0], size_needed, nullptr, nullptr);
    return result;
}

std::string wstring_to_utf8(const wchar_t* wide) {
    if (!wide || *wide == L'\0') {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 1) {
        return std::string();
    }

    std::string result(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], size_needed, nullptr, nullptr);
    result.resize(size_needed - 1); // Remove the null terminator
    return result;
}

} // namespace yamy::platform
