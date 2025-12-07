#pragma once
#ifndef _INPUT_EVENT_H
#define _INPUT_EVENT_H

#include "../utils/misc.h" // For basic types if needed, or standard types

/// derived from w2kddk/inc/ntddkbd.h
/// This serves as the platform-neutral input event structure for Yamy engine
class KEYBOARD_INPUT_DATA
{
public:
    ///
    enum {
        /// key release flag
        BREAK = 1,
        /// extended key flag
        E0 = 2,
        /// extended key flag
        E1 = 4,
        /// extended key flag (E0 | E1)
        E0E1 = 6,
        ///
        TERMSRV_SET_LED = 8,
        /// Define the keyboard overrun MakeCode.
        KEYBOARD_OVERRUN_MAKE_CODE_ = 0xFF,
    };

public:
    /** Unit number.  E.g., for \Device\KeyboardPort0 the unit is '0', for
        \Device\KeyboardPort1 the unit is '1', and so on. */
    unsigned short UnitId; // USHORT

    /** The "make" scan code (key depression). */
    unsigned short MakeCode; // USHORT

    /** The flags field indicates a "break" (key release) and other miscellaneous
        scan code information defined above. */
    unsigned short Flags; // USHORT

    ///
    unsigned short Reserved; // USHORT

    /** Device-specific additional information for the event. */
    unsigned long ExtraInformation; // ULONG
};

#endif // !_INPUT_EVENT_H
