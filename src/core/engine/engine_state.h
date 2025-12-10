#pragma once

#ifndef _ENGINE_STATE_H
#define _ENGINE_STATE_H

namespace yamy {

/// Defines the major operational states of the Engine
enum class EngineState {
    Stopped,    /// The engine is not running and not processing input.
    Loading,    /// The engine is starting up and loading the configuration.
    Running,    /// The engine is active and processing input.
    Error,      /// The engine has encountered a fatal error and is stopped.
};

} // namespace yamy

#endif // _ENGINE_STATE_H
