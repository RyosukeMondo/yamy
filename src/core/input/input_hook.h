#pragma once
#ifndef _INPUT_HOOK_H
#define _INPUT_HOOK_H

class Engine;

/// Platform-neutral interface for installing/uninstalling input hooks.
class InputHook
{
public:
    virtual ~InputHook() { }

    /**
     * Start the hook.
     * @param i_engine Pointer to the engine instance (used for callbacks).
     * @return true if successful.
     */
    virtual bool start(Engine *i_engine) = 0;

    /**
     * Stop the hook.
     * @return true if successful.
     */
    virtual bool stop() = 0;

    /**
     * Pause the hook (e.g. temporarily disable processing).
     * @return true if successful.
     */
    virtual bool pause() = 0;

    /**
     * Resume the hook.
     * @return true if successful.
     */
    virtual bool resume() = 0;
};

#endif // !_INPUT_HOOK_H
