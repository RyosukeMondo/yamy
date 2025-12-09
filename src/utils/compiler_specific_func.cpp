//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// compiler_specific_func.cpp


#include "compiler_specific_func.h"
#include <cstdio>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Microsoft Visual C++ 6.0

#if defined(_MSC_VER)

// get compiler version string
tstring getCompilerVersionString()
{
    TCHAR buf[200];
    _sntprintf(buf, NUMBER_OF(buf),
               _T("Microsoft (R) 32-bit C/C++ Optimizing Compiler Version %d.%02d"),
               _MSC_VER / 100,
               _MSC_VER % 100);
    return tstring(buf);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Borland C++ 5.5.1

#elif defined(__BORLANDC__)

// get compiler version string
tstring getCompilerVersionString()
{
    TCHAR buf[100];
    _sntprintf(buf, NUMBER_OF(buf), _T("Borland C++ %d.%d.%d"),
               __BORLANDC__ / 0x100,
               __BORLANDC__ / 0x10 % 0x10,
               __BORLANDC__ % 0x10);
    return tstring(buf);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MinGW / GCC

#elif defined(__MINGW32__) || defined(__GNUC__)

// get compiler version string
tstring getCompilerVersionString()
{
    TCHAR buf[256];
#ifdef UNICODE
    #ifdef _WIN32
        _sntprintf(buf, NUMBER_OF(buf), _T("GCC %hs"), __VERSION__);
    #else
        swprintf(buf, NUMBER_OF(buf), _T("GCC %ls"), _T(__VERSION__));
    #endif
#else
    #ifdef _WIN32
        _sntprintf(buf, NUMBER_OF(buf), _T("GCC %s"), __VERSION__);
    #else
        snprintf(buf, NUMBER_OF(buf), _T("GCC %s"), __VERSION__);
    #endif
#endif
    return tstring(buf);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// unknown

#else
#  error "I don't know the details of this compiler... Plz hack."

#endif
