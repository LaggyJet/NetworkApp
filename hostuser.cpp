#include "hostuser.h"
#include <QSocketNotifier>

HostUser::HostUser(QObject *parent, uint16_t port) : QObject(parent), listeningSocket(INVALID_SOCKET) {
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket creation failed: " + QString::number(WSAGetLastError()));
        return;
    }

    sockaddr_in serverAddress{AF_INET, htons(port), INADDR_ANY};
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

    acceptNotifier = new QSocketNotifier(listeningSocket, QSocketNotifier::Read, this);
    connect(acceptNotifier, &QSocketNotifier::activated, this, &HostUser::OnClientConnected);

    emit StatusUpdate("Server listening on port " + QString::number(port));
}

HostUser::~HostUser() {
    for (QSocketNotifier* notifier : clientNotifiers)
        delete notifier;
    for (SOCKET clientSocket : clients)
        ShutdownSocket(clientSocket);
    ShutdownSocket(listeningSocket);
    delete acceptNotifier;
}

void HostUser::OnClientConnected() {
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&clientAddress, &clientAddressSize);

    if (clientSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Accept failed: " + QString::number(WSAGetLastError()));
        return;
    }

    clients.push_back(clientSocket);
    const char* welcomeMessage = "Welcome to the server!";
    send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);

    emit NewClientConnected();

    QSocketNotifier* clientNotifier = new QSocketNotifier(clientSocket, QSocketNotifier::Read, this);
    clientNotifiers.push_back(clientNotifier);
    connect(clientNotifier, &QSocketNotifier::activated, this, [this, clientSocket]() { OnClientDataReady(clientSocket); });
}

void HostUser::OnClientDataReady(int clientSocket) { ProcessIncomingData(clientSocket); }

void HostUser::ProcessIncomingData(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0) {
        QString data = QString::fromUtf8(buffer, bytesReceived);
        emit DataReceived(data);
    }
    else if (bytesReceived == 0 || WSAGetLastError() != WSAEWOULDBLOCK) {
        ShutdownSocket(clientSocket);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
        emit ClientDisconnected();
    }
}

void HostUser::ShutdownSocket(SOCKET &socket) {
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_BOTH);
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}
