//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// compiler_specific_func.cpp


#include "compiler_specific_func.h"
#include <cstdio>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Microsoft Visual C++ 6.0

#if defined(_MSC_VER)

// get compiler version string
std::string getCompilerVersionString()
{
    std::stringstream ss;
    ss << "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version "
       << _MSC_VER / 100 << "." << std::setfill('0') << std::setw(2) << _MSC_VER % 100;
    return ss.str();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Borland C++ 5.5.1

#elif defined(__BORLANDC__)

// get compiler version string
std::string getCompilerVersionString()
{
    std::stringstream ss;
    ss << "Borland C++ " << (__BORLANDC__ / 0x100) << "."
       << (__BORLANDC__ / 0x10 % 0x10) << "." << (__BORLANDC__ % 0x10);
    return ss.str();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MinGW / GCC

#elif defined(__MINGW32__) || defined(__GNUC__)

// get compiler version string
std::string getCompilerVersionString()
{
    std::stringstream ss;
    ss << "GCC " << __VERSION__;
    return ss.str();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// unknown

#else
#  error "I don't know the details of this compiler... Plz hack."

#endif
