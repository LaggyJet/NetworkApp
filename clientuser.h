#ifndef CLIENTUSER_H
#define CLIENTUSER_H

#include <winsock2.h>
#include <QWidget>
#include <QList>
#include <QTimer>

class ClientUser : public QWidget {
    Q_OBJECT

    public:
        ClientUser(QWidget* parent = nullptr, const char* address="127.0.0.1", uint16_t port = 31337);
        ~ClientUser();
    
    private slots:
        void ReadData();
        void SendData();
        void SendPeriodicMessage();
    
    private:
        SOCKET clientSocket;
        QList<QByteArray> sendQueue;
        QTimer cooldownTimer, periodicMessageTimer;
        
        void HandleError(const QString &message);
        void ShutdownSocket();
        void HandleDisconnection();
};

#endif // CLIENTUSER_H
