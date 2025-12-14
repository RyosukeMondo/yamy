#pragma once
#ifndef _INPUT_HOOK_H
#define _INPUT_HOOK_H

/**
 * @file input_hook.h
 * @brief Platform-neutral interface for input hook management.
 *
 * Defines the InputHook abstract interface for installing and controlling
 * platform-specific keyboard/mouse hooks.
 */

class Engine;

/**
 * @brief Platform-neutral interface for installing/uninstalling input hooks.
 *
 * This interface abstracts the installation and lifecycle management of
 * platform-specific input hooks (e.g., SetWindowsHookEx on Windows, evdev
 * on Linux). The hook intercepts raw input events and forwards them to the
 * Engine for processing.
 *
 * @note Hooks operate at low latency (<1ms) and should not block.
 *
 * @code
 * InputHook* hook = createPlatformHook();
 * Engine* engine = createEngine();
 *
 * if (hook->start(engine)) {
 *     // Hook active, processing input
 *     hook->pause();  // Temporarily disable
 *     hook->resume(); // Re-enable
 *     hook->stop();   // Stop completely
 * }
 * @endcode
 */
class InputHook
{
public:
    virtual ~InputHook() { }

    /**
     * @brief Start the input hook.
     *
     * Installs the platform-specific hook and begins intercepting input events.
     *
     * @param i_engine Pointer to the engine instance that will receive hook callbacks.
     *                 The engine processes intercepted events and decides whether to
     *                 pass them through or suppress them.
     * @return true if successful, false on failure.
     *
     * @pre i_engine must be non-null and initialized
     * @pre Hook must not already be started
     * @post Hook is installed and actively intercepting input
     *
     * @note Typically requires elevated permissions on some platforms.
     */
    virtual bool start(Engine *i_engine) = 0;

    /**
     * @brief Stop the input hook.
     *
     * Removes the hook and stops intercepting input events.
     *
     * @return true if successful, false on failure.
     *
     * @pre Hook must have been started with start()
     * @post Hook is removed and no longer intercepting input
     *
     * @note Always call this before destroying the InputHook object.
     */
    virtual bool stop() = 0;

    /**
     * @brief Pause the hook (temporarily disable processing).
     *
     * The hook remains installed but stops forwarding events to the engine.
     * Input events pass through unmodified while paused.
     *
     * @return true if successful, false on failure.
     *
     * @pre Hook must be started and not already paused
     * @post Hook is paused, events pass through without processing
     *
     * @note Use this when temporarily disabling YAMY without full shutdown.
     */
    virtual bool pause() = 0;

    /**
     * @brief Resume the hook after pausing.
     *
     * Resumes forwarding events to the engine for processing.
     *
     * @return true if successful, false on failure.
     *
     * @pre Hook must be paused with pause()
     * @post Hook is active and processing events normally
     */
    virtual bool resume() = 0;
};

#endif // !_INPUT_HOOK_H
