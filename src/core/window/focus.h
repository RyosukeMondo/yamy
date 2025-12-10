#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// focus.h


#ifndef _FOCUS_H
#  define _FOCUS_H

#  include <windows.h>
#  include <cstdint>


///
extern uint16_t Register_focus();

enum {
    WM_APP_notifyFocus = WM_APP + 103,
    WM_APP_notifyVKey  = WM_APP + 104,
};


#endif // !_FOCUS_H
