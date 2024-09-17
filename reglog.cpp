#include "reglog.h"
#include "./ui_reglog.h"
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QThread>


RegLog::RegLog(QWidget *parent, const QString &regLog, ClientUser *client, HostUser *host) : QWidget(parent), ui(new Ui::RegLog), regOrLog(regLog), clientUser(client), hostUser(host)  {
    ui->setupUi(this);
    setWindowTitle(regOrLog);
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);

    ui->regPageButton->setVisible(regOrLog == "Register");
    ui->logPageButton->setVisible(regOrLog == "Login");

    connect(ui->regPageButton, &QPushButton::clicked, this, &RegLog::RegisterButton);
    connect(ui->logPageButton, &QPushButton::clicked, this, &RegLog::LoginButton);
}

RegLog::~RegLog() { delete ui; }


void RegLog::RegisterButton() {
    QString regInfo = "regUser " + ui->username->text() + " " + ui->password->text();
    if (clientUser)
        clientUser->SendData(regInfo.toStdString().c_str(), regInfo.size());
    else if (hostUser)
        hostUser->RegisterUser(INVALID_SOCKET, ui->username->text(), ui->password->text());
    close();
}

void RegLog::LoginButton() {
    QString logInfo = "logUser " + ui->username->text() + " " + ui->password->text();
    if (clientUser)
        clientUser->SendData(logInfo.toStdString().c_str(), logInfo.size());
    else if (hostUser)
        hostUser->LoginUser(INVALID_SOCKET, ui->username->text(), ui->password->text());
    close();
}
