#include "reglog.h"
#include "./ui_reglog.h"
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QThread>
#include <QRegularExpression>

RegLog::RegLog(QWidget *parent, const QString &regLog, ClientUser *client, HostUser *host) : QWidget(parent), ui(new Ui::RegLog), regOrLog(regLog), clientUser(client), hostUser(host)  {
    ui->setupUi(this);
    setWindowTitle(regOrLog);
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    ui->regPageButton->setVisible(regOrLog == "Register");
    ui->logPageButton->setVisible(regOrLog == "Login");
    connect(ui->regPageButton, &QPushButton::clicked, this, &RegLog::RegisterButton);
    connect(ui->logPageButton, &QPushButton::clicked, this, &RegLog::LoginButton);
    connect(ui->username, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->username, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
    connect(ui->password, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->password, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });

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

void RegLog::ErrorCheckBeforeSend(bool enterUsed) {
    ui->regLogWarnings->clear();
    if (ui->username->text().isEmpty()) {
        ui->regLogWarnings->setText("Enter a username");
        return;
    }
    if (ui->password->text().isEmpty()) {
        ui->regLogWarnings->setText("Enter a password");
        return;
    }
    if (regOrLog == "Register") {
        if (ui->username->text().length() > 36) {
            ui->regLogWarnings->setText("Username must be smaller than 36 characters");
            return;
        }
        if (ui->password->text().length() < 8) {
            ui->regLogWarnings->setText("Password must be 8 characters or longer");
            return;
        }
        if (!ui->password->text().contains(QRegularExpression("[0123456789]"))) {
            ui->regLogWarnings->setText("Password must have 1 number");
            return;
        }
        if (!ui->password->text().contains(QRegularExpression("[!@#$%^&*()_+=<>?]"))) {
            ui->regLogWarnings->setText("Password must have 1 special character");
            return;
        }
    }
    if (enterUsed)
        (regOrLog == "Register") ? RegisterButton() : LoginButton();
}
