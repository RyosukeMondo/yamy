//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// stringtool.cpp
// Cross-platform string utilities using UTF-8 std::string


#include "stringtool.h"
#include <vector>
#include <locale>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <mbstring.h>
#else
#include <unistd.h>
#endif


/* ************************************************************************** *

STRLCPY(3)                OpenBSD Programmer's Manual               STRLCPY(3)

NAME
     strlcpy, strlcat - size-bounded string copying and concatenation



SYNOPSIS
     #include <string.h>

     size_t
     strlcpy(char *dst, const char *src, size_t size);

     size_t
     strlcat(char *dst, const char *src, size_t size);

DESCRIPTION
     The strlcpy() and strlcat() functions copy and concatenate strings re-
     spectively.  They are designed to be safer, more consistent, and less er-
     ror prone replacements for strncpy(3) and strncat(3).  Unlike those func-
     tions, strlcpy() and strlcat() take the full size of the buffer (not just
     the length) and guarantee to NUL-terminate the result (as long as size is
     larger than 0).  Note that you should include a byte for the NUL in size.

     The strlcpy() function copies up to size - 1 characters from the NUL-ter-
     minated string src to dst, NUL-terminating the result.

     The strlcat() function appends the NUL-terminated string src to the end
     of dst. It will append at most size - strlen(dst) - 1 bytes, NUL-termi-
     nating the result.

RETURN VALUES
     The strlcpy() and strlcat() functions return the total length of the
     string they tried to create.  For strlcpy() that means the length of src.
     For strlcat() that means the initial length of dst plus the length of
     src. While this may seem somewhat confusing it was done to make trunca-
     tion detection simple.

EXAMPLES
     The following code fragment illustrates the simple case:

           char *s, *p, buf[BUFSIZ];

           ...

           (void)strlcpy(buf, s, sizeof(buf));
           (void)strlcat(buf, p, sizeof(buf));

     To detect truncation, perhaps while building a pathname, something like
     the following might be used:

           char *dir, *file, pname[MAXPATHNAMELEN];

           ...

           if (strlcpy(pname, dir, sizeof(pname)) >= sizeof(pname))
                   goto toolong;
           if (strlcat(pname, file, sizeof(pname)) >= sizeof(pname))
                   goto toolong;

     Since we know how many characters we copied the first time, we can speed
     things up a bit by using a copy instead on an append:

           char *dir, *file, pname[MAXPATHNAMELEN];
           size_t n;

           ...

           n = strlcpy(pname, dir, sizeof(pname));
           if (n >= sizeof(pname))
                   goto toolong;
           if (strlcpy(pname + n, file, sizeof(pname) - n) >= sizeof(pname)-n)
                   goto toolong;

     However, one may question the validity of such optimizations, as they de-
     feat the whole purpose of strlcpy() and strlcat().  As a matter of fact,
     the first version of this manual page got it wrong.

SEE ALSO
     snprintf(3),  strncat(3),  strncpy(3)

OpenBSD 2.6                      June 22, 1998                               2


-------------------------------------------------------------------------------

Source: OpenBSD 2.6 man pages. Copyright: Portions are copyrighted by BERKELEY
SOFTWARE DESIGN, INC., The Regents of the University of California,
Massachusetts Institute of Technology, Free Software Foundation, FreeBSD Inc.,
and others.

* ************************************************************************** */


// copy
template <class T>
static inline size_t xstrlcpy(T *o_dest, const T *i_src, size_t i_destSize)
{
    T *d = o_dest;
    const T *s = i_src;
    size_t n = i_destSize;

    ASSERT( o_dest != nullptr );
    ASSERT( i_src != nullptr );

    // Copy as many bytes as will fit
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    // Not enough room in o_dest, add NUL and traverse rest of i_src
    if (n == 0) {
        if (i_destSize != 0)
            *d = T();                    // NUL-terminate o_dest
        while (*s++)
            ;
    }

    return (s - i_src - 1);            // count does not include NUL
}


// copy
size_t strlcpy(char *o_dest, const char *i_src, size_t i_destSize)
{
    return xstrlcpy(o_dest, i_src, i_destSize);
}


// copy
size_t wcslcpy(wchar_t *o_dest, const wchar_t *i_src, size_t i_destSize)
{
    return xstrlcpy(o_dest, i_src, i_destSize);
}


// copy (UTF-8 safe - handles multi-byte sequences)
size_t mbslcpy(unsigned char *o_dest, const unsigned char *i_src,
               size_t i_destSize)
{
    unsigned char *d = o_dest;
    const unsigned char *s = i_src;
    size_t n = i_destSize;

    ASSERT( o_dest != nullptr );
    ASSERT( i_src != nullptr );

    if (n == 0)
        return strlen(reinterpret_cast<const char *>(i_src));

    // Copy as many bytes as will fit, respecting UTF-8 multi-byte sequences
    for (-- n; *s && 0 < n; -- n) {
        // Check for UTF-8 multi-byte sequence lead byte
        if ((*s & 0xC0) == 0xC0) {
            // Count bytes needed for this UTF-8 character
            int bytes_needed = 1;
            if ((*s & 0xE0) == 0xC0) bytes_needed = 2;      // 110xxxxx
            else if ((*s & 0xF0) == 0xE0) bytes_needed = 3; // 1110xxxx
            else if ((*s & 0xF8) == 0xF0) bytes_needed = 4; // 11110xxx

            // Check if we have room for the complete sequence
            if (bytes_needed > (int)(n + 1))
                break;

            // Copy the multi-byte sequence
            for (int i = 0; i < bytes_needed && *s; ++i) {
                *d++ = *s++;
                if (i > 0) --n;
            }
            continue;
        }
        *d++ = *s++;
    }
    *d = '\0';

    for (; *s; ++ s)
        ;

    return s - i_src;
}


/// stream output (escaped/quoted string output)
std::ostream &operator<<(std::ostream &i_ost, const stringq &i_data)
{
    i_ost << "\"";
    for (const char *s = i_data.c_str(); *s; ++ s) {
        // Handle UTF-8 multi-byte sequences
        unsigned char c = static_cast<unsigned char>(*s);
        if ((c & 0x80) != 0) {
            // UTF-8 multi-byte character - output as-is
            int bytes = 1;
            if ((c & 0xE0) == 0xC0) bytes = 2;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xF8) == 0xF0) bytes = 4;

            for (int i = 0; i < bytes && s[i]; ++i) {
                i_ost << s[i];
            }
            s += bytes - 1;  // -1 because loop will increment
            continue;
        }

        switch (*s) {
        case '\a':
            i_ost << "\\a";
            break;
        case '\f':
            i_ost << "\\f";
            break;
        case '\n':
            i_ost << "\\n";
            break;
        case '\r':
            i_ost << "\\r";
            break;
        case '\t':
            i_ost << "\\t";
            break;
        case '\v':
            i_ost << "\\v";
            break;
        case '"':
            i_ost << "\\\"";
            break;
        default:
            if (std::isprint(c)) {
                i_ost << *s;
            } else {
                // Output as hex escape
                char buf[5];
                snprintf(buf, sizeof(buf), "\\x%02x", c);
                i_ost << buf;
            }
            break;
        }
    }
    i_ost << "\"";
    return i_ost;
}


namespace {

// Helper to parse a hexadecimal escape sequence like \x{...} or \x..
// Advances the source pointer and writes the character to the destination.
static void interpretHexEscape(const char*& i_str, const char* end, char*& d)
{
    i_str++; // consume 'x' or 'X'
    static const char* hexchar = "0123456789ABCDEFabcdef";
    static int hexvalue[] = { 0, 1, 2, 3, 4, 5 ,6, 7, 8, 9,
                              10, 11, 12, 13, 14, 15,
                              10, 11, 12, 13, 14, 15,
                            };
    bool brace = false;
    if (i_str < end && *i_str == '{') {
        i_str++;
        brace = true;
    }
    int n = 0;
    for (; i_str < end && *i_str; i_str++) {
        if (const char* cc = strchr(hexchar, *i_str)) {
            n = n * 16 + hexvalue[cc - hexchar];
        } else {
            break;
        }
    }
    if (i_str < end && *i_str == '}' && brace) {
        i_str++;
    }
    if (0 < n) {
        *d++ = static_cast<char>(n);
    }
}

// Helper to parse an octal escape sequence.
// Advances the source pointer and writes the character to the destination.
static void interpretOctalEscape(const char*& i_str, const char* end, char*& d)
{
    static const char* octalchar = "01234567";
    static int octalvalue[] = { 0, 1, 2, 3, 4, 5 ,6, 7, };
    int n = 0;
    for (; i_str < end && *i_str; i_str++) {
        if (const char* cc = strchr(octalchar, *i_str)) {
            n = n * 8 + octalvalue[cc - octalchar];
        } else {
            break;
        }
    }
    if (0 < n) {
        *d++ = static_cast<char>(n);
    }
}

// Helper to parse a control character escape sequence like \c[.
// Advances the source pointer and writes the character to the destination.
static void interpretControlCode(const char*& i_str, const char* end, char*& d)
{
    i_str++; // consume 'c'
    if (i_str < end && *i_str) {
        static const char* ctrlchar =
            "@ABCDEFGHIJKLMNO"
            "PQRSTUVWXYZ[\\]^_"
            "@abcdefghijklmno"
            "pqrstuvwxyz@@@@?";
        static const char* ctrlcode =
            "\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17"
            "\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37"
            "\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17"
            "\20\21\22\23\24\25\26\27\30\31\32\00\00\00\00\177";
        if (const char* cc = strchr(ctrlchar, *i_str)) {
            *d++ = ctrlcode[cc - ctrlchar];
            i_str++;
        }
    }
}

// Helper to interpret a single escape sequence starting after the backslash.
// Advances the source pointer and writes the character(s) to the destination.
static void interpretEscapeSequence(const char*& i_str, const char* end, char*& d,
                                    const char* i_quote, bool i_doesUseRegexpBackReference)
{
    if (i_quote && strchr(i_quote, *i_str)) {
        *d++ = *i_str++;
        return;
    }

    switch (*i_str) {
    case 'a': *d++ = '\x07'; i_str++; break;
    case 'e': *d++ = '\x1b'; i_str++; break;
    case 'f': *d++ = '\f'; i_str++; break;
    case 'n': *d++ = '\n'; i_str++; break;
    case 'r': *d++ = '\r'; i_str++; break;
    case 't': *d++ = '\t'; i_str++; break;
    case 'v': *d++ = '\v'; i_str++; break;
    case '\'': *d++ = '\''; i_str++; break;
    case '"': *d++ = '"'; i_str++; break;
    case '\\': *d++ = '\\'; i_str++; break;
    case 'c':
        interpretControlCode(i_str, end, d);
        break;
    case 'x':
    case 'X':
        interpretHexEscape(i_str, end, d);
        break;
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7':
        if (i_doesUseRegexpBackReference) {
            goto handle_default;
        }
        // fall through
    case '0':
        interpretOctalEscape(i_str, end, d);
        break;
    default:
    handle_default:
        *d++ = '\\';
        *d++ = *i_str++;
        break;
    }
}

} // namespace


// interpret meta characters such as \n (UTF-8 version)
std::string interpretMetaCharacters(const char *i_str, size_t i_len,
                                    const char *i_quote,
                                    bool i_doesUseRegexpBackReference)
{
    // interpreted string is always less than or equal to i_len
    std::vector<char> result(i_len + 1);
    char *d = result.data();
    const char *end = i_str + i_len;

    while (i_str < end && *i_str) {
        if (*i_str == '\\') {
            i_str++; // Consume backslash
            if (i_str < end && *i_str) {
                // We have a character after the backslash, so interpret it
                interpretEscapeSequence(i_str, end, d, i_quote, i_doesUseRegexpBackReference);
            } else {
                // Dangling backslash at the end of the string, treat as literal
                *d++ = '\\';
            }
        } else {
             // Handle UTF-8 multi-byte sequences by copying them completely
            unsigned char c = static_cast<unsigned char>(*i_str);
            if ((c & 0x80) != 0) {
                int bytes = 1;
                if ((c & 0xE0) == 0xC0) bytes = 2;
                else if ((c & 0xF0) == 0xE0) bytes = 3;
                else if ((c & 0xF8) == 0xF0) bytes = 4;

                for (int i = 0; i < bytes && i_str < end && *i_str; ++i) {
                    *d++ = *i_str++;
                }
            } else {
                // Plain ASCII character
                *d++ = *i_str++;
            }
        }
    }
    *d = '\0';
    return std::string(result.data());
}


// add session id to i_str (cross-platform)
std::string addSessionId(const char *i_str)
{
    std::string s(i_str);
#ifdef _WIN32
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%u", (unsigned int)sessionId);
        s += buf;
    }
#else
    // On Linux, use the session ID from getsid() or just the process ID
    // For uniqueness in IPC naming, the PID is sufficient
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", (int)getpid());
    s += buf;
#endif
    return s;
}


// add session id to i_str (wide char version)
std::wstring addSessionId(const wchar_t *i_str)
{
#ifdef _WIN32
    std::wstring s(i_str);
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
        wchar_t buf[20];
        swprintf_s(buf, 20, L"%u", (unsigned int)sessionId);
        s += buf;
    }
    return s;
#else
    return std::wstring(i_str);
#endif
}


#ifdef _WIN32
// escape regexp special characters in MBCS trail bytes (Windows only)
std::string guardRegexpFromMbcs(const char *i_str)
{
#ifdef _MBCS
    size_t len = strlen(i_str);
    std::vector<char> buf(len * 2 + 1);
    char *p = buf.data();
    while (*i_str) {
        if (_ismbblead(static_cast<unsigned char>(*i_str)) && i_str[1]) {
            *p ++ = *i_str ++;
            if (strchr(".*?+(){}[]^$", *i_str))
                *p ++ = '\\';
        }
        *p ++ = *i_str ++;
    }
    return std::string(buf.data(), p);
#else
    // For non-MBCS builds, just return as-is
    return std::string(i_str);
#endif
}
#else
// On Linux with UTF-8, no MBCS escaping needed
std::string guardRegexpFromMbcs(const char *i_str)
{
    return std::string(i_str);
}
#endif


// converter
std::wstring to_wstring(const std::string &i_str)
{
    size_t size = mbstowcs(nullptr, i_str.c_str(), i_str.size() + 1);
    if (size == (size_t)-1)
        return std::wstring();
    std::vector<wchar_t> result(size + 1);
    mbstowcs(result.data(), i_str.c_str(), i_str.size() + 1);
    return std::wstring(result.data());
}


// converter
std::string to_string(const std::wstring &i_str)
{
    size_t size = wcstombs(nullptr, i_str.c_str(), i_str.size() + 1);
    if (size == (size_t)-1)
        return std::string();
    std::vector<char> result(size + 1);
    wcstombs(result.data(), i_str.c_str(), i_str.size() + 1);
    return std::string(result.data());
}


/// stream output
std::ostream &operator<<(std::ostream &i_ost, const Regex &i_data)
{
    return i_ost << i_data.str();
}


/// get lower string (UTF-8 safe for ASCII characters)
std::string toLower(const std::string &i_str)
{
    std::string str(i_str);
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        // Skip UTF-8 multi-byte sequences (only lowercase ASCII)
        if ((c & 0x80) != 0) {
            // UTF-8 continuation or lead byte - skip
            continue;
        }
        str[i] = static_cast<char>(std::tolower(c));
    }
    return str;
}


// Case-insensitive string comparison for std::string (UTF-8)
int strcasecmp_utf8(const char* s1, const char* s2) {
#ifdef _WIN32
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}


// convert wstring to UTF-8
std::string to_UTF_8(const std::wstring &i_str)
{
    // 0xxxxxxx: 00-7F
    // 110xxxxx 10xxxxxx: 0080-07FF
    // 1110xxxx 10xxxxxx 10xxxxxx: 0800 - FFFF

    int size = 0;

    // count needed buffer size
    for (std::wstring::const_iterator i = i_str.begin(); i != i_str.end(); ++ i) {
        if (0x0000 <= *i && *i <= 0x007f)
            size += 1;
        else if (0x0080 <= *i && *i <= 0x07ff)
            size += 2;
        else if (0x0800 <= *i && *i <= 0xffff)
            size += 3;
    }

    std::vector<char> result(size);
    int ri = 0;

    // make UTF-8
    for (std::wstring::const_iterator i = i_str.begin(); i != i_str.end(); ++ i) {
        if (0x0000 <= *i && *i <= 0x007f)
            result[ri ++] = static_cast<char>(*i);
        else if (0x0080 <= *i && *i <= 0x07ff) {
            result[ri ++] = static_cast<char>(((*i & 0x0fc0) >>  6) | 0xc0);
            result[ri ++] = static_cast<char>(( *i & 0x003f       ) | 0x80);
        } else if (0x0800 <= *i && *i <= 0xffff) {
            result[ri ++] = static_cast<char>(((*i & 0xf000) >> 12) | 0xe0);
            result[ri ++] = static_cast<char>(((*i & 0x0fc0) >>  6) | 0x80);
            result[ri ++] = static_cast<char>(( *i & 0x003f       ) | 0x80);
        }
    }

    return std::string(result.begin(), result.end());
}
