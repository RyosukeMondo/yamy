#pragma once
#ifndef _INPUT_EVENT_H
#define _INPUT_EVENT_H

/**
 * @file input_event.h
 * @brief Platform-neutral keyboard input event structure.
 *
 * Defines the KEYBOARD_INPUT_DATA structure derived from Windows DDK
 * (w2kddk/inc/ntddkbd.h) but adapted for cross-platform use in YAMY.
 */

#include "../utils/misc.h" // For basic types if needed, or standard types

/**
 * @brief Platform-neutral keyboard input event structure.
 *
 * This structure represents raw keyboard input events captured by the input driver.
 * Derived from the Windows DDK KEYBOARD_INPUT_DATA structure but adapted for
 * platform-agnostic use.
 *
 * @note This is a low-level structure used by the input driver and hook subsystems.
 *       Higher-level code should use the Engine's input processing APIs instead.
 *
 * @code
 * KEYBOARD_INPUT_DATA event;
 * event.MakeCode = 0x1E; // 'A' key
 * event.Flags = 0;        // Key press (not BREAK)
 * event.UnitId = 0;       // Primary keyboard
 * @endcode
 */
class KEYBOARD_INPUT_DATA
{
public:
    /**
     * @brief Flags for keyboard event interpretation.
     */
    enum {
        BREAK = 1,                          ///< Key release flag (vs key press)
        E0 = 2,                             ///< Extended key flag (E0 prefix)
        E1 = 4,                             ///< Extended key flag (E1 prefix)
        E0E1 = 6,                           ///< Extended key flag (E0 | E1)
        TERMSRV_SET_LED = 8,                ///< Terminal services LED control
        KEYBOARD_OVERRUN_MAKE_CODE_ = 0xFF, ///< Keyboard buffer overrun indicator
    };

public:
    /**
     * @brief Unit number identifying the keyboard device.
     *
     * For \Device\KeyboardPort0 the unit is '0', for \Device\KeyboardPort1
     * the unit is '1', and so on.
     */
    unsigned short UnitId; // USHORT

    /**
     * @brief The "make" scan code (key depression).
     *
     * This is the hardware scan code generated when a key is pressed.
     * For key releases, the BREAK flag in Flags is set.
     */
    unsigned short MakeCode; // USHORT

    /**
     * @brief Event flags indicating key release and extended key information.
     *
     * Combination of BREAK, E0, E1, TERMSRV_SET_LED flags.
     * @see KEYBOARD_INPUT_DATA::BREAK
     * @see KEYBOARD_INPUT_DATA::E0
     */
    unsigned short Flags; // USHORT

    /**
     * @brief Reserved field for alignment.
     */
    unsigned short Reserved; // USHORT

    /**
     * @brief Device-specific additional information for the event.
     *
     * This field can contain application-defined data injected with the event.
     * YAMY uses this for identifying internally-generated events.
     */
    unsigned long ExtraInformation; // ULONG
};

#endif // !_INPUT_EVENT_H
