//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_linux.cpp - Unix IPC using domain sockets (Track 7)

#include "ipc_linux.h"
#include "../../utils/platform_logger.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cerrno>

namespace yamy::platform {

// Helper to ensure all data is sent, handling partial writes and EINTR
static bool sendAll(int sock, const void* buffer, size_t length) {
    const char* ptr = static_cast<const char*>(buffer);
    size_t remaining = length;
    while (remaining > 0) {
        // Use MSG_NOSIGNAL to prevent SIGPIPE on closed connection
        ssize_t sent = send(sock, ptr, remaining, MSG_NOSIGNAL);
        if (sent <= 0) {
            // Error or connection closed
            PLATFORM_LOG_DEBUG("ipc", "sendAll: send failed after %zu bytes: %s",
                               length - remaining, std::strerror(errno));
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
    (void)sender;
    (void)flags;
    (void)timeout_ms;

    // 1. Create Unix domain socket (AF_UNIX, SOCK_STREAM)
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        PLATFORM_LOG_ERROR("ipc", "Failed to create socket: %s", std::strerror(errno));
        return false;
    }

    // 2. Connect to /tmp/yamy_{target_handle}.sock
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // Format socket path using target handle (WindowHandle is void*)
    // We use %p to format the handle as a pointer/ID.
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/yamy_%p.sock", target);

    PLATFORM_LOG_DEBUG("ipc", "sendCopyData: connecting to %s", addr.sun_path);

    // Note: timeout_ms is ignored for connect as per instructions (blocking connect)
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        // Target might not be listening or path is wrong
        PLATFORM_LOG_DEBUG("ipc", "sendCopyData: connect failed: %s", std::strerror(errno));
        close(sock);
        return false;
    }

    PLATFORM_LOG_DEBUG("ipc", "sendCopyData: connected, sending id=%u size=%zu", data.id, data.size);

    // 3. Send: data.id, data.size, data.data
    bool success = true;

    // Send ID
    if (!sendAll(sock, &data.id, sizeof(data.id))) {
        PLATFORM_LOG_WARN("ipc", "sendCopyData: failed to send id");
        success = false;
    }

    // Send Size
    if (success && !sendAll(sock, &data.size, sizeof(data.size))) {
        PLATFORM_LOG_WARN("ipc", "sendCopyData: failed to send size");
        success = false;
    }

    // Send Data payload
    if (success && data.size > 0 && data.data) {
        if (!sendAll(sock, data.data, data.size)) {
            PLATFORM_LOG_WARN("ipc", "sendCopyData: failed to send payload");
            success = false;
        }
    }

    // 4. Close socket
    close(sock);

    if (success) {
        PLATFORM_LOG_DEBUG("ipc", "sendCopyData: success (id=%u size=%zu)", data.id, data.size);
        // If result pointer is provided, we can't really return a value from the other process
        // in this simple one-way protocol. Just set it to 1 (true/success).
        if (result) {
            *result = 1;
        }
    } else {
        PLATFORM_LOG_WARN("ipc", "sendCopyData: failed");
    }

    return success;
}

} // namespace yamy::platform
