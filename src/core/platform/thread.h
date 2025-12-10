#pragma once
#include <cstdint>
#include "types.h"

namespace yamy::platform {

// Thread entry point function signature
using ThreadRoutine = void* (*)(void* arg);

// Cross-platform sleep function
// Sleeps for the specified number of milliseconds
void sleep_ms(uint32_t milliseconds);

// Creates a new thread
// Returns the thread handle, or nullptr on failure
ThreadHandle createThread(ThreadRoutine routine, void* arg);

// Joins a thread (waits for it to finish)
// returns true on success
bool joinThread(ThreadHandle handle);

// Detaches a thread (runs independently)
// returns true on success
bool detachThread(ThreadHandle handle);

// Sets the priority of a thread
// priority: platform dependent value
bool setThreadPriority(ThreadHandle handle, int priority);

} // namespace yamy::platform
