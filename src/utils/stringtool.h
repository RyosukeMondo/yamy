#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// stringtool.h
// Cross-platform string utilities using UTF-8 std::string


#ifndef _STRINGTOOL_H
#  define _STRINGTOOL_H

#  include "misc.h"
#  include <cctype>
#  include <cstring>
#  include <string>
#  include <iosfwd>
#  include <fstream>
#  include <sstream>
#  include <locale>
#  include <regex>
#  include <stdio.h>                // for snprintf

// Platform-independent case-insensitive string comparison
#ifdef _WIN32
#  include <string.h>
#  define strcasecmp_platform _stricmp
#  define tcslcpy wcslcpy

// On Windows, these are wide-character types
typedef std::wstring tstring;
typedef std::wifstream tifstream;
typedef std::wstringstream tstringstream;
typedef std::wostream tostream;
typedef std::wstring tstringi; // Treating tstringi as tstring for now
#else
#  include <strings.h>
#  define strcasecmp_platform strcasecmp
#  define tcslcpy strlcpy

// On Linux, use standard types directly (no tstring abstraction)
typedef std::ifstream tifstream;
typedef std::stringstream tstringstream;
typedef std::ostream tostream;
#endif

// Regex wrapper for std::string (UTF-8)
class Regex : public std::regex
{
public:
    typedef std::regex base_type;
    typedef base_type::flag_type flag_type;

    static const flag_type normal = std::regex_constants::ECMAScript;
    static const flag_type icase = std::regex_constants::icase;
    static const flag_type nosubs = std::regex_constants::nosubs;
    static const flag_type optimize = std::regex_constants::optimize;
    static const flag_type collate = std::regex_constants::collate;

private:
    std::string m_pattern;

public:
    Regex() : base_type() {}
    Regex(const char* p, flag_type f = normal) : base_type(p, f), m_pattern(p) {}
    Regex(const std::string& s, flag_type f = normal) : base_type(s, f), m_pattern(s) {}
    Regex(const Regex& other) : base_type(other), m_pattern(other.m_pattern) {}
    Regex& operator=(const Regex& other) {
        base_type::operator=(other);
        m_pattern = other.m_pattern;
        return *this;
    }
    void assign(const std::string& s, flag_type f = normal) {
        base_type::assign(s, f);
        m_pattern = s;
    }
    const std::string& str() const { return m_pattern; }
};

/// stream output for Regex
std::ostream& operator<<(std::ostream& i_ost, const Regex& i_data);

/// string with custom stream output (escaped/quoted)
class stringq : public std::string
{
public:
    stringq() { }
    stringq(const stringq& i_str) : std::string(i_str) { }
    stringq(const std::string& i_str) : std::string(i_str) { }
    stringq(const char* i_str) : std::string(i_str) { }
    stringq(const char* i_str, size_t i_n) : std::string(i_str, i_n) { }
    stringq(const char* i_str, size_t i_pos, size_t i_n)
            : std::string(i_str + i_pos, i_n) { }
    stringq(size_t i_n, char i_c) : std::string(i_n, i_c) { }
};

/// stream output
std::ostream& operator<<(std::ostream& i_ost, const stringq& i_data);


/// interpret meta characters such as \n
std::string interpretMetaCharacters(const char* i_str, size_t i_len,
                                    const char* i_quote = nullptr,
                                    bool i_doesUseRegexpBackReference = false);


/// add session id to i_str
std::string addSessionId(const char* i_str);
std::wstring addSessionId(const wchar_t* i_str);

/// copy
size_t strlcpy(char* o_dest, const char* i_src, size_t i_destSize);
/// copy
size_t mbslcpy(unsigned char* o_dest, const unsigned char* i_src,
               size_t i_destSize);
/// copy
size_t wcslcpy(wchar_t* o_dest, const wchar_t* i_src, size_t i_destSize);
// escape regexp special characters in MBCS trail bytes
std::string guardRegexpFromMbcs(const char* i_str);

/// converter
std::wstring to_wstring(const std::string& i_str);
/// converter
std::string to_string(const std::wstring& i_str);

// Alias for tstring conversion (tstring is wstring on Windows)
#ifdef UNICODE
inline std::wstring to_tstring(const std::string& i_str) {
    return to_wstring(i_str);
}
#else
inline std::string to_tstring(const std::string& i_str) {
    return i_str;  // tstring is string when UNICODE is not defined
}
#endif

// convert wstring to UTF-8
std::string to_UTF_8(const std::wstring& i_str);

// convert string to UTF-8 (passthrough for UTF-8 input)
inline std::string to_UTF_8(const std::string& i_str) {
    return i_str;
}

/// get lower string
std::string toLower(const std::string& i_str);

// Case-insensitive string comparison for std::string (UTF-8)
int strcasecmp_utf8(const char* s1, const char* s2);

#endif // !_STRINGTOOL_H
