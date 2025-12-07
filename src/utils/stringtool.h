#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// stringtool.h


#ifndef _STRINGTOOL_H
#  define _STRINGTOOL_H

#  include "misc.h"
#  include <tchar.h>
#  include <cctype>
#  include <string>
#  include <iosfwd>
#  include <fstream>
#  include <locale>
#  include <regex>
#  include <stdio.h>                // for snprintf


/// string for generic international text
typedef std::basic_string<_TCHAR> tstring;
/// istream for generic international text
typedef std::basic_istream<_TCHAR> tistream;
/// ostream for generic international text
typedef std::basic_ostream<_TCHAR> tostream;
/// streambuf for for generic international text
typedef std::basic_streambuf<_TCHAR> tstreambuf;
/// stringstream for generic international text
typedef std::basic_stringstream<_TCHAR> tstringstream;
/// ifstream for generic international text
typedef std::basic_ifstream<_TCHAR> tifstream;
/// ofstream for generic international text
typedef std::basic_ofstream<_TCHAR> tofstream;
/// basic_regex for generic international text
// Inherit from std::basic_regex to allow template deduction and passing to std functions
class tregex : public std::basic_regex<_TCHAR>
{
public:
    typedef std::basic_regex<_TCHAR> base_type;
    typedef base_type::flag_type flag_type;
    
    // Map flags to std constants
    static const flag_type normal = std::regex_constants::ECMAScript;
    static const flag_type icase = std::regex_constants::icase;
    static const flag_type nosubs = std::regex_constants::nosubs;
    static const flag_type optimize = std::regex_constants::optimize;
    static const flag_type collate = std::regex_constants::collate;

private:
    tstring m_pattern;

public:
    tregex() : base_type() {}
    
    tregex(const _TCHAR* p, flag_type f = normal) : base_type(p, f), m_pattern(p) {}
    
    tregex(const tstring& s, flag_type f = normal) : base_type(s, f), m_pattern(s) {}
    
    // Copy constructor
    tregex(const tregex& other) : base_type(other), m_pattern(other.m_pattern) {}
    
    // Assignment
    tregex& operator=(const tregex& other) {
        base_type::operator=(other);
        m_pattern = other.m_pattern;
        return *this;
    }
    
    void assign(const tstring& s, flag_type f = normal) {
        base_type::assign(s, f);
        m_pattern = s;
    }
    
    const tstring& str() const { return m_pattern; }
};

/// match_results for generic international text
typedef std::match_results<tstring::const_iterator> tsmatch;


/// string with custom stream output
class tstringq : public tstring
{
public:
    ///
    tstringq() { }
    ///
    tstringq(const tstringq &i_str) : tstring(i_str) { }
    ///
    tstringq(const tstring &i_str) : tstring(i_str) { }
    ///
    tstringq(const _TCHAR *i_str) : tstring(i_str) { }
    ///
    tstringq(const _TCHAR *i_str, size_t i_n) : tstring(i_str, i_n) { }
    ///
    tstringq(const _TCHAR *i_str, size_t i_pos, size_t i_n)
            : tstring(i_str, i_pos, i_n) { }
    ///
    tstringq(size_t i_n, _TCHAR i_c) : tstring(i_n, i_c) { }
};


/// stream output
extern tostream &operator<<(tostream &i_ost, const tstringq &i_data);


/// interpret meta characters such as \n
tstring interpretMetaCharacters(const _TCHAR *i_str, size_t i_len,
                                const _TCHAR *i_quote = NULL,
                                bool i_doesUseRegexpBackReference = false);

/// add session id to i_str
tstring addSessionId(const _TCHAR *i_str);

/// copy
size_t strlcpy(char *o_dest, const char *i_src, size_t i_destSize);
/// copy
size_t mbslcpy(unsigned char *o_dest, const unsigned char *i_src,
               size_t i_destSize);
/// copy
size_t wcslcpy(wchar_t *o_dest, const wchar_t *i_src, size_t i_destSize);
/// copy
inline size_t tcslcpy(char *o_dest, const char *i_src, size_t i_destSize)
{
    return strlcpy(o_dest, i_src, i_destSize);
}
/// copy
inline size_t tcslcpy(unsigned char *o_dest, const unsigned char *i_src,
                      size_t i_destSize)
{
    return mbslcpy(o_dest, i_src, i_destSize);
}
/// copy
inline size_t tcslcpy(wchar_t *o_dest, const wchar_t *i_src, size_t i_destSize)
{
    return wcslcpy(o_dest, i_src, i_destSize);
}

// escape regexp special characters in MBCS trail bytes
std::string guardRegexpFromMbcs(const char *i_str);
/// converter
std::wstring to_wstring(const std::string &i_str);
/// converter
std::string to_string(const std::wstring &i_str);
// convert wstring to UTF-8
std::string to_UTF_8(const std::wstring &i_str);


/// case insensitive string
class tstringi : public tstring
{
public:
    ///
    tstringi() { }
    ///
    tstringi(const tstringi &i_str) : tstring(i_str) { }
    ///
    tstringi(const tstring &i_str) : tstring(i_str) { }
    ///
    tstringi(const _TCHAR *i_str) : tstring(i_str) { }
    ///
    tstringi(const _TCHAR *i_str, size_t i_n) : tstring(i_str, i_n) { }
    ///
    tstringi(const _TCHAR *i_str, size_t i_pos, size_t i_n)
            : tstring(i_str, i_pos, i_n) { }
    ///
    tstringi(size_t i_n, _TCHAR i_c) : tstring(i_n, i_c) { }
    ///
    int compare(const tstringi &i_str) const {
        return compare(i_str.c_str());
    }
    ///
    int compare(const tstring &i_str) const {
        return compare(i_str.c_str());
    }
    ///
    int compare(const _TCHAR *i_str) const {
        return _tcsicmp(c_str(), i_str);
    }
    ///
    tstring &getString() {
        return *this;
    }
    ///
    const tstring &getString() const {
        return *this;
    }
};

/// case insensitive string comparison
inline bool operator<(const tstringi &i_str1, const _TCHAR *i_str2)
{
    return i_str1.compare(i_str2) < 0;
}
/// case insensitive string comparison
inline bool operator<(const _TCHAR *i_str1, const tstringi &i_str2)
{
    return 0 < i_str2.compare(i_str1);
}
/// case insensitive string comparison
inline bool operator<(const tstringi &i_str1, const tstring &i_str2)
{
    return i_str1.compare(i_str2) < 0;
}
/// case insensitive string comparison
inline bool operator<(const tstring &i_str1, const tstringi &i_str2)
{
    return 0 < i_str2.compare(i_str1);
}
/// case insensitive string comparison
inline bool operator<(const tstringi &i_str1, const tstringi &i_str2)
{
    return i_str1.compare(i_str2) < 0;
}

/// case insensitive string comparison
inline bool operator==(const _TCHAR *i_str1, const tstringi &i_str2)
{
    return i_str2.compare(i_str1) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi &i_str1, const _TCHAR *i_str2)
{
    return i_str1.compare(i_str2) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstring &i_str1, const tstringi &i_str2)
{
    return i_str2.compare(i_str1) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi &i_str1, const tstring &i_str2)
{
    return i_str1.compare(i_str2) == 0;
}
/// case insensitive string comparison
inline bool operator==(const tstringi &i_str1, const tstringi &i_str2)
{
    return i_str1.compare(i_str2) == 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// workarounds for Borland C++


/// case insensitive string comparison
inline bool operator!=(const _TCHAR *i_str1, const tstringi &i_str2)
{
    return i_str2.compare(i_str1) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi &i_str1, const _TCHAR *i_str2)
{
    return i_str1.compare(i_str2) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstring &i_str1, const tstringi &i_str2)
{
    return i_str2.compare(i_str1) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi &i_str1, const tstring &i_str2)
{
    return i_str1.compare(i_str2) != 0;
}
/// case insensitive string comparison
inline bool operator!=(const tstringi &i_str1, const tstringi &i_str2)
{
    return i_str1.compare(i_str2) != 0;
}


/// stream output
extern tostream &operator<<(tostream &i_ost, const tregex &i_data);

/// get lower string
extern tstring toLower(const tstring &i_str);


#endif // !_STRINGTOOL_H
