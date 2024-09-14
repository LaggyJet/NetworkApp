#include "clientuser.h"
#include <ws2tcpip.h>
#include <QMessageBox>

ClientUser::ClientUser(QWidget* parent, const char* address, uint16_t port) : QWidget(parent), clientSocket(INVALID_SOCKET) {
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        HandleError("Socket creation failed");
        return;
    }

    sockaddr_in server = {AF_INET, htons(port)};
    if (inet_pton(AF_INET, address, &server.sin_addr) != 1) {
        HandleError("Invalid Address");
        return;
    }

    if (::connect(clientSocket, (sockaddr *)&server, sizeof(server)) != 0) {
        HandleError("Connection failed");
        return;
    }

    u_long mode = 1;
    if (ioctlsocket(clientSocket, FIONBIO, &mode) != NO_ERROR) {
        HandleError("Failed to set non-blocking mode");
        return;
    }

    //TODO: Figure out which timers need to stay or not
    QMessageBox::information(this, "Connection Established", "Successfully connected to the server.");

    periodicMessageTimer.setInterval(5000);
    connect(&periodicMessageTimer, &QTimer::timeout, this, &ClientUser::SendPeriodicMessage);
    periodicMessageTimer.start();

    cooldownTimer.setInterval(100);
    connect(&cooldownTimer, &QTimer::timeout, this, &ClientUser::ReadData);
    cooldownTimer.start();
}

ClientUser::~ClientUser() {
    if (clientSocket != INVALID_SOCKET)
        ShutdownSocket();
}

void ClientUser::HandleError(const QString &message) { QMessageBox::critical(this, "Error", message); }

void ClientUser::ShutdownSocket() {
    if (clientSocket != INVALID_SOCKET) {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}

void ClientUser::ReadData() {
    if (clientSocket == INVALID_SOCKET)
        return;
    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        QMessageBox::information(this, "Data Received", QString::fromUtf8(buffer));
    }
    else {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAEWOULDBLOCK) {} 
        else if (errorCode == WSAECONNRESET) {
            QMessageBox::information(this, "Connection Closed", "Server closed the connection.");
            ShutdownSocket();
        } 
        else
            HandleError("Receive failed: " + QString::number(errorCode));
    }
}

void ClientUser::SendData() {
    if (clientSocket == INVALID_SOCKET)
        return;
    if (!sendQueue.isEmpty()) {
        QByteArray data = sendQueue.takeFirst();
        if (send(clientSocket, data.constData(), data.size(), 0) == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
                sendQueue.prepend(data);
            else
                HandleError("Send failed: " + QString::number(error));
        }
    }
}

void ClientUser::SendPeriodicMessage() {
    if (clientSocket == INVALID_SOCKET)
        return;
    QByteArray data = "Periodic message from client";
    if (data.size() > 0xFFFF) {
        HandleError("Message size too large");
        return;
    }
    if (send(clientSocket, data.constData(), data.size(), 0) == SOCKET_ERROR)
        HandleError("Periodic send failed: " + QString::number(WSAGetLastError()));
}
