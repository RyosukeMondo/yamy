#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// target.h


#ifndef _TARGET_H
#  define _TARGET_H

#  include <cstdint>
#  include "../platform/message_constants.h"

///
extern uint16_t Register_target();

///
enum {
    ///
    WM_APP_targetNotify = yamy::platform::MSG_APP_TARGET_NOTIFY,
};


#endif // !_TARGET_H
