#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "hostuser.h"
#include "clientuser.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    protected:
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void MainPageRegister();
        void MainPageLogin();
        void RegisterPageButton();
        void LoginPageButton();
        void onNewClientConnected();
        void onClientDisconnected();
        void onDataReceived(const QString &data);
        void onErrorOccurred(const QString &error);

    private:
        Ui::MainWindow *ui;
        HostUser *hostUser;
        ClientUser *clientUser;
        bool isDarkMode;
        void LoadStyleSheet(const QString &fileName);
        void ToggleBackgroundColor();
};
#endif // MAINWINDOW_H
