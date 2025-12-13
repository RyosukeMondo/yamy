#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc.h


#ifndef _MISC_H
#  define _MISC_H

#  include "compiler_specific.h"
#  ifdef _WIN32
#    include <windows.h>
#  else
#    include <cstddef>
#    include <cstring>
#    include <cstdint>
     // Shim for Win32 types on Linux/non-Windows
     typedef uint32_t DWORD;
     typedef uint16_t WORD;
     typedef uint16_t USHORT;
     typedef uint8_t BYTE;
     typedef int32_t LONG;
     typedef int32_t BOOL;
     typedef void* HWND;
     typedef void* HGLOBAL;
     typedef void* HANDLE;
     typedef void* HMODULE;
     typedef void* HINSTANCE;
     typedef int64_t __int64;
     typedef uint64_t __uint64;
     typedef uint32_t UINT;
     typedef intptr_t LPARAM;
     typedef uintptr_t WPARAM;
     typedef long LRESULT;
     typedef TCHAR _TCHAR;
     typedef struct tagPOINT {
         LONG x;
         LONG y;
     } POINT;
     typedef struct tagRECT {
         LONG left;
         LONG top;
         LONG right;
         LONG bottom;
     } RECT;
     typedef struct _OVERLAPPED {
         uintptr_t Internal;
         uintptr_t InternalHigh;
         union {
             struct {
                 DWORD Offset;
                 DWORD OffsetHigh;
             } DUMMYSTRUCTNAME;
             void* Pointer;
         } DUMMYUNIONNAME;
         HANDLE hEvent;
     } OVERLAPPED;
     #define MAX_PATH 260
     #define WINAPI
     #define CALLBACK
     #define INFINITE 0xFFFFFFFF

     // ShowWindow constants for cross-platform compatibility
     #define SW_HIDE             0
     #define SW_SHOWNORMAL       1
     #define SW_NORMAL           1
     #define SW_SHOWMINIMIZED    2
     #define SW_SHOWMAXIMIZED    3
     #define SW_MAXIMIZE         3
     #define SW_SHOWNOACTIVATE   4
     #define SW_SHOW             5
     #define SW_MINIMIZE         6
     #define SW_SHOWMINNOACTIVE  7
     #define SW_SHOWNA           8
     #define SW_RESTORE          9
     #define SW_SHOWDEFAULT      10
     #define SW_FORCEMINIMIZE    11

     // Win32 API stubs for Linux - these are no-ops
     inline BOOL PostMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
     inline BOOL SendMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
     inline BOOL PostThreadMessage(DWORD, UINT, WPARAM, LPARAM) { return 0; }
     inline BOOL MessageBeep(UINT) { return 0; }

     // Windows Message constants
     #define WM_NULL         0x0000
     #define WM_USER         0x0400
     #define WM_APP          0x8000
     #define WM_QUIT         0x0012
     #define WM_KEYDOWN      0x0100
     #define WM_KEYUP        0x0101
     #define WM_CHAR         0x0102
     #define WM_SYSCOMMAND   0x0112
     #define WM_CLOSE        0x0010

     // System Command constants
     #define SC_MINIMIZE     0xF020
     #define SC_MAXIMIZE     0xF030
     #define SC_RESTORE      0xF120
     #define SC_CLOSE        0xF060

     // MessageBox constants
     #define MB_OK               0x00000000L
     #define MB_OKCANCEL         0x00000001L
     #define MB_ABORTRETRYIGNORE 0x00000002L
     #define MB_YESNOCANCEL      0x00000003L
     #define MB_YESNO            0x00000004L
     #define MB_RETRYCANCEL      0x00000005L
     #define MB_ICONERROR        0x00000010L
     #define MB_ICONWARNING      0x00000030L
     #define MB_ICONINFORMATION  0x00000040L

     // Virtual Key constants (subset needed by Core module)
     #define VK_LBUTTON    0x01
     #define VK_RBUTTON    0x02
     #define VK_MBUTTON    0x04
     #define VK_XBUTTON1   0x05
     #define VK_XBUTTON2   0x06
     #define VK_SHIFT      0x10
     #define VK_CONTROL    0x11
     #define VK_MENU       0x12
     #define VK_LSHIFT     0xA0
     #define VK_RSHIFT     0xA1
     #define VK_LCONTROL   0xA2
     #define VK_RCONTROL   0xA3
     #define VK_LMENU      0xA4
     #define VK_RMENU      0xA5

     // Layered window attributes
     #define LWA_COLORKEY  0x00000001
     #define LWA_ALPHA     0x00000002

     // Pointer types
     typedef uintptr_t ULONG_PTR;
     typedef intptr_t LONG_PTR;
#  endif
#  include <cassert>

#define YAMY_SUCCESS                        0
#define YAMY_ERROR_ON_GET_USERNAME            1001
#define YAMY_ERROR_INSUFFICIENT_BUFFER        1002
#define YAMY_ERROR_NO_MEMORY                1003
#define YAMY_ERROR_ON_GET_LOGONUSERNAME        1004
#define YAMY_ERROR_ON_GET_SECURITYINFO        1005
#define YAMY_ERROR_ON_GET_DACL                1006
#define YAMY_ERROR_ON_INITIALIZE_ACL        1007
#define YAMY_ERROR_ON_GET_ACE                1008
#define YAMY_ERROR_ON_ADD_ACE                1009
#define YAMY_ERROR_ON_ADD_ALLOWED_ACE        1010
#define YAMY_ERROR_ON_SET_SECURITYINFO        1011
#define YAMY_ERROR_ON_OPEN_YAMY_PROCESS        1012
#define YAMY_ERROR_ON_OPEN_YAMY_TOKEN        1013
#define YAMY_ERROR_ON_IMPERSONATE            1014
#define YAMY_ERROR_ON_REVERT_TO_SELF        1015
#define YAMY_ERROR_ON_OPEN_CURRENT_PROCESS    1016
#define YAMY_ERROR_ON_LOOKUP_PRIVILEGE        1017
#define YAMY_ERROR_ON_ADJUST_PRIVILEGE        1018
#define YAMY_ERROR_ON_OPEN_WINLOGON_PROCESS    1019
#define YAMY_ERROR_ON_VIRTUALALLOCEX        1020
#define YAMY_ERROR_ON_WRITEPROCESSMEMORY    1021
#define YAMY_ERROR_ON_CREATEREMOTETHREAD    1022
#define YAMY_ERROR_TIMEOUT_INJECTION        1023
#define YAMY_ERROR_RETRY_INJECTION_SUCCESS    1024
#define YAMY_ERROR_ON_READ_SCANCODE_MAP        1025
#define YAMY_ERROR_ON_WRITE_SCANCODE_MAP    1026
#define YAMY_ERROR_ON_GET_WINLOGON_PID        1027

typedef unsigned char u_char;            /// unsigned char
typedef unsigned short u_short;            /// unsigned short
typedef unsigned long u_long;            /// unsigned long

typedef char int8;                /// signed 8bit
typedef short int16;                /// signed 16bit
typedef long int32;                /// signed 32bit
typedef unsigned char u_int8;            /// unsigned 8bit
typedef unsigned short u_int16;            /// unsigned 16bit
typedef unsigned long u_int32;            /// unsigned 32bit
#if defined(__BORLANDC__)
typedef unsigned __int64 u_int64;            /// unsigned 64bit
#elif defined(_MSC_VER) && (_MSC_VER <= 1300)
typedef unsigned _int64 u_int64;            /// unsigned 64bit
#else
typedef unsigned long long u_int64;            /// unsigned 64bit
#endif


#  ifdef NDEBUG
#    define ASSERT(i_exp)
#    define CHECK(i_cond, i_exp)    ((void)(i_exp))
#    define CHECK_TRUE(i_exp)        ((void)(i_exp))
#    define CHECK_FALSE(i_exp)        ((void)(i_exp))
#  else // NDEBUG
/// assertion. i_exp is evaluated only in debug build
#    define ASSERT(i_exp)        assert(i_exp)
/// assertion, but i_exp is always evaluated
#    define CHECK(i_cond, i_exp)    assert(i_cond (i_exp))
/// identical to CHECK(!!, i_exp)
#    define CHECK_TRUE(i_exp)        assert(!!(i_exp))
/// identical to CHECK(!, i_exp)
#    define CHECK_FALSE(i_exp)        assert(!(i_exp))
#  endif // NDEBUG


/// get number of array elements
#  define NUMBER_OF(i_array) (sizeof(i_array) / sizeof((i_array)[0]))

/// max path length
#  define GANA_MAX_PATH        (MAX_PATH * 4)

/// max length of global atom
#  define GANA_MAX_ATOM_LENGTH    256

#  undef MAX
/// redefine MAX macro
#  define MAX(a, b)    (((b) < (a)) ? (a) : (b))

#  undef MIN
/// redefine MIN macro
#  define MIN(a, b)    (((a) < (b)) ? (a) : (b))


#endif // !_MISC_H
