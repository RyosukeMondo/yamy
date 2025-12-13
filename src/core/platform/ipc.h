#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc.h - Platform-agnostic inter-process communication

#ifndef _PLATFORM_IPC_H
#define _PLATFORM_IPC_H

#include <cstdint>

namespace yamy::platform {
    /// Copy data structure for IPC
    struct CopyData {
        uint32_t id;        /// Data identifier
        uint32_t size;      /// Data size in bytes
        const void* data;   /// Pointer to data
    };

    /// Send message flags
    namespace SendMessageFlags {
        constexpr uint32_t BLOCK = 0x0001;           /// Block until processed
        constexpr uint32_t ABORT_IF_HUNG = 0x0002;   /// Abort if target hung
        constexpr uint32_t NORMAL = BLOCK | ABORT_IF_HUNG;
    }

} // namespace yamy::platform

#endif // _PLATFORM_IPC_H
