#pragma once
#ifndef _INPUT_INJECTOR_H
#define _INPUT_INJECTOR_H

#include "input_event.h"
#include "../window/window_system.h"

struct InjectionContext {
    bool isDragging;
    WindowPoint dragStartPos;
};

class InputInjector {
public:
    virtual ~InputInjector() = default;

    /**
     * Inject an event.
     * @param data The platform-neutral input data.
     * @param ctx Context for injection (dragging state, etc.)
     * @param rawData Optional platform-specific data (e.g. KBDLLHOOKSTRUCT* on Windows)
     */
    virtual void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) = 0;
};

#endif // !_INPUT_INJECTOR_H
