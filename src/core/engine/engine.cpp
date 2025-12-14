//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp - Main engine implementation (core logic split into separate files)

#include "misc.h"
#include "engine.h"

// Implementation split across the following files:
// - engine_lifecycle.cpp: Constructor, destructor, start, stop
// - engine_keyboard_handler.cpp: Keyboard event handling thread
// - engine_ipc_handler.cpp: IPC message handling
// - engine_event_processor.cpp: Event processing logic
// - engine_generator.cpp: Key event generation
// - engine_modifier.cpp: Modifier key handling
// - engine_focus.cpp: Window focus management
// - engine_window.cpp: Window management
// - engine_setting.cpp: Settings management
// - engine_input.cpp: Input injection
// - engine_log.cpp: Logging
