//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// registry.cpp


#include "registry.h"
#include "stringtool.h"
#include "array.h"
#include "utf_conversion.h"
#include <malloc.h>
#include <vector>

using namespace yamy::platform;

void Registry::setRoot(HKEY i_root, const std::string &i_path) {
    m_root = i_root;
    if (m_root) {
        m_path = i_path;
    } else {
        wchar_t exePath[MAX_PATH];
        wchar_t exeDrive[MAX_PATH];
        wchar_t exeDir[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        _wsplitpath_s(exePath, exeDrive, MAX_PATH, exeDir, MAX_PATH, nullptr, 0, nullptr, 0);
        m_path = wstring_to_utf8(exeDrive);
        m_path += wstring_to_utf8(exeDir);
        m_path += "yamy.ini";
    }
}

// remove
bool Registry::remove(HKEY i_root, const std::string &i_path,
                      const std::string &i_name)
{
    if (i_root) {
        if (i_name.empty()) {
            std::wstring wpath = utf8_to_wstring(i_path);
            return RegDeleteKeyW(i_root, wpath.c_str()) == ERROR_SUCCESS;
        }
        HKEY hkey;
        std::wstring wpath = utf8_to_wstring(i_path);
        if (ERROR_SUCCESS != RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_SET_VALUE, &hkey))
            return false;
        std::wstring wname = utf8_to_wstring(i_name);
        LONG r = RegDeleteValueW(hkey, wname.c_str());
        RegCloseKey(hkey);
        return r == ERROR_SUCCESS;
    } else {
        if (i_name.empty())
            return false;
        std::wstring wname = utf8_to_wstring(i_name);
        std::wstring wpath = utf8_to_wstring(i_path);
        return WritePrivateProfileStringW(L"yamy", wname.c_str(), nullptr, wpath.c_str()) == TRUE;
    }
}


// does exist the key ?
bool Registry::doesExist(HKEY i_root, const std::string &i_path)
{
    if (i_root) {
        HKEY hkey;
        std::wstring wpath = utf8_to_wstring(i_path);
        if (ERROR_SUCCESS != RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_READ, &hkey))
            return false;
        RegCloseKey(hkey);
        return true;
    } else {
        return true;
    }
}


// read DWORD
bool Registry::read(HKEY i_root, const std::string &i_path,
                    const std::string &i_name, int *o_value, int i_defaultValue)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    if (i_root) {
        HKEY hkey;
        if (ERROR_SUCCESS == RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_READ, &hkey)) {
            DWORD type = REG_DWORD;
            DWORD size = sizeof(*o_value);
            LONG r = RegQueryValueExW(hkey, wname.c_str(), nullptr, &type, (BYTE *)o_value, &size);
            RegCloseKey(hkey);
            if (r == ERROR_SUCCESS)
                return true;
        }
        *o_value = i_defaultValue;
        return false;
    } else {
        *o_value = GetPrivateProfileIntW(L"yamy", wname.c_str(), i_defaultValue, wpath.c_str());
        return true;
    }
}


// write DWORD
bool Registry::write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     int i_value)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    if (i_root) {
        HKEY hkey;
        DWORD disposition;
        if (ERROR_SUCCESS !=
            RegCreateKeyExW(i_root, wpath.c_str(), 0, (LPWSTR)L"",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS, nullptr, &hkey, &disposition))
            return false;
        LONG r = RegSetValueExW(hkey, wname.c_str(), 0, REG_DWORD,
                               (BYTE *)&i_value, sizeof(i_value));
        RegCloseKey(hkey);
        return r == ERROR_SUCCESS;
    } else {
        DWORD ret;
        wchar_t buf[MAX_PATH];

        swprintf_s(buf, MAX_PATH, L"%d", i_value);
        ret =  WritePrivateProfileStringW(L"yamy", wname.c_str(),
                                         buf, wpath.c_str());
        return ret != 0;
    }
}


// read string
bool Registry::read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                    std::string *o_value, const std::string &i_defaultValue)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    if (i_root) {
        HKEY hkey;
        if (ERROR_SUCCESS ==
                RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_READ, &hkey)) {
            DWORD type = REG_SZ;
            DWORD size = 0;
            BYTE dummy;
            if (ERROR_MORE_DATA ==
                    RegQueryValueExW(hkey, wname.c_str(), nullptr, &type, &dummy, &size)) {
                if (0 < size) {
                    Array<BYTE> buf(size + sizeof(wchar_t)); // Allocate extra for null terminator
                    if (ERROR_SUCCESS == RegQueryValueExW(hkey, wname.c_str(),
                                                         nullptr, &type, buf.get(), &size)) {
                        // Ensure null termination
                        buf[size] = 0;
                        buf[size + 1] = 0; // Assuming sizeof(wchar_t) >= 2

                        wchar_t* wideStr = reinterpret_cast<wchar_t*>(buf.get());
                        *o_value = wstring_to_utf8(wideStr);
                        RegCloseKey(hkey);
                        return true;
                    }
                }
            }
            RegCloseKey(hkey);
        }
        if (!i_defaultValue.empty())
            *o_value = i_defaultValue;
        return false;
    } else {
        wchar_t buf[MAX_PATH];
        DWORD len;
        len = GetPrivateProfileStringW(L"yamy", wname.c_str(), L"",
                                      buf, sizeof(buf) / sizeof(buf[0]), wpath.c_str());
        if (len > 0) {
            *o_value = wstring_to_utf8(buf);
            return true;
        }
        if (!i_defaultValue.empty())
            *o_value = i_defaultValue;
        return false;
    }
}


// write string
bool Registry::write(HKEY i_root, const std::string &i_path,
                     const std::string &i_name, const std::string &i_value)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);
    std::wstring wvalue = utf8_to_wstring(i_value);

    if (i_root) {
        HKEY hkey;
        DWORD disposition;
        if (ERROR_SUCCESS !=
                RegCreateKeyExW(i_root, wpath.c_str(), 0, (LPWSTR)L"",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS, nullptr, &hkey, &disposition))
            return false;
        RegSetValueExW(hkey, wname.c_str(), 0, REG_SZ,
                      (BYTE *)wvalue.c_str(),
                      (DWORD)((wvalue.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hkey);
        return true;
    } else {
        DWORD ret;

        ret =  WritePrivateProfileStringW(L"yamy", wname.c_str(),
                                         wvalue.c_str(), wpath.c_str());
        return ret != 0;
    }
}


#ifndef USE_INI
// read list of string
bool Registry::read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                    strings *o_value, const strings &i_defaultValue)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    HKEY hkey;
    if (ERROR_SUCCESS ==
            RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_READ, &hkey)) {
        DWORD type = REG_MULTI_SZ;
        DWORD size = 0;
        BYTE dummy;
        if (ERROR_MORE_DATA ==
                RegQueryValueExW(hkey, wname.c_str(), nullptr, &type, &dummy, &size)) {
            if (0 < size) {
                Array<BYTE> buf(size + 2 * sizeof(wchar_t)); // Allocate extra for double null
                if (ERROR_SUCCESS == RegQueryValueExW(hkey, wname.c_str(),
                                                     nullptr, &type, buf.get(), &size)) {
                    // Ensure double null termination for REG_MULTI_SZ
                    buf[size] = 0;
                    buf[size+1] = 0;
                    buf[size+2] = 0;
                    buf[size+3] = 0;

                    o_value->clear();
                    const wchar_t *p = reinterpret_cast<wchar_t *>(buf.get());
                    const wchar_t *end = reinterpret_cast<wchar_t *>(buf.get() + size);

                    while (p < end && *p) {
                        o_value->push_back(wstring_to_utf8(p));
                        p += wcslen(p) + 1;
                    }
                    RegCloseKey(hkey);
                    return true;
                }
            }
        }
        RegCloseKey(hkey);
    }
    if (!i_defaultValue.empty())
        *o_value = i_defaultValue;
    return false;
}


// write list of string
bool Registry::write(HKEY i_root, const std::string &i_path,
                     const std::string &i_name, const strings &i_value)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    HKEY hkey;
    DWORD disposition;
    if (ERROR_SUCCESS !=
            RegCreateKeyExW(i_root, wpath.c_str(), 0, (LPWSTR)L"",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS, nullptr, &hkey, &disposition))
        return false;

    std::wstring value;
    for (const auto& s : i_value) {
        value += utf8_to_wstring(s);
        value += L'\0';
    }
    value += L'\0'; // Double null termination

    RegSetValueExW(hkey, wname.c_str(), 0, REG_MULTI_SZ,
                  (BYTE *)value.c_str(),
                  (DWORD)(value.size() * sizeof(wchar_t))); // size() includes the nulls we added
    RegCloseKey(hkey);
    return true;
}
#endif //!USE_INI


// read binary
bool Registry::read(HKEY i_root, const std::string &i_path,
                    const std::string &i_name, BYTE *o_value, DWORD *i_valueSize,
                    const BYTE *i_defaultValue, DWORD i_defaultValueSize)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    if (i_root) {
        if (i_valueSize) {
            HKEY hkey;
            if (ERROR_SUCCESS ==
                    RegOpenKeyExW(i_root, wpath.c_str(), 0, KEY_READ, &hkey)) {
                DWORD type = REG_BINARY;
                LONG r = RegQueryValueExW(hkey, wname.c_str(), nullptr, &type,
                                         (BYTE *)o_value, i_valueSize);
                RegCloseKey(hkey);
                if (r == ERROR_SUCCESS)
                    return true;
            }
        }
        if (i_defaultValue)
            CopyMemory(o_value, i_defaultValue,
                       MIN(i_defaultValueSize, *i_valueSize));
        return false;
    } else {
        return false;
    }
}


// write binary
bool Registry::write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     const BYTE *i_value, DWORD i_valueSize)
{
    std::wstring wpath = utf8_to_wstring(i_path);
    std::wstring wname = utf8_to_wstring(i_name);

    if (i_root) {
        if (!i_value)
            return false;
        HKEY hkey;
        DWORD disposition;
        if (ERROR_SUCCESS !=
                RegCreateKeyExW(i_root, wpath.c_str(), 0, (LPWSTR)L"",
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS, nullptr, &hkey, &disposition))
            return false;
        RegSetValueExW(hkey, wname.c_str(), 0, REG_BINARY, i_value, i_valueSize);
        RegCloseKey(hkey);
        return true;
    } else {
        return false;
    }
}


//
static bool string2logfont(LOGFONT *o_lf, const std::string &i_strlf)
{
    // -13,0,0,0,400,0,0,0,128,1,2,1,1,Terminal
    // Using standard regex for UTF-8 string
    std::regex lf("^(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),"
                  "(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),"
                  "(-?\\d+),(-?\\d+),(-?\\d+),(.+)$");
    std::smatch what;

    if (!std::regex_match(i_strlf, what, lf))
        return false;
    o_lf->lfHeight         =       atoi(what.str(1).c_str());
    o_lf->lfWidth          =       atoi(what.str(2).c_str());
    o_lf->lfEscapement     =       atoi(what.str(3).c_str());
    o_lf->lfOrientation    =       atoi(what.str(4).c_str());
    o_lf->lfWeight         =       atoi(what.str(5).c_str());
    o_lf->lfItalic         = (BYTE)atoi(what.str(6).c_str());
    o_lf->lfUnderline      = (BYTE)atoi(what.str(7).c_str());
    o_lf->lfStrikeOut      = (BYTE)atoi(what.str(8).c_str());
    o_lf->lfCharSet        = (BYTE)atoi(what.str(9).c_str());
    o_lf->lfOutPrecision   = (BYTE)atoi(what.str(10).c_str());
    o_lf->lfClipPrecision  = (BYTE)atoi(what.str(11).c_str());
    o_lf->lfQuality        = (BYTE)atoi(what.str(12).c_str());
    o_lf->lfPitchAndFamily = (BYTE)atoi(what.str(13).c_str());

    std::string faceName = what.str(14);
    std::wstring wFaceName = utf8_to_wstring(faceName);
    wcscpy_s(o_lf->lfFaceName, LF_FACESIZE, wFaceName.c_str());

    return true;
}


// read LOGFONT
bool Registry::read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                    LOGFONT *o_value, const std::string &i_defaultStringValue)
{
    std::string buf;
    if (!read(i_root, i_path, i_name, &buf) || !string2logfont(o_value, buf)) {
        if (!i_defaultStringValue.empty())
            string2logfont(o_value, i_defaultStringValue);
        return false;
    }
    return true;
}


// write LOGFONT
bool Registry::write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     const LOGFONT &i_value)
{
    char buf[1024];
    std::string faceName = wstring_to_utf8(i_value.lfFaceName);

    snprintf(buf, sizeof(buf),
               "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%s",
               i_value.lfHeight, i_value.lfWidth, i_value.lfEscapement,
               i_value.lfOrientation, i_value.lfWeight, (int)i_value.lfItalic,
               (int)i_value.lfUnderline, (int)i_value.lfStrikeOut, (int)i_value.lfCharSet,
               (int)i_value.lfOutPrecision, (int)i_value.lfClipPrecision,
               (int)i_value.lfQuality,
               (int)i_value.lfPitchAndFamily, faceName.c_str());
    return Registry::write(i_root, i_path, i_name, std::string(buf));
}
