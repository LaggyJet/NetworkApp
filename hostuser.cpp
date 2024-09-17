#include "HostUser.h"

HostUser::HostUser(uint16_t port, uint16_t chatCap, QChar commandChar, QObject *parent) : QObject(parent), port(port), chatCap(chatCap), commandChar(commandChar), listeningSocket(INVALID_SOCKET), running(false), users(), clientUsernames() {}

HostUser::~HostUser() { Stop(); }

void HostUser::Start() {
    if (listeningSocket != INVALID_SOCKET) {
        emit ErrorOccurred("Host is already running.");
        return;
    }
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket creation failed");
        return;
    }
    u_long mode = 1;
    ioctlsocket(listeningSocket, FIONBIO, &mode);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(listeningSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        emit ErrorOccurred("Bind failed: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        return;
    }
    if (listen(listeningSocket, 15) == SOCKET_ERROR) {
        emit ErrorOccurred("Listen failed: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        return;
    }
    running = false;
    clientHandlerThread = std::thread([this]() { while (!running) { HandleClients(); } });
    SendHostStartInfo();
}

void HostUser::Stop() {
    running = true;
    if (clientHandlerThread.joinable())
        clientHandlerThread.join();
    for (SOCKET clientSocket : clients)
        ShutdownSocket(clientSocket);
    clients.clear();
    ShutdownSocket(listeningSocket);
}

void HostUser::HandleClients() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listeningSocket, &readfds);
    timeval timeout{0, 100000};
    if (::select(0, &readfds, NULL, NULL, &timeout) == SOCKET_ERROR) {
        emit ErrorOccurred("Select failed: " + QString::number(WSAGetLastError()));
        return;
    }
    if (FD_ISSET(listeningSocket, &readfds)) {
        sockaddr_in clientAddress;
        int addrlen = sizeof(clientAddress);
        static bool hostRegedError = false;
        SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&clientAddress, &addrlen);
        if (clientSocket == INVALID_SOCKET) {
            emit ErrorOccurred("Accept failed: " + QString::number(WSAGetLastError()));
            return;
        }
        if (clientUsernames.size() == 0) {
            if (!hostRegedError) {
                emit ErrorOccurred("Accept failed: Register and Login before accepting clients");
                hostRegedError = true;
            }
            QString errorMessage = "Connection denied: Server is not ready. Make sure the host is logged in before trying to join again";
            std::string errorMsgString = errorMessage.toStdString();
            const char* errorMsgCStr = errorMsgString.c_str();
            SendMessageToClient(clientSocket, errorMsgCStr, strlen(errorMsgCStr));
            ShutdownSocket(clientSocket);
            return;
        } 
        else
            hostRegedError = false;
        clients.push_back(clientSocket);
        emit DataReceived("Private - New client connected");
        QString fullMessage = QString("Welcome to the server!\nThe command character is: ") + QString(commandChar);
        std::string messageString = fullMessage.toStdString();
        const char* welcomeMessage = messageString.c_str();
        SendMessageToClient(clientSocket, welcomeMessage, strlen(welcomeMessage));
    }
    for (auto it = clients.begin(); it != clients.end();) {
        SOCKET clientSocket = *it;
        int response = ReceiveMessageFromClient(clientSocket);
        if (response == 0) {
            emit DataReceived("Private - Client disconnected");
            ShutdownSocket(clientSocket);
            it = clients.erase(it);
        }
        else if (response == 1)
            ++it;
    }
}

int HostUser::ReceiveMessageFromClient(SOCKET clientSocket) {
    if (clientSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket is not connected");
        return 0;
    }
    char lengthByte;
    int bytesReceived = recv(clientSocket, &lengthByte, 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAECONNRESET)
            return 0;
        else if (errorCode != WSAEWOULDBLOCK && errorCode != 0) {
            emit ErrorOccurred("Failed to read message length: " + QString::number(errorCode));
            return 2;
        }
        return 1; 
    }
    if (bytesReceived == 0)
        return 0;
    uint8_t messageLength = static_cast<uint8_t>(lengthByte);
    if (messageLength == 0 || messageLength > 255) {
        emit ErrorOccurred("Invalid message length");
        return 2;
    }
    char messageBuffer[256]; 
    int totalBytesRead = 0;
    while (totalBytesRead < messageLength) {
        int curByte = recv(clientSocket, messageBuffer + totalBytesRead, messageLength - totalBytesRead, 0);
        if (curByte == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            if (errorCode != WSAEWOULDBLOCK) {
                emit ErrorOccurred("Failed to read message content: " + QString::number(errorCode));
                return 2;
            }
        }
        else if (curByte == 0)
            return 0;
        totalBytesRead += curByte;
    }
    QString data = QString::fromUtf8(messageBuffer, messageLength);
    if ((data.startsWith("regUser") || data.startsWith("logUser")) && clientUsernames.find(clientSocket) != clientUsernames.end()) {
        std::string logWarning = "Please sign out before trying to register or login";
        SendMessageToClient(clientSocket, logWarning.c_str(), logWarning.size());
    }
    else if ((data.startsWith("regUser") || data.startsWith("logUser")) && clientUsernames.find(clientSocket) == clientUsernames.end() && chatCap == clientUsernames.size() - 1) {
        std::string logWarning = "Chat is currently full, try again later.";
        SendMessageToClient(clientSocket, logWarning.c_str(), logWarning.size());
    }
    else if (data.startsWith("regUser")) {
        QStringList args = data.split(' ');
        RegisterUser(clientSocket, args[1], args[2]);
    }
    else if (data.startsWith("logUser")) {
        QStringList args = data.split(' ');
        LoginUser(clientSocket, args[1], args[2]);
    }
    else if (clientUsernames.find(clientSocket) == clientUsernames.end()) {
        std::string logWarning = "Please register and login before sending a message";
        SendMessageToClient(clientSocket, logWarning.c_str(), logWarning.size());
    }
    else {
        QString msg = clientUsernames[clientSocket] + " - " + data;
        std::string dataStr = msg.toStdString();
        SendToOtherClients(clientSocket, dataStr.c_str(), dataStr.size());
        emit DataReceived(msg);
    }
    return 1;
}

void HostUser::SendMessageToClient(SOCKET clientSocket, const char *data, int32_t length) {
    if (clientSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Invalid client socket");
        return;
    }
    if (length > 255 || length < 0) {
        emit ErrorOccurred("Invalid data size");
        return;
    }
    char lengthByte = static_cast<char>(length);
    int sentBytes = send(clientSocket, &lengthByte, 1, 0);
    if (sentBytes == SOCKET_ERROR) {
        emit ErrorOccurred("Failed to send length byte: " + QString::number(WSAGetLastError()));
        return;
    }
    int totalBytesSent = 0;
    while (totalBytesSent < length) {
        int bytesSent = send(clientSocket, data + totalBytesSent, length - totalBytesSent, 0);
        if (bytesSent == SOCKET_ERROR) {
            emit ErrorOccurred("Send failed: " + QString::number(WSAGetLastError()));
            return;
        }
        totalBytesSent += bytesSent;
    }
}

void HostUser::ShutdownSocket(SOCKET &socket) {
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_SEND);
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

void HostUser::SendHostStartInfo() {
    QString ipv4Address = "N/A";
    QString ipv6Address = "N/A";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    bool foundIPv4 = false;
    bool foundIPv6 = false;
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
            if (!foundIPv4) {
                ipv4Address = address.toString();
                foundIPv4 = true;
            }
        }
        else if (address.protocol() == QAbstractSocket::IPv6Protocol && !address.isLoopback()) {
            if (!foundIPv6) {
                ipv6Address = address.toString();
                foundIPv6 = true;
            }
        }
    }
    if (!foundIPv4)
        ipv4Address = "No external IPv4 address found";
    if (!foundIPv6)
        ipv6Address = "No external IPv6 address found";
    QString message = QString("IPv4: %1\nIPv6: %2\nPort: %3\n").arg(ipv4Address).arg(ipv6Address).arg(port);
    message += "Make sure to register and login before allowing a client to connect using ~reg and ~log\n";
    emit DataReceived(message);
}

void HostUser::SendGlobalMessage(const char *data, int32_t length) {
    if (data == nullptr || length <= 0) {
        emit ErrorOccurred("Invalid data or length");
        return;
    }
    for (SOCKET clientSocket : clients) {
        if (clientSocket == INVALID_SOCKET) {
            emit ErrorOccurred("Invalid client socket detected in clients list");
            continue;
        }
        SendMessageToClient(clientSocket, data, length);
    }
}

void HostUser::SendToOtherClients(SOCKET exSocket, const char *data, int32_t length) {
    if (data == nullptr || length <= 0) {
        emit ErrorOccurred("Invalid data or length");
        return;
    }
    for (SOCKET clientSocket : clients) {
        if (clientSocket == INVALID_SOCKET) {
            emit ErrorOccurred("Invalid client socket detected in clients list");
            continue;
        }
        if (clientSocket == exSocket)
            continue;
        SendMessageToClient(clientSocket, data, length);
    }
}

void HostUser::RegisterUser(SOCKET clientSocket, const QString &username, const QString &password) {
    const char *message;
    if (users.find(username) != users.end())
        message = "Username is already taken.";
    else {
        users[username] = password;
        QString successMessage = "Registration successful for user: " + username;
        message = successMessage.toStdString().c_str();
    }
    if (clientSocket == INVALID_SOCKET)
        emit DataReceived(message);
    else
        SendMessageToClient(clientSocket, message, strlen(message));
}

void HostUser::LoginUser(SOCKET clientSocket, const QString &username, const QString &password) {
    std::string responseMessage;
    if (users.find(username) == users.end())
        responseMessage = "Username does not exist.";
    else if (users.at(username) != password)
        responseMessage = "Password is not correct, please try again.";
    else {
        bool usernameInUse = false;
        for (const auto &pair : clientUsernames) {
            if (pair.second == username) {
                usernameInUse = true;
                break;
            }
        }
        if (usernameInUse)
            responseMessage = "Username is already in use by another client.";
        else {
            clientUsernames[clientSocket] = username;
            responseMessage = "Logged in as: " + username.toStdString();
        }
    }
    if (clientSocket == INVALID_SOCKET)
        emit DataReceived(responseMessage.c_str());
    else
        SendMessageToClient(clientSocket, responseMessage.c_str(), responseMessage.length());
}
