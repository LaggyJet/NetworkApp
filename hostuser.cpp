#include "HostUser.h"
#include "logger.h"

HostUser::HostUser(uint16_t port, uint16_t chatCap, QChar commandChar) : port(port), chatCap(chatCap), commandChar(commandChar), listeningSocket(INVALID_SOCKET), udpSocket(INVALID_SOCKET), running(false), users(), clientUsernames(), lastUdpBroadcastTime(std::chrono::steady_clock::now()) {}

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
    if (ioctlsocket(listeningSocket, FIONBIO, &mode) == SOCKET_ERROR) {
        emit ErrorOccurred("Failed to set TCP socket to non-blocking mode: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        return;
    }
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
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        emit ErrorOccurred("UDP Socket creation failed");
        ShutdownSocket(listeningSocket);
        return;
    }
    bool broadcastEnabled = TRUE;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastEnabled, sizeof(broadcastEnabled)) == SOCKET_ERROR) {
        emit ErrorOccurred("Failed to enable broadcast on UDP socket: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        ShutdownSocket(udpSocket);
        return;
    }
    if (bind(udpSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        emit ErrorOccurred("UDP Bind failed: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        ShutdownSocket(udpSocket);
        return;
    }
    if (ioctlsocket(udpSocket, FIONBIO, &mode) == SOCKET_ERROR) {
        emit ErrorOccurred("Failed to set UDP socket to non-blocking mode: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        ShutdownSocket(udpSocket);
        return;
    }
    running = true; 
    clientHandlerThread = std::thread([this]() { while (running) { HandleClients(); HandleUDP(); } });
    SendHostStartInfo();
}

void HostUser::Stop() {
    running = false;
    if (clientHandlerThread.joinable())
        clientHandlerThread.join();
    for (SOCKET clientSocket : clients)
        ShutdownSocket(clientSocket);
    clients.clear();
    ShutdownSocket(listeningSocket);
    ShutdownSocket(udpSocket);
}

void HostUser::HandleUDP() {
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = currentTime - lastUdpBroadcastTime;
    if (elapsed.count() >= 5.0) {
        const std::string broadcastMessage = "Base UDP message from host.";
        sockaddr_in broadcastAddr;
        broadcastAddr.sin_family = AF_INET;
        broadcastAddr.sin_port = htons(port);
        broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
        int sentBytes = sendto(udpSocket, broadcastMessage.c_str(), broadcastMessage.size(), 0, (sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
        if (sentBytes == SOCKET_ERROR)
            emit DataReceived("Failed to send UDP broadcast: " + QString::number(WSAGetLastError()));
        else
            lastUdpBroadcastTime = currentTime; 
    }
}

void HostUser::HandleClients() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listeningSocket, &readfds);
    timeval timeout{0, 100000};
    if (select(0, &readfds, NULL, NULL, &timeout) == SOCKET_ERROR) {
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
                emit DataReceived("Accept failed: Register and Login before accepting clients");
                hostRegedError = true;
            }
            std::string errorMessage = "Connection denied: Server is not ready. Make sure the host is logged in before trying to join again";
            SendMessageToClient(clientSocket, errorMessage.c_str(), errorMessage.size());
            ShutdownSocket(clientSocket);
            return;
        }
        else if (chatCap == clientUsernames.size() - 1) {
            if (!hostRegedError) {
                emit DataReceived("Accept failed: Server is full");
                hostRegedError = true;
            }
            std::string logWarning = "Chat is currently full, try again later";
            SendMessageToClient(clientSocket, logWarning.c_str(), logWarning.size());
            ShutdownSocket(clientSocket);
        }
        else
            hostRegedError = false;
        clients.push_back(clientSocket);
        emit DataReceived("Private - New client connected");
        QString fullMessage = QString("Welcome to the server!\nThe command character is: ") + QString(commandChar);
        std::string messageString = fullMessage.toStdString();
        const char* welcomeMessage = messageString.c_str();
        SendMessageToClient(clientSocket, welcomeMessage, strlen(welcomeMessage));
        SyncTableInit(clientSocket);
    }
    for (auto it = clients.begin(); it != clients.end();) {
        SOCKET clientSocket = *it;
        int response = ReceiveMessageFromClient(clientSocket);
        if (response == 0) {
            if (clientUsernames.find(clientSocket) != clientUsernames.end()) {
                QString msg = clientUsernames[clientSocket] + " disconnected from the server";
                emit DataReceived(msg);
                SendToOtherClients(clientSocket, msg.toStdString().c_str(), msg.size());
                SyncTableAddRemove(clientUsernames[clientSocket], false, true);
            }
            clientUsernames.erase(clientSocket);
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
            emit DataReceived("Failed to read message length: " + QString::number(errorCode));
            return 2;
        }
        return 1; 
    }
    if (bytesReceived == 0)
        return 0;
    uint8_t messageLength = static_cast<uint8_t>(lengthByte);
    if (messageLength == 0 || messageLength > 255) {
        emit DataReceived("Invalid message length");
        return 2;
    }
    char messageBuffer[256]; 
    int totalBytesRead = 0;
    while (totalBytesRead < messageLength) {
        int curByte = recv(clientSocket, messageBuffer + totalBytesRead, messageLength - totalBytesRead, 0);
        if (curByte == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            if (errorCode != WSAEWOULDBLOCK) {
                emit DataReceived("Failed to read message content: " + QString::number(errorCode));
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
    else if (data.startsWith("regUser")) {
        QStringList args = data.split(' ');
        RegisterUser(clientSocket, args[1], args[2]);
    }
    else if (data.startsWith("logUser")) {
        QStringList args = data.split(' ');
        LoginUser(clientSocket, args[1], args[2]);
    }
    else if (data.startsWith("dmUser")) {
        QStringList args = data.split("\\");
        args[0].remove("dmUser ");
        SendDM(args[0], args[1], clientSocket);
    }
    else if (data == "getLog")
        GetLog(clientSocket);
    else if (data.startsWith("logCmd-")) {
        QStringList args = data.split("-");
        LogCmd(args[1], args[2]);
    }
    else if (data.startsWith("logMsg-")) {
        QStringList args = data.split("-");
        LogMsg(args[1], args[2]);
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
        emit DataReceived("Invalid data size");
        return;
    }
    char lengthByte = static_cast<char>(length);
    int sentBytes = send(clientSocket, &lengthByte, 1, 0);
    if (sentBytes == SOCKET_ERROR) {
        emit DataReceived("Failed to send length byte: " + QString::number(WSAGetLastError()));
        return;
    }
    int totalBytesSent = 0;
    while (totalBytesSent < length) {
        int bytesSent = send(clientSocket, data + totalBytesSent, length - totalBytesSent, 0);
        if (bytesSent == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            if (errorCode != WSAECONNABORTED)
                emit DataReceived("Send failed: " + QString::number(errorCode));
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
    QString message = "IPv4: " + ipv4Address + "\nIPv6: " + ipv6Address +"\nPort: " + QString::number(port) + "\nMake sure to register and login before allowing a client to connect using ~reg and ~log\n";
    emit DataReceived(message);
}

void HostUser::SendGlobalMessage(const char *data, int32_t length) {
    if (data == nullptr || length <= 0) {
        emit DataReceived("Invalid data or length");
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
        emit DataReceived("Invalid data or length");
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
    if (users.find(username) == users.end()) {
        responseMessage = "Username does not exist.";
        SendMessageToClient(clientSocket, responseMessage.c_str(), responseMessage.length());
        return;
    }
    else if (users.at(username) != password) {
        responseMessage = "Password is not correct, please try again.";
        SendMessageToClient(clientSocket, responseMessage.c_str(), responseMessage.length());
        return;
    }
    bool usernameInUse = false;
    for (const auto &pair : clientUsernames) {
        if (pair.second == username) {
            usernameInUse = true;
            break;
        }
    }
    if (usernameInUse) {
        responseMessage = "Username is already in use by another client.";
        SendMessageToClient(clientSocket, responseMessage.c_str(), responseMessage.length());
        return;
    }
    else {
        clientUsernames[clientSocket] = username;
        responseMessage = "Logged in as: " + username.toStdString();
    }
    if (clientSocket == INVALID_SOCKET) {
        emit DataReceived(responseMessage.c_str());
        SyncTableInit(clientSocket);
    }
    else
        SendMessageToClient(clientSocket, responseMessage.c_str(), responseMessage.length());
    QString msg = clientUsernames[clientSocket] + " connected to the server";
    emit DataReceived(msg);
    SendToOtherClients(clientSocket, msg.toStdString().c_str(), msg.size());
    SyncTableAddRemove(clientUsernames[clientSocket], false, false);
    std::string message = "loggedUser-" + clientUsernames[clientSocket].toStdString();
    if (clientSocket == INVALID_SOCKET)
        emit DataReceived(message.c_str());
    else
        SendMessageToClient(clientSocket, message.c_str(), message.size());
}

void HostUser::SyncTableInit(SOCKET clientSocket) {
    std::string serializedData;
    QString hostUsername;
    for (const auto &pair : clientUsernames) {
        if (pair.first == INVALID_SOCKET)
            hostUsername = pair.second;
        else
            serializedData += pair.second.toStdString() + "\\client;";
    }
    if (!hostUsername.isEmpty())
        serializedData = hostUsername.toStdString() + "\\host;" + serializedData;
    const size_t maxChunkSize = 255 - 8;
    std::vector<QString> chunks;
    QString currentChunk = "TabInit-";
    size_t currentChunkSize = currentChunk.size();
    QString currentUsername;
    for (size_t i = 0; i < serializedData.size(); ++i) {
        currentUsername += serializedData[i];
        if (serializedData[i] == ';') {
            if (currentChunkSize + currentUsername.size() > maxChunkSize) {
                chunks.push_back(currentChunk);
                currentChunk = "TabInit-";
                currentChunkSize = currentChunk.size();
            }
            currentChunk += currentUsername;
            currentChunkSize += currentUsername.size();
            currentUsername.clear();
        }
    }
    if (!currentChunk.isEmpty())
        chunks.push_back(currentChunk);
    for (const auto &chunk : chunks) {
        if (clientSocket == INVALID_SOCKET)
            emit DataReceived(chunk);
        else
            SendMessageToClient(clientSocket, chunk.toStdString().c_str(), chunk.size());
    }
}

void HostUser::SyncTableAddRemove(const QString &username, bool isHost, bool removed) {
    QString msg = (removed ? "TabRem-" : "TabAdd-") + username + "\\" + (isHost ? "Host" : "Client");
    emit DataReceived(msg);
    SendGlobalMessage(msg.toStdString().c_str(), msg.size());
}

void HostUser::SendDM(const QString &username, const QString &message, SOCKET msgSender) {
    SOCKET user = INVALID_SOCKET;
    for (const auto &pair : clientUsernames) {
        if (pair.second == username) {
            user = pair.first;
            break;
        }
    }
    if (user == INVALID_SOCKET) {
        QString msg = "User was not found, make sure you spelt their username right.";
        if (msgSender != INVALID_SOCKET)
            SendMessageToClient(msgSender, msg.toStdString().c_str(), msg.size());
        else
            emit DataReceived(msg);
        return;
    }
    QString msg = "From " + clientUsernames[msgSender] + " - " + message;
    SendMessageToClient(user, msg.toStdString().c_str(), msg.size());
}

void HostUser::LogCmd(const QString &cmd, const QString &username) { Logger::GetInstance().Log(Logger::LogType::Command, cmd, username); }

void HostUser::LogMsg(const QString &msg, const QString &username) { Logger::GetInstance().Log(Logger::LogType::Message, msg, username); }

void HostUser::GetLog(SOCKET clientSocket) {
    QString log = "Start of Chat History\n\n" + Logger::GetInstance().GetLog() + "\nEnd of Chat History\n\n";
    const size_t maxChunkSize = 255; 
    std::vector<QString> chunks;
    QString currentChunk; 
    QStringList lines = log.split("\n"); 
    size_t currentChunkSize = 0; 
    for (const QString& line : lines) {
        if (currentChunkSize + line.size() + 1 > maxChunkSize) { 
            chunks.push_back(currentChunk); 
            currentChunk.clear(); 
            currentChunkSize = 0;
        }
        if (!currentChunk.isEmpty()) {
            currentChunk += "\n";
            currentChunkSize++;
        }
        currentChunk += line;
        currentChunkSize += line.size();
    }
    if (!currentChunk.isEmpty())
        chunks.push_back(currentChunk);
    for (const auto& chunk : chunks) {
        if (clientSocket == INVALID_SOCKET)
            emit DataReceived(chunk); 
        else {
            QByteArray byteArray = chunk.toUtf8(); 
            SendMessageToClient(clientSocket, byteArray.data(), byteArray.size());
        }
    }
}
