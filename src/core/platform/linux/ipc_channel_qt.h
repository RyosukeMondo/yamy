#pragma once

#include "core/platform/ipc_channel_interface.h"
#include <QLocalSocket>
#include <QLocalServer>
#include <QByteArray>
#include <memory>
#include <string>

namespace yamy::platform {

/**
 * @class IPCChannelQt
 * @brief Qt-based IPC channel using Unix domain sockets
 *
 * Implements IIPCChannel interface for Linux using QLocalSocket (client)
 * and QLocalServer (server) for inter-process communication between
 * the Qt GUI and the engine process.
 *
 * Thread Safety: All operations are thread-safe via Qt's queued connections.
 *
 * Example Usage:
 * @code
 * IPCChannelQt channel("my-client");
 * QObject::connect(&channel, &IPCChannelQt::messageReceived, this, &MyClass::onMessage);
 * channel.connect("yamy-engine");
 * @endcode
 */
class IPCChannelQt : public IIPCChannel {
    Q_OBJECT

public:
    /**
     * @brief Construct a new IPCChannelQt object
     * @param name Channel name for socket path generation
     */
    explicit IPCChannelQt(const std::string& name);

    /**
     * @brief Destructor - cleans up connections and servers
     */
    ~IPCChannelQt() override;

    /**
     * @brief Connects to a named IPC server using Unix domain socket
     *
     * Initiates asynchronous connection to the specified IPC server.
     * The connection is non-blocking; use isConnected() or wait for
     * the connected() signal to verify successful connection.
     *
     * @param name Server name (e.g., "yamy-engine")
     *             Socket path will be /tmp/yamy-{name}-{UID}
     *
     * @note This method does not block. Connection happens asynchronously.
     * @note If already connected, this method disconnects first.
     *
     * @see isConnected(), disconnected() signal
     */
    void connect(const std::string& name) override;

    /**
     * @brief Disconnect from the IPC server
     *
     * Closes the client socket connection. Safe to call even if not connected.
     */
    void disconnect() override;

    /**
     * @brief Start listening for incoming connections (server mode)
     *
     * Creates a QLocalServer and begins listening on the socket path.
     * The socket path is /tmp/yamy-{name}-{UID} where name was provided
     * in the constructor and UID is the current user ID.
     *
     * @note Only one server can listen on a given socket path.
     * @note If socket file already exists, it will be removed first.
     */
    void listen() override;

    /**
     * @brief Check if the channel is currently connected
     *
     * @return true if client socket is connected or server has active connections
     * @return false otherwise
     */
    bool isConnected() override;

    /**
     * @brief Send a message through the IPC channel
     *
     * Serializes the message with a 4-byte length prefix (big-endian) followed
     * by the message data, and sends it through the socket.
     *
     * @param msg Message to send (contains type, data pointer, and size)
     *
     * @note This is asynchronous - the method returns before data is transmitted.
     * @note If not connected, the message is silently dropped.
     */
    void send(const ipc::Message& msg) override;

    /**
     * @brief Non-blocking receive of a message
     *
     * @return std::unique_ptr<ipc::Message> Received message, or nullptr if no message available
     *
     * @note This implementation uses signal/slot mechanism, so this method
     *       always returns nullptr. Use messageReceived() signal instead.
     */
    std::unique_ptr<ipc::Message> nonBlockingReceive() override;

private slots:
    /**
     * @brief Handle incoming data on the socket
     *
     * Called by Qt when data is available. Accumulates data in the receive
     * buffer, parses message frames (4-byte length + data), and emits
     * messageReceived() signal for each complete message.
     */
    void onReadyRead();

    /**
     * @brief Handle successful connection
     *
     * Called when client socket successfully connects to server.
     */
    void onConnected();

    /**
     * @brief Handle disconnection
     *
     * Called when socket is disconnected (either client or server side).
     * Cleans up state and prepares for potential reconnection.
     */
    void onDisconnected();

    /**
     * @brief Handle new incoming connection (server mode)
     *
     * Called when a client connects to our server. Sets up the client
     * socket for communication.
     */
    void onNewConnection();

    /**
     * @brief Handle socket errors
     *
     * Logs the error and emits disconnected() signal if appropriate.
     *
     * @param error The socket error that occurred
     */
    void onError(QLocalSocket::LocalSocketError error);

private:
    /**
     * @brief Generate socket path for the given name
     *
     * Creates a user-specific socket path: /tmp/yamy-{name}-{UID}
     *
     * @param name Channel name
     * @return std::string Socket path
     */
    std::string getSocketPath(const std::string& name) const;

    /**
     * @brief Process accumulated receive buffer
     *
     * Parses the buffer for complete messages (4-byte length prefix + data),
     * deserializes them, and emits messageReceived() signal.
     * Keeps partial messages in the buffer for next call.
     *
     * @param socket The socket that sent the data (used for logging, can be nullptr)
     * @param buffer The buffer to process
     */
    void processReceiveBuffer(QLocalSocket* socket, QByteArray& buffer);

    std::string m_name;                           ///< Channel name
    QLocalSocket* m_clientSocket;                 ///< Client socket (client mode)
    QLocalServer* m_server;                       ///< Server socket (server mode)
    QList<QLocalSocket*> m_serverClientSockets;   ///< Connected clients (server mode)
    QByteArray m_receiveBuffer;                   ///< Buffer for partial messages (client mode)
    QMap<QLocalSocket*, QByteArray> m_clientReceiveBuffers;  ///< Per-client buffers (server mode)
    bool m_isServerMode;                          ///< True if in server mode, false if client
};

} // namespace yamy::platform
