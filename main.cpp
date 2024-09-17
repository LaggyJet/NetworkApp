#include "appscreen.h"
#include <QApplication>
#include <winsock2.h>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        QMessageBox::critical(nullptr, "Error", "WSAStartup failed");
        return -1;
    }
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/icon.ico"));
    AppScreen w;
    w.show();
    int ret =  a.exec();
    WSACleanup();
    return ret;
}
