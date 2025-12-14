#pragma once

#ifndef _ENGINE_STATE_H
#define _ENGINE_STATE_H

/**
 * @file engine_state.h
 * @brief Defines the operational state machine for the YAMY engine.
 *
 * This file contains the core state enumeration that tracks the engine's
 * lifecycle from startup through normal operation to shutdown or error states.
 */

namespace yamy {

/**
 * @brief Defines the major operational states of the Engine.
 *
 * The engine follows a strict state machine:
 * - Stopped -> Loading -> Running (normal startup)
 * - Running -> Stopped (normal shutdown)
 * - Any state -> Error (on fatal error)
 *
 * State transitions are managed internally by the Engine class.
 *
 * @see Engine::getState()
 */
enum class EngineState {
    Stopped,    ///< The engine is not running and not processing input.
    Loading,    ///< The engine is starting up and loading the configuration.
    Running,    ///< The engine is active and processing input.
    Error,      ///< The engine has encountered a fatal error and is stopped.
};

} // namespace yamy

#endif // _ENGINE_STATE_H
