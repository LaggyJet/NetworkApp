#ifndef REGLOG_H
#define REGLOG_H

#include <QWidget>
#include "hostuser.h"
#include "clientuser.h"

namespace Ui {
    class RegLog;
}


class RegLog : public QWidget {
    Q_OBJECT

    public:
        RegLog(QWidget *parent = nullptr, const QString &regLog = "NA", ClientUser *client = nullptr, HostUser *host = nullptr);
        ~RegLog();

    private slots:
        void RegisterButton();
        void LoginButton();

    private:
        Ui::RegLog *ui;
        QString regOrLog;
        HostUser *hostUser;
        ClientUser *clientUser;
};
#endif // REGLOG_H
