#include "socketmanager.h"
#include "QCoreApplication"

SocketManager::SocketManager() {}

void SocketManager::CloseApp() {
    //Handle Socket terminations

    QCoreApplication::quit();
}
