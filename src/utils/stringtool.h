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
#else
#  include <strings.h>
#  define strcasecmp_platform strcasecmp
#endif

// Backward compatibility: tstring is now always std::string (UTF-8)
using tstring = std::string;
using tistream = std::istream;
using tostream = std::ostream;
using tstreambuf = std::streambuf;
using tstringstream = std::stringstream;
using tifstream = std::ifstream;
using tofstream = std::ofstream;

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

// Backward compatibility: tregex is now Regex
using tregex = Regex;
using tsmatch = std::smatch;

/// stream output for Regex
std::ostream& operator<<(std::ostream& i_ost, const Regex& i_data);

/// string with custom stream output (escaped/quoted)
class tstringq : public std::string
{
public:
    tstringq() { }
    tstringq(const tstringq& i_str) : std::string(i_str) { }
    tstringq(const std::string& i_str) : std::string(i_str) { }
    tstringq(const char* i_str) : std::string(i_str) { }
    tstringq(const char* i_str, size_t i_n) : std::string(i_str, i_n) { }
    tstringq(const char* i_str, size_t i_pos, size_t i_n)
            : std::string(i_str + i_pos, i_n) { }
    tstringq(size_t i_n, char i_c) : std::string(i_n, i_c) { }
};

/// stream output
std::ostream& operator<<(std::ostream& i_ost, const tstringq& i_data);


/// interpret meta characters such as \n
std::string interpretMetaCharacters(const char* i_str, size_t i_len,
                                    const char* i_quote = nullptr,
                                    bool i_doesUseRegexpBackReference = false);


/// add session id to i_str
std::string addSessionId(const char* i_str);

/// copy
size_t strlcpy(char* o_dest, const char* i_src, size_t i_destSize);
/// copy
size_t mbslcpy(unsigned char* o_dest, const unsigned char* i_src,
               size_t i_destSize);
/// copy
size_t wcslcpy(wchar_t* o_dest, const wchar_t* i_src, size_t i_destSize);
/// copy
inline size_t tcslcpy(char* o_dest, const char* i_src, size_t i_destSize)
{
    return strlcpy(o_dest, i_src, i_destSize);
}
/// copy
inline size_t tcslcpy(unsigned char* o_dest, const unsigned char* i_src,
                      size_t i_destSize)
{
    return mbslcpy(o_dest, i_src, i_destSize);
}
/// copy
inline size_t tcslcpy(wchar_t* o_dest, const wchar_t* i_src, size_t i_destSize)
{
    return wcslcpy(o_dest, i_src, i_destSize);
}

// escape regexp special characters in MBCS trail bytes
std::string guardRegexpFromMbcs(const char* i_str);

/// converter
std::wstring to_wstring(const std::string& i_str);
/// converter
std::string to_string(const std::wstring& i_str);

// convert wstring to UTF-8
std::string to_UTF_8(const std::wstring& i_str);

// convert string to UTF-8 (passthrough for UTF-8 input)
inline std::string to_UTF_8(const std::string& i_str) {
    return i_str;
}

/// internal string type (UTF-8)
using ustring = std::string;

/// helper to convert ustring to tstring (identity for UTF-8)
inline tstring to_tstring(const ustring& i_str) {
    return i_str;
}


/// case insensitive string
class tstringi : public std::string
{
public:
    tstringi() { }
    tstringi(const tstringi& i_str) : std::string(i_str) { }
    tstringi(const std::string& i_str) : std::string(i_str) { }
    tstringi(const char* i_str) : std::string(i_str) { }
    tstringi(const char* i_str, size_t i_n) : std::string(i_str, i_n) { }
    tstringi(const char* i_str, size_t i_pos, size_t i_n)
            : std::string(i_str + i_pos, i_n) { }
    tstringi(size_t i_n, char i_c) : std::string(i_n, i_c) { }

    int compare(const tstringi& i_str) const {
        return compare(i_str.c_str());
    }
    int compare(const std::string& i_str) const {
        return compare(i_str.c_str());
    }
    int compare(const char* i_str) const {
        return strcasecmp_platform(c_str(), i_str);
    }
    std::string& getString() {
        return *this;
    }
    const std::string& getString() const {
        return *this;
    }
};

/// case insensitive string comparison
inline bool operator<(const tstringi& i_str1, const char* i_str2)
{
    return i_str1.compare(i_str2) < 0;
}
/// case insensitive string comparison
inline bool operator<(const char* i_str1, const tstringi& i_str2)
{
    return 0 < i_str2.compare(i_str1);
}
/// case insensitive string comparison
inline bool operator<(const tstringi& i_str1, const std::string& i_str2)
{
    return i_str1.compare(i_str2) < 0;
}
/// case insensitive string comparison
inline bool operator<(const std::string& i_str1, const tstringi& i_str2)
{
    return 0 < i_str2.compare(i_str1);
}
/// case insensitive string comparison
inline bool operator<(const tstringi& i_str1, const tstringi& i_str2)
{
    return i_str1.compare(i_str2) < 0;
}

/// case insensitive string comparison
inline bool operator==(const char* i_str1, const tstringi& i_str2)
{
    return i_str2.compare(i_str1) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi& i_str1, const char* i_str2)
{
    return i_str1.compare(i_str2) == 0;
}
/// case insensitive string comparison
inline bool operator==(const std::string& i_str1, const tstringi& i_str2)
{
    return i_str2.compare(i_str1) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi& i_str1, const std::string& i_str2)
{
    return i_str1.compare(i_str2) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi& i_str1, const tstringi& i_str2)
{
    return i_str1.compare(i_str2) == 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// workarounds for Borland C++


/// case insensitive string comparison
inline bool operator!=(const char* i_str1, const tstringi& i_str2)
{
    return i_str2.compare(i_str1) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi& i_str1, const char* i_str2)
{
    return i_str1.compare(i_str2) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const std::string& i_str1, const tstringi& i_str2)
{
    return i_str2.compare(i_str1) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi& i_str1, const std::string& i_str2)
{
    return i_str1.compare(i_str2) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi& i_str1, const tstringi& i_str2)
{
    return i_str1.compare(i_str2) != 0;
}


/// get lower string
std::string toLower(const std::string& i_str);

// Case-insensitive string comparison for std::string (UTF-8)
int strcasecmp_utf8(const char* s1, const char* s2);

#endif // !_STRINGTOOL_H
