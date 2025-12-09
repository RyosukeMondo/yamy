#pragma once
#include <string>

namespace yamy::platform {

// UTF-8 to UTF-16 conversion for Windows APIs
std::wstring utf8_to_wstring(const std::string& utf8);
std::wstring utf8_to_wstring(const char* utf8);

// UTF-16 to UTF-8 conversion from Windows APIs
std::string wstring_to_utf8(const std::wstring& wide);
std::string wstring_to_utf8(const wchar_t* wide);

} // namespace yamy::platform
