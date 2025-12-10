#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// target.h


#ifndef _TARGET_H
#  define _TARGET_H

#  include <cstdint>

///
extern uint16_t Register_target();

///
enum {
    ///
    WM_APP_targetNotify = 0x8000 + 102,
};


#endif // !_TARGET_H
