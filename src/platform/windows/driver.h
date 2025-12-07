#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// driver.h


#ifndef _DRIVER_H
#  define _DRIVER_H

#  include <winioctl.h>
#  include "../../core/input/input_event.h"


/// mayu device file name
#    define MAYU_DEVICE_FILE_NAME _T("\\\\.\\MayuDetour1")
///
#    define MAYU_DRIVER_NAME _T("mayud")

/// Ioctl value
#include "../driver/ioctl.h"


#endif // !_DRIVER_H
