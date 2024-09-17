#ifndef APPSCREEN_H
#define APPSCREEN_H

#include <QMainWindow>
#include <QAction>
#include "hostuser.h"
#include "clientuser.h"

namespace Ui {
    class AppScreen;
}

class AppScreen : public QMainWindow {
    Q_OBJECT

    public:
        explicit AppScreen(QWidget *parent = nullptr);
        ~AppScreen();

    protected:
        void resizeEvent(QResizeEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private slots:
        void ExitActionTriggered();
        void DisconnectActionTriggered(bool forceClose);
        void HandleConJoinChoice(const QString &choice);
        void HandleConnectSettings(const QString &ip, const uint16_t &port);
        void HandleHostSettings(const uint16_t &port, const uint16_t &chatCap, const QChar &commandChar);
        void DataReceived(const QString &data);
        void ErrorOccurred(const QString &error);
        void ToggleBackgroundColor();
        void MessageBoxEnter();
        void SendMessageClicked();

    private:
        Ui::AppScreen *ui;
        bool isDarkMode;
        std::thread *hostThread = nullptr, *clientThread = nullptr;
        HostUser *hostUser = nullptr;
        ClientUser *clientUser = nullptr;
        QChar cmdChar;
        void SendMessage();
        void LoadStyleSheet(const QString &fileName);
        void HandleCommand(const QString &cmdMessage);
};

#endif // APPSCREEN_H
