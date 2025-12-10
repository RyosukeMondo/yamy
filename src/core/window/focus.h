#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// focus.h


#ifndef _FOCUS_H
#  define _FOCUS_H

#  include <cstdint>
#  include "../platform/message_constants.h"


///
extern uint16_t Register_focus();

enum {
    WM_APP_notifyFocus = yamy::platform::MSG_APP_NOTIFY_FOCUS,
    WM_APP_notifyVKey  = yamy::platform::MSG_APP_NOTIFY_VKEY,
};


#endif // !_FOCUS_H
