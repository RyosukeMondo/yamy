#include "ipc_channel_qt.h"
#include <QDataStream>
#include <QBuffer>
#include <QFile>
#include <iostream>
#include <climits>
#include <unistd.h>

namespace yamy::platform {

IPCChannelQt::IPCChannelQt(const std::string& name)
    : m_name(name)
    , m_clientSocket(nullptr)
    , m_server(nullptr)
    , m_serverClientSocket(nullptr)
    , m_isServerMode(false)
{
}

IPCChannelQt::~IPCChannelQt() {
    disconnect();
    if (m_server) {
        m_server->close();
        delete m_server;
    }
}

std::string IPCChannelQt::getSocketPath(const std::string& name) const {
    uid_t uid = getuid();
    return "/tmp/yamy-" + name + "-" + std::to_string(uid);
}

void IPCChannelQt::connect(const std::string& name) {
    // Disconnect if already connected
    if (m_clientSocket && m_clientSocket->state() != QLocalSocket::UnconnectedState) {
        disconnect();
    }

    m_isServerMode = false;

    // Create client socket if needed
    if (!m_clientSocket) {
        m_clientSocket = new QLocalSocket(this);

        // Connect signals
        QObject::connect(m_clientSocket, &QLocalSocket::connected,
                        this, &IPCChannelQt::onConnected);
        QObject::connect(m_clientSocket, &QLocalSocket::disconnected,
                        this, &IPCChannelQt::onDisconnected);
        QObject::connect(m_clientSocket, &QLocalSocket::readyRead,
                        this, &IPCChannelQt::onReadyRead);
        QObject::connect(m_clientSocket,
                        QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
                        this, &IPCChannelQt::onError);
    }

    // Initiate connection
    std::string socketPath = getSocketPath(name);
    m_clientSocket->connectToServer(QString::fromStdString(socketPath));
}

void IPCChannelQt::disconnect() {
    if (m_clientSocket) {
        m_clientSocket->disconnectFromServer();
        if (m_clientSocket->state() != QLocalSocket::UnconnectedState) {
            m_clientSocket->waitForDisconnected(1000);
        }
    }

    if (m_serverClientSocket) {
        m_serverClientSocket->disconnectFromServer();
        m_serverClientSocket = nullptr;
    }

    m_receiveBuffer.clear();
}

void IPCChannelQt::listen() {
    m_isServerMode = true;

    if (!m_server) {
        m_server = new QLocalServer(this);

        QObject::connect(m_server, &QLocalServer::newConnection,
                        this, &IPCChannelQt::onNewConnection);
    }

    std::string socketPath = getSocketPath(m_name);
    QString qSocketPath = QString::fromStdString(socketPath);

    // Remove existing socket file if it exists
    QFile::remove(qSocketPath);

    if (!m_server->listen(qSocketPath)) {
        std::cerr << "[IPCChannelQt] Failed to listen on " << socketPath
                  << ": " << m_server->errorString().toStdString() << std::endl;
    } else {
        std::cout << "[IPCChannelQt] Listening on " << socketPath << std::endl;
    }
}

bool IPCChannelQt::isConnected() {
    if (m_isServerMode) {
        return m_serverClientSocket != nullptr &&
               m_serverClientSocket->state() == QLocalSocket::ConnectedState;
    } else {
        return m_clientSocket != nullptr &&
               m_clientSocket->state() == QLocalSocket::ConnectedState;
    }
}

void IPCChannelQt::send(const ipc::Message& msg) {
    QLocalSocket* socket = nullptr;

    if (m_isServerMode) {
        socket = m_serverClientSocket;
    } else {
        socket = m_clientSocket;
    }

    if (!socket || socket->state() != QLocalSocket::ConnectedState) {
        return;  // Silently drop if not connected
    }

    // Serialize message: 4-byte length (big-endian) + MessageType (4 bytes) + data
    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream.setByteOrder(QDataStream::BigEndian);

    // Calculate total size: MessageType (4 bytes) + data size
    uint32_t totalSize = sizeof(uint32_t) + static_cast<uint32_t>(msg.size);

    // Write length prefix
    stream << totalSize;

    // Write message type
    stream << static_cast<uint32_t>(msg.type);

    // Write message data - validate size is within reasonable bounds
    if (msg.data && msg.size > 0 && msg.size < INT_MAX) {
        stream.writeRawData(static_cast<const char*>(msg.data), static_cast<int>(msg.size));
    } else if (msg.size >= INT_MAX) {
        std::cerr << "[IPCChannelQt] Error: Message size too large: " << msg.size << std::endl;
        return;  // Drop message
    }

    // Send to socket
    buffer.close();
    socket->write(bufferData);
    socket->flush();
}

std::unique_ptr<ipc::Message> IPCChannelQt::nonBlockingReceive() {
    // This implementation uses signal/slot mechanism
    // Messages are delivered via messageReceived() signal
    return nullptr;
}

void IPCChannelQt::onReadyRead() {
    QLocalSocket* socket = nullptr;

    if (m_isServerMode) {
        socket = qobject_cast<QLocalSocket*>(sender());
        if (!socket) return;
    } else {
        socket = m_clientSocket;
    }

    if (!socket) return;

    // Read all available data into buffer
    m_receiveBuffer.append(socket->readAll());

    // Process complete messages
    processReceiveBuffer();
}

void IPCChannelQt::processReceiveBuffer() {
    while (true) {
        // Need at least 4 bytes for length prefix
        if (m_receiveBuffer.size() < 4) {
            break;
        }

        // Read length prefix (big-endian) - copy first 4 bytes to ensure read-only access
        QByteArray lengthBytes = m_receiveBuffer.left(4);
        QDataStream lengthStream(&lengthBytes, QIODevice::ReadOnly);
        lengthStream.setByteOrder(QDataStream::BigEndian);
        uint32_t messageSize;
        lengthStream >> messageSize;

        // Check if we have the complete message
        if (m_receiveBuffer.size() < static_cast<int>(4 + messageSize)) {
            break;  // Wait for more data
        }

        // Extract message data (skip the 4-byte length prefix)
        QByteArray messageData = m_receiveBuffer.mid(4, messageSize);

        // Remove processed message from buffer
        m_receiveBuffer.remove(0, 4 + messageSize);

        // Deserialize message - use QDataStream with ReadOnly mode
        QDataStream messageStream(&messageData, QIODevice::ReadOnly);
        messageStream.setByteOrder(QDataStream::BigEndian);

        uint32_t messageType;
        messageStream >> messageType;

        // Read remaining data - validate size
        size_t dataSize = messageSize - sizeof(uint32_t);
        QByteArray data;
        if (dataSize > 0 && dataSize < INT_MAX) {
            data.resize(dataSize);
            messageStream.readRawData(data.data(), static_cast<int>(dataSize));
        } else if (dataSize >= INT_MAX) {
            std::cerr << "[IPCChannelQt] Error: Data size too large: " << dataSize << std::endl;
            continue;  // Skip this message
        }

        // Create message struct
        // Note: We need to allocate memory for the data that will outlive this function
        // The receiver is responsible for managing this memory
        void* msgData = nullptr;
        if (dataSize > 0) {
            msgData = malloc(dataSize);
            memcpy(msgData, data.data(), dataSize);
        }

        ipc::Message msg;
        msg.type = static_cast<ipc::MessageType>(messageType);
        msg.data = msgData;
        msg.size = dataSize;

        // Emit signal
        emit messageReceived(msg);

        // Clean up allocated memory
        if (msgData) {
            free(msgData);
        }
    }
}

void IPCChannelQt::onConnected() {
    std::cout << "[IPCChannelQt] Connected to server" << std::endl;
}

void IPCChannelQt::onDisconnected() {
    std::cout << "[IPCChannelQt] Disconnected from server" << std::endl;
    m_receiveBuffer.clear();
}

void IPCChannelQt::onNewConnection() {
    if (!m_server) return;

    QLocalSocket* clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) return;

    std::cout << "[IPCChannelQt] New client connected" << std::endl;

    // If we already have a client, disconnect it
    if (m_serverClientSocket) {
        m_serverClientSocket->disconnectFromServer();
        m_serverClientSocket->deleteLater();
    }

    m_serverClientSocket = clientSocket;

    // Connect signals
    QObject::connect(m_serverClientSocket, &QLocalSocket::readyRead,
                    this, &IPCChannelQt::onReadyRead);
    QObject::connect(m_serverClientSocket, &QLocalSocket::disconnected,
                    this, &IPCChannelQt::onDisconnected);
}

void IPCChannelQt::onError(QLocalSocket::LocalSocketError error) {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        std::cerr << "[IPCChannelQt] Socket error: "
                  << socket->errorString().toStdString() << std::endl;
    }

    // Emit disconnected signal for certain errors
    if (error == QLocalSocket::PeerClosedError ||
        error == QLocalSocket::ConnectionRefusedError ||
        error == QLocalSocket::SocketAccessError ||
        error == QLocalSocket::SocketResourceError) {
        onDisconnected();
    }
}

} // namespace yamy::platform
