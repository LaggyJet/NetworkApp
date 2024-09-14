#ifndef HOSTUSER_H
#define HOSTUSER_H

#include <QObject>
#include <winsock2.h>
#include <vector>
#include <QSocketNotifier>

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
        void OnClientConnected();
        void OnClientDataReady(int socketDescriptor);

    private:
        SOCKET listeningSocket;
        std::vector<SOCKET> clients;
        QSocketNotifier *acceptNotifier;
        std::vector<QSocketNotifier*> clientNotifiers;

        void ProcessIncomingData(SOCKET clientSocket);
        void ShutdownSocket(SOCKET &socket);
};

#endif // HOSTUSER_H
