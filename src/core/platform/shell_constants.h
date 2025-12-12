// shell_constants.h - Cross-platform shell error constants

#ifndef _SHELL_CONSTANTS_H
#define _SHELL_CONSTANTS_H

#ifdef _WIN32
#include <shellapi.h> // Provides all the constants on Windows
#else
// Shell error constants for Linux (values match Windows for compatibility)
#define ERROR_FILE_NOT_FOUND    2
#define ERROR_PATH_NOT_FOUND    3
#define ERROR_BAD_FORMAT        11
#define SE_ERR_ACCESSDENIED     5
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDEBUSY          30
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DLLNOTFOUND      32
#define SE_ERR_FNF              2
#define SE_ERR_NOASSOC          31
#define SE_ERR_OOM              8
#define SE_ERR_PNF              3
#define SE_ERR_SHARE            26
#endif

#endif // _SHELL_CONSTANTS_H
