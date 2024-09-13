#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include <QObject>

class SocketManager : public QObject
{
    Q_OBJECT

public:
    SocketManager();
    static void CloseApp();
};

#endif // SOCKETMANAGER_H
