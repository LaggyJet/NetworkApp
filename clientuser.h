#ifndef CLIENTUSER_H
#define CLIENTUSER_H

#include <QObject>
#include <QString>
#include <thread>
#include <winsock2.h>
#include <mutex>
#include <ws2tcpip.h>

class ClientUser : public QObject {
    Q_OBJECT

    public:
        explicit ClientUser(const QString &address, int port);
        ~ClientUser();

        void Start();   
        void Stop();   
        void SendData(const char *data, int32_t length);

    signals:
        void DataReceived(const QString &data);   
        void ErrorOccurred(const QString &error); 

    private:
        SOCKET socket;
        sockaddr_in serverAddr;
        std::thread readThread;
        std::mutex mtx;
        bool running;

        void ReadingThread();
        bool ReadData(char *buffer, int32_t size, int32_t &returnedMsgSize);
        void ShutdownSocket();
};

#endif // CLIENTUSER_H
