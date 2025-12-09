#pragma once
#include <string>

namespace yamy::platform {

std::wstring utf8_to_wstring(const std::string& utf8);
std::wstring utf8_to_wstring(const char* utf8);

std::string wstring_to_utf8(const std::wstring& wide);
std::string wstring_to_utf8(const wchar_t* wide);

} // namespace yamy::platform
