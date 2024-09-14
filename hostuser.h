#ifndef HOSTUSER_H
#define HOSTUSER_H

#include <QObject>
#include <winsock2.h>
#include <vector>
#include <QTimer>

class HostUser : public QObject {
    Q_OBJECT

    public:
        explicit HostUser(QObject *parent = nullptr, uint16_t port = 31337);
        ~HostUser();

    signals:
        void ErrorOccurred(const QString &message);
        void StatusUpdate(const QString &status);
        void NewClientConnected();
        void ClientDisconnected();
        void DataReceived(const QString &data);

    private slots:
        void CheckForConnections();
        void CheckForClientData();

    private:
        SOCKET listeningSocket;
        std::vector<SOCKET> clients;
        QTimer connectionCheckTimer;
        QTimer dataCheckTimer;

        void ProcessIncomingData(SOCKET clientSocket);
        void ShutdownSocket(SOCKET &socket);
};

#endif // HOSTUSER_H
