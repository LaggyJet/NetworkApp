#include "hostuser.h"
#include <QTimer>
#include <QMessageBox>
#include <algorithm>

HostUser::HostUser(QObject *parent, uint16_t port) 
    : QObject(parent), listeningSocket(INVALID_SOCKET) {

    // Create a socket
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        emit ErrorOccurred("Socket creation failed: " + QString::number(WSAGetLastError()));
        return;
    }

    // Set the socket to non-blocking mode
    u_long mode = 1;
    ioctlsocket(listeningSocket, FIONBIO, &mode);

    // Bind the socket to the specified port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listeningSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        emit ErrorOccurred("Bind failed: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        return;
    }

    // Start listening for connections
    if (listen(listeningSocket, 15) == SOCKET_ERROR) {
        emit ErrorOccurred("Listen failed: " + QString::number(WSAGetLastError()));
        ShutdownSocket(listeningSocket);
        return;
    }

    // Set up timers to periodically check for connections and data
    connectionCheckTimer.setInterval(100);
    connect(&connectionCheckTimer, &QTimer::timeout, this, &HostUser::CheckForConnections);
    connectionCheckTimer.start();

    dataCheckTimer.setInterval(100);
    connect(&dataCheckTimer, &QTimer::timeout, this, &HostUser::CheckForClientData);
    dataCheckTimer.start();

    emit StatusUpdate("Server listening on port " + QString::number(port));
}

HostUser::~HostUser() {
    // Shut down all client sockets
    for (SOCKET clientSocket : clients) {
        ShutdownSocket(clientSocket);
    }
    // Shut down the listening socket
    ShutdownSocket(listeningSocket);
}

void HostUser::CheckForConnections() {
    // Check if there is a pending connection
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&clientAddress, &clientAddressSize);

    if (clientSocket != INVALID_SOCKET) {
        // Successfully accepted a client
        u_long mode = 1;
        ioctlsocket(clientSocket, FIONBIO, &mode); // Set client socket to non-blocking mode
        clients.push_back(clientSocket);
        
        const char* welcomeMessage = "Welcome to the server!";
        send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);

        emit NewClientConnected();
    }
    else if (WSAGetLastError() != WSAEWOULDBLOCK) {
        emit ErrorOccurred("Accept failed: " + QString::number(WSAGetLastError()));
    }
}

void HostUser::CheckForClientData() {
    // Iterate over the list of client sockets and check for data
    for (auto it = clients.begin(); it != clients.end();) {
        SOCKET clientSocket = *it;
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            // Data was received
            QString data = QString::fromUtf8(buffer, bytesReceived);
            emit DataReceived(data);
            ++it;
        }
        else if (bytesReceived == 0 || WSAGetLastError() != WSAEWOULDBLOCK) {
            // Client disconnected or an error occurred
            emit ClientDisconnected();
            ShutdownSocket(clientSocket);
            it = clients.erase(it);
        }
        else {
            ++it;
        }
    }
}

void HostUser::ProcessIncomingData(SOCKET clientSocket) {
    // This function processes incoming data from a client
    // No longer directly called, but logic is inside CheckForClientData
}

void HostUser::ShutdownSocket(SOCKET &socket) {
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_BOTH);
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}
