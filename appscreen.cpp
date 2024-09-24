#include "appscreen.h"
#include "ui_appscreen.h"
#include <QResizeEvent>
#include <QMessageBox>
#include <QFont>
#include <QFile>
#include "connectjoinpopup.h"
#include "hostsettingpopup.h"
#include "connectsettingpopup.h"
#include "reglog.h"
#include "logger.h"

AppScreen::AppScreen(QWidget *parent): QMainWindow(parent), ui(new Ui::AppScreen), isDarkMode(true) {
    ui->setupUi(this);
    LoadStyleSheet(":/styles/darkmode.qss");
    ConnectJoinPopup *popup = new ConnectJoinPopup(this);
    connect(popup, &ConnectJoinPopup::ChoiceMade, this, &AppScreen::HandleConJoinChoice);
    popup->show();
    connect(ui->connectAction, &QAction::triggered, this, [this]() { HandleConJoinChoice("Connect"); });
    connect(ui->hostAction, &QAction::triggered, this, [this]() { HandleConJoinChoice("Host"); });
    connect(ui->disconnectAction, &QAction::triggered, this, [this]() { DisconnectActionTriggered(false); });
    connect(ui->toggleDarkModeAction, &QAction::triggered,  this, &AppScreen::ToggleBackgroundColor);
    connect(ui->exitAction, &QAction::triggered, this, &AppScreen::ExitActionTriggered);
    connect(ui->messageBox, &QLineEdit::returnPressed, this, &AppScreen::MessageBoxEnter);
    connect(ui->sendMessageButton, &QPushButton::clicked, this, &AppScreen::SendMessageClicked);
}

void AppScreen::closeEvent(QCloseEvent *event) { ExitActionTriggered(); }

AppScreen::~AppScreen() { delete ui; }

void AppScreen::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    const int baseButtonWidth = 120, baseButtonHeight = 40;
    double scaleFactor = qMin(static_cast<double>(size().width()) / 1707, static_cast<double>(size().height()) / 1170);
    double buttonScaleFactor = qMin(scaleFactor, 0.5);
    int buttonWidth = qMax(static_cast<int>(baseButtonWidth * buttonScaleFactor), baseButtonWidth);
    int buttonHeight = qMax(static_cast<int>(baseButtonHeight * buttonScaleFactor), baseButtonHeight);
    ui->sendMessageButton->setFixedSize(buttonWidth, buttonHeight);
    double fontScaleFactor = qMin(scaleFactor * 0.5, 1.0);
    QFont buttonFont = ui->sendMessageButton->font();
    int baseFontSize = 10;
    int newFontSize = static_cast<int>(baseFontSize * fontScaleFactor);
    buttonFont.setPointSize(newFontSize);
    ui->sendMessageButton->setFont(buttonFont);
    if (newFontSize < 8) {
        buttonFont.setPointSize(8);
        ui->sendMessageButton->setFont(buttonFont);
    }
}

void AppScreen::DisconnectActionTriggered(bool forceClose) {
    if (!clientUser && !hostUser && !forceClose) {
        QMessageBox::information(this, "Disconnect", "You can only disconnect once connected");
        return;
    }
    bool wasHost = (hostUser != nullptr);
    if (clientUser) {
        clientUser->Stop();
        if (clientThread && clientThread->joinable())
            clientThread->join();
        delete clientThread;
        clientThread = nullptr;
        delete clientUser;
        clientUser = nullptr;
    }
    if (hostUser) {
        hostUser->Stop();
        if (hostThread && hostThread->joinable())
            hostThread->join();
        delete hostThread;
        hostThread = nullptr;
        delete hostUser;
        hostUser = nullptr;
    }
    ui->messagesField->clear();
    for (int i = ui->usersTable->rowCount() - 1; i >= 0; --i)
        ui->usersTable->removeRow(i);
    if (!forceClose)
        QMessageBox::information(this, "Disconnection", wasHost ? "You have shutdown the server" : "You have disconnected from the current host");
}

void AppScreen::ExitActionTriggered() {
    DisconnectActionTriggered(true);
    close();
}

void AppScreen::HandleConJoinChoice(const QString &choice) {
    if (clientUser || hostUser)
        QMessageBox::information(this, "Warning", "You are currently connected to a server. Please disconnect and try again");
    else if (choice == "Host") {
        HostSettingPopup *hostSettings = new HostSettingPopup(this);
        connect(hostSettings, &HostSettingPopup::HostSettings, this, &AppScreen::HandleHostSettings);
        hostSettings->show();
    }
    else if (choice == "Connect") {
        ConnectSettingPopup *connectSettings = new ConnectSettingPopup(this);
        connect(connectSettings, &ConnectSettingPopup::ConnectSettings, this, &AppScreen::HandleConnectSettings);
        connectSettings->show();
    }
}

void AppScreen::HandleConnectSettings(const QString &ip, const uint16_t &port) {
    if (clientUser) {
        delete clientUser;
        clientUser = nullptr;
    }
    if (clientThread && clientThread->joinable()) {
        clientThread->join();
        delete clientThread;
        clientThread = nullptr;
    }
    clientUser = new ClientUser(ip, port);
    clientThread = new std::thread([this]() { clientUser->Start(); });
    connect(clientUser, &ClientUser::DataReceived, this, &AppScreen::DataReceived, Qt::QueuedConnection);
    connect(clientUser, &ClientUser::ErrorOccurred, this, &AppScreen::ErrorOccurred);
}

void AppScreen::HandleHostSettings(const uint16_t &port, const uint16_t &chatCap, const QChar &commandChar) {
    cmdChar = commandChar;
    if (hostUser) {
        delete hostUser;
        hostUser = nullptr;
    }
    if (hostThread && hostThread->joinable()) {
        hostThread->join();
        delete hostThread;
        hostThread = nullptr;
    }
    hostUser = new HostUser(port, chatCap, commandChar);
    hostThread = new std::thread([this]() { hostUser->Start(); });
    connect(hostUser, &HostUser::DataReceived, this, &AppScreen::DataReceived);
    connect(hostUser, &HostUser::ErrorOccurred, this, &AppScreen::ErrorOccurred);
}

void AppScreen::DataReceived(const QString &data) {
    if (data == "clsCon") {
        DisconnectActionTriggered(true);
        return;
    }
    if (data.startsWith("loggedUser-")) {
        QString user = data;
        username = user.remove("loggedUser-");
        return;
    }
    if (data.startsWith("TabInit-")) {
        QStringList users = data.mid(QString("TabInit-").length()).split(';', Qt::SkipEmptyParts);
        ui->usersTable->setRowCount(0);
        for (const QString &userEntry : users) {
            QStringList userDetails = userEntry.split('\\');
            if (userDetails.size() == 2) {
                QString username = userDetails[0];
                QString role = userDetails[1] == "host" ? "Host" : "Client";
                int curRow = ui->usersTable->rowCount();
                ui->usersTable->insertRow(curRow);
                QTableWidgetItem *usernameItem = new QTableWidgetItem(username);
                usernameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->usersTable->setItem(curRow, 0, usernameItem);
                QTableWidgetItem *statusItem = new QTableWidgetItem(role);
                statusItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                ui->usersTable->setItem(curRow, 1, statusItem);
            }
        }
        ui->usersTable->sortItems(0, Qt::AscendingOrder);
        return;
    }
    else if (data.startsWith("TabAdd-")) {
        QStringList userDetails = data.mid(QString("TabAdd-").length()).split('\\');
        if (userDetails.size() != 2)
            return;
        QString username = userDetails[0];
        QString role = userDetails[1];
        bool userExists = false;
        for (int row = 0; row < ui->usersTable->rowCount(); row++) {
            QTableWidgetItem *item = ui->usersTable->item(row, 0);
            if (item && item->text() == username) {
                userExists = true;
                break;
            }
        }
        if (!userExists) {
            int curRow = ui->usersTable->rowCount();
            ui->usersTable->insertRow(curRow);
            QTableWidgetItem *usernameItem = new QTableWidgetItem(username);
            usernameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            ui->usersTable->setItem(curRow, 0, usernameItem);
            QTableWidgetItem *statusItem = new QTableWidgetItem(role);
            statusItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            ui->usersTable->setItem(curRow, 1, statusItem);
        }
        ui->usersTable->sortItems(0, Qt::AscendingOrder);
        return;
    }
    else if (data.startsWith("TabRem-")) {
        QString username = data.mid(QString("TabRem-").length()).split('\\')[0];
        for (int row = 0; row < ui->usersTable->rowCount(); row++) {
            QTableWidgetItem *item = ui->usersTable->item(row, 0);
            if (item && item->text() == username) {
                ui->usersTable->removeRow(row);
                break;
            }
        }
        return;
    }
    ui->messagesField->appendPlainText(data);
    if (clientUser) {
        const QString expectedMsg = "Welcome to the server!\nThe command character is: ";
        if (data.startsWith(expectedMsg))
            cmdChar = data.at(data.length() - 1);
    }
}

void AppScreen::ErrorOccurred(const QString &error) { QMessageBox::critical(this, "Error", "Error: " + error); }

void AppScreen::MessageBoxEnter() { SendMessage(); }

void AppScreen::SendMessageClicked() { SendMessage(); }

void AppScreen::SendMessage() {
    QString message = ui->messageBox->text();
    if (message.isEmpty()) {
        QMessageBox::warning(this, "Empty Message", "You cannot send a blank message");
        return;
    }
    if (message.length() > 255) {
        QMessageBox::warning(this, "Too Long Message", "Your message is too long");
        return;
    }
    if ((message.at(0) == cmdChar || message.at(0) == '~') && message.length() != 1) {
        HandleCommand(message);
        ui->messageBox->clear();
        return;
    }
    ui->messagesField->appendPlainText("You - " + message);
    if (hostUser)
        message = "Host - " + ui->messageBox->text();
    QByteArray bytes = message.toUtf8();
    if (clientUser) {
        clientUser->SendData(bytes.data(), bytes.size());
    }
    if (hostUser)
        hostUser->SendGlobalMessage(bytes.data(), bytes.size());
    ui->messageBox->clear();
    if (clientUser) {
        QString msg = "logMsg-" + message + "-" + username;
        clientUser->SendData(msg.toStdString().c_str(), msg.size());
    }
    if (hostUser)
        hostUser->LogMsg(message.mid(QString("Host - ").length()), username);
}

void AppScreen::HandleCommand(const QString &cmdMessage) {
    QStringList parts = cmdMessage.split(' ', Qt::SkipEmptyParts);
    QString cmd = parts[0].toLower();
    cmd = cmd.remove(0, 1);
    if (cmd == "help") {
        QString cmdList =   "Commands for this server:\n" +
                            QString(cmdChar) + "help - shows the list of commands\n" +
                            QString(cmdChar) + "register (reg) - pulls up the register page to make an account\n" +
                            QString(cmdChar) + "login (log) - allows you to login to a registered account on this server\n" +
                            QString(cmdChar) + "logout - disconnects you from the server\n" +
                            QString(cmdChar) + "getlist (glt) - sends a list of all users connected in the chat\n" +
                            QString(cmdChar) + "getlog (glg) - sends a lsit of all the public messages sent\n" +
                            QString(cmdChar) + "send (dm) {user} {message} - sends a private message to the user typed\n";
        DataReceived(cmdList);
    }
    else if (cmd == "register" || cmd == "reg" || cmd == "login" || cmd == "log") {
        QString action = (cmd == "register" || cmd == "reg") ? "Register" : "Login";
        RegLog *regLog;
        if (clientUser)
            regLog = new RegLog(this, action, clientUser, nullptr);
        else
            regLog = new RegLog(this, action, nullptr, hostUser);
        regLog->show();
    }
    else if (cmd == "logout")
        DisconnectActionTriggered(false);
    else if (cmd == "getlist" || cmd == "glt") {
        QString users = "Current Users signed in:\n";
        for (int row = 0; row < ui->usersTable->rowCount(); row++)
            users += ui->usersTable->item(row, 0)->text() + "\n";
        DataReceived(users);
    }
    else if (cmd == "getlog" || cmd == "glg") {
        if (clientUser) {
            const char *msg = "getLog";
            clientUser->SendData(msg, strlen(msg));
        }
        else if (hostUser)
            hostUser->GetLog(INVALID_SOCKET);
    }
    else if (cmd == "send" || cmd == "dm") {
        int start = 2, end = parts.size() - 1;            
        if (start < parts.size()) {
            QString combined = parts.mid(start, end - start + 1).join(" ");
            parts[start] = combined;
            for (int i = end; i > start; --i)
                parts.removeAt(i);
        }
        if (clientUser) {
            QString message = "dmUser " + parts[1] + "\\" + parts[2];
            QByteArray bytes = message.toUtf8();
            clientUser->SendData(bytes.data(), bytes.size());
        }
        else if (hostUser) {
            hostUser->SendDM(parts[1], parts[2], INVALID_SOCKET);
        }
    }
    else {
        QMessageBox::warning(this, "Invalid command", cmd + " is not a valid command");
        return;
    }
    if (clientUser) {
        QString msg = "logCmd-" + cmdMessage + "-" + username;
        clientUser->SendData(msg.toStdString().c_str(), msg.size());
    }
    if (hostUser)
        hostUser->LogCmd(cmdMessage, username);
}

void AppScreen::ToggleBackgroundColor() {
    isDarkMode = !isDarkMode;
    LoadStyleSheet(isDarkMode ? ":/styles/darkmode.qss" : ":/styles/lightmode.qss");
    resizeEvent(nullptr);
}

void AppScreen::LoadStyleSheet(const QString &fileName) {
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString style = file.readAll();
        qApp->setStyleSheet(style);
        file.close();
    }
}
