//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_linux.cpp - Unix IPC using domain sockets (Track 7)

#include "ipc_linux.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <errno.h>

namespace yamy::platform {

// Helper to ensure all data is sent, handling partial writes and EINTR
static bool sendAll(int sock, const void* buffer, size_t length) {
    const char* ptr = static_cast<const char*>(buffer);
    size_t remaining = length;
    while (remaining > 0) {
        // Use MSG_NOSIGNAL to prevent SIGPIPE on closed connection
        ssize_t sent = send(sock, ptr, remaining, MSG_NOSIGNAL);
        if (sent <= 0) {
            if (sent < 0 && errno == EINTR) {
                continue; // Retry on interruption
            }
            // Error or connection closed
            return false;
        }
        ptr += sent;
        remaining -= sent;
    }
    return true;
}

bool IPCLinux::sendCopyData(WindowHandle sender,
                           WindowHandle target,
                           const CopyData& data,
                           uint32_t flags,
                           uint32_t timeout_ms,
                           uintptr_t* result)
{
    // 1. Create Unix domain socket (AF_UNIX, SOCK_STREAM)
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "[IPC] Failed to create socket" << std::endl;
        return false;
    }

    // 2. Connect to /tmp/yamy_{target_handle}.sock
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // Format socket path using target handle (WindowHandle is void*)
    // We use %p to format the handle as a pointer/ID.
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/yamy_%p.sock", target);

    // Note: timeout_ms is ignored for connect as per instructions (blocking connect)
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        // Target might not be listening or path is wrong
        // std::cerr << "[IPC] Failed to connect to " << addr.sun_path << std::endl;
        close(sock);
        return false;
    }

    // 3. Send: data.id, data.size, data.data
    bool success = true;

    // Send ID
    if (!sendAll(sock, &data.id, sizeof(data.id))) {
        success = false;
    }

    // Send Size
    if (success && !sendAll(sock, &data.size, sizeof(data.size))) {
        success = false;
    }

    // Send Data payload
    if (success && data.size > 0 && data.data) {
        if (!sendAll(sock, data.data, data.size)) {
            success = false;
        }
    }

    // 4. Close socket
    close(sock);

    if (success) {
        // If result pointer is provided, we can't really return a value from the other process
        // in this simple one-way protocol. Just set it to 1 (true/success).
        if (result) {
            *result = 1;
        }
    }

    return success;
}

} // namespace yamy::platform
