#include "clientuser.h"

ClientUser::ClientUser(const QString &address, int port) : socket(INVALID_SOCKET), running(false) {
    socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket creation failed.");
        return;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.toStdString().c_str(), &serverAddr.sin_addr);
}

ClientUser::~ClientUser() { Stop(); }

void ClientUser::Start() {
    if (running)
        return;
    running = true;
    if (::connect(socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        emit ErrorOccurred("Connect failed: " + QString::number(errorCode));
        Stop();  
        return;
    }
    readThread = std::thread(&ClientUser::ReadingThread, this);
}

void ClientUser::Stop() {
    if (!running)
        return;
    running = false;
    ShutdownSocket();
    if (readThread.joinable())
        readThread.join();
}

void ClientUser::ReadingThread() {
    while (running) {
        if (socket == INVALID_SOCKET)
            break;

        QByteArray buffer(256, '\0');
        int messageLength = 0;
        if (!running)
            break;
        if (ReadData(buffer.data(), buffer.size(), messageLength) && messageLength > 0)
            QMetaObject::invokeMethod(this, "DataReceived", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(buffer.data(), messageLength)));
    }
}

void ClientUser::SendData(const char *data, int32_t length) {
    if (socket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket is not connected");
        Stop();
        return;
    }
    if (length > 255 || length <= 0) {
        emit DataReceived("Invalid data size");
        return;
    }
    char lengthByte = (char)length;
    if (send(socket, &lengthByte, 1, 0) == SOCKET_ERROR) {
        emit DataReceived("Failed to send length byte: " + QString::number(WSAGetLastError()));
        return;
    }
    int totalSent = 0;
    while (totalSent < length) {
        int curByte = send(socket, data + totalSent, length - totalSent, 0);
        if (curByte == SOCKET_ERROR) {
            emit DataReceived("Failed to send data: " + QString::number(WSAGetLastError()));
            return;
        }
        totalSent += curByte;
    }
}

bool ClientUser::ReadData(char *buffer, int32_t size, int32_t &returnedMsgSize) {
    if (socket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket is not connected");
        Stop();
        return false;
    }
    char lengthByte;
    int bytesReceived = recv(socket, &lengthByte, 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode != WSAEINTR && errorCode != WSAECONNRESET)
            emit DataReceived("Failed to read length byte: " + QString::number(errorCode));
        return false;
    } 
    else if (bytesReceived == 0) {
        emit ErrorOccurred("Connection closed by server");
        ShutdownSocket();
        return false;
    }
    returnedMsgSize = lengthByte;
    int length = (int)lengthByte;
    if (length > size) {
        emit DataReceived("Message too large to receive");
        return false;
    }
    int bytesRead = 0;
    while (bytesRead < length) {
        int curByte = recv(socket, buffer + bytesRead, length - bytesRead, 0);
        if (curByte == SOCKET_ERROR) {
            emit DataReceived("Failed to read data: " + QString::number(WSAGetLastError()));
            return false;
        } else if (curByte == 0) {
            emit ErrorOccurred("Connection closed by server");
            ShutdownSocket();
            return false;
        }
        bytesRead += curByte;
    }
    return true;
}

void ClientUser::ShutdownSocket() {
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_BOTH);
        closesocket(socket);
        socket = INVALID_SOCKET;
        emit DataReceived("clsCon");
    }
}
