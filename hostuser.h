#ifndef HOSTUSER_H
#define HOSTUSER_H

#include <QObject>
#include <winsock2.h>
#include <thread>
#include <QHostAddress>
#include <QNetworkInterface>
#include <unordered_map>

class HostUser : public QObject {
    Q_OBJECT

    public:
        explicit HostUser(uint16_t port, uint16_t chatCap, QChar commandChar);
        ~HostUser();

        void Start();
        void Stop();
        void SendGlobalMessage(const char *data, int32_t length);
        void SendDM(const QString &username, const QString &message, SOCKET msgSender);
        void RegisterUser(SOCKET clientSocket, const QString &username, const QString &password);
        void LoginUser(SOCKET clientSocket, const QString &username, const QString &password);
        void LogCmd(const QString &cmd, const QString &username);
        void LogMsg(const QString &msg, const QString &username);
        void GetLog(SOCKET clientSocket);

    signals:
        void ErrorOccurred(const QString &error);
        void DataReceived(const QString &data);

    private:
        void HandleClients();
        void HandleUDP();
        int ReceiveMessageFromClient(SOCKET clientSocket);
        void SendMessageToClient(SOCKET clientSocket, const char *data, int32_t length);
        void ShutdownSocket(SOCKET &socket);
        void SendHostStartInfo();
        void SendToOtherClients(SOCKET exSocket, const char *data, int32_t length);
        void SyncTableInit(SOCKET clientSocket);
        void SyncTableAddRemove(const QString &username, bool isHost, bool removed);

        std::unordered_map<QString, QString> users;
        std::unordered_map<SOCKET, QString> clientUsernames;
        uint16_t port, chatCap;
        QChar commandChar;
        SOCKET listeningSocket, udpSocket;
        std::vector<SOCKET> clients;
        std::thread clientHandlerThread;
        bool running;
        std::chrono::steady_clock::time_point lastUdpBroadcastTime;
};

#endif // HOSTUSER_H
