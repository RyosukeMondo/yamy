#include "core/platform/thread.h"
#include <windows.h>
#include <process.h>

namespace yamy::platform {

// Helper structure to pass thread routine info to Windows thread entry
struct ThreadStartInfo {
    ThreadRoutine routine;
    void* arg;
};

// Windows thread entry point that calls the user's routine
static unsigned int __stdcall threadEntry(void* param) {
    ThreadStartInfo* info = static_cast<ThreadStartInfo*>(param);
    ThreadRoutine routine = info->routine;
    void* arg = info->arg;
    delete info;

    routine(arg);
    return 0;
}

void sleep_ms(uint32_t milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

ThreadHandle createThread(ThreadRoutine routine, void* arg) {
    ThreadStartInfo* info = new ThreadStartInfo{routine, arg};
    uintptr_t handle = _beginthreadex(nullptr, 0, threadEntry, info, 0, nullptr);
    if (handle == 0) {
        delete info;
        return nullptr;
    }
    return reinterpret_cast<ThreadHandle>(handle);
}

bool joinThread(ThreadHandle handle) {
    if (!handle) return false;
    DWORD result = WaitForSingleObject(static_cast<HANDLE>(handle), INFINITE);
    return result == WAIT_OBJECT_0;
}

bool detachThread(ThreadHandle handle) {
    // Windows threads don't have a detach concept like pthread
    // Just close the handle to indicate we don't need to wait for it
    return destroyThread(handle);
}

bool destroyThread(ThreadHandle handle) {
    if (!handle) return false;
    return CloseHandle(static_cast<HANDLE>(handle)) != 0;
}

bool setThreadPriority(ThreadHandle handle, int priority) {
    if (!handle) return false;
    // Map generic priority to Windows priority
    // priority range: -2 (lowest) to +2 (highest)
    int winPriority;
    if (priority <= -2) winPriority = THREAD_PRIORITY_LOWEST;
    else if (priority == -1) winPriority = THREAD_PRIORITY_BELOW_NORMAL;
    else if (priority == 0) winPriority = THREAD_PRIORITY_NORMAL;
    else if (priority == 1) winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
    else winPriority = THREAD_PRIORITY_HIGHEST;

    return SetThreadPriority(static_cast<HANDLE>(handle), winPriority) != 0;
}

} // namespace yamy::platform
