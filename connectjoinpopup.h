#ifndef CONNECTJOINPOPUP_H
#define CONNECTJOINPOPUP_H

#include <QWidget>

namespace Ui {
    class ConnectJoinPopup;
}

class ConnectJoinPopup : public QWidget {
        Q_OBJECT

    public:
        explicit ConnectJoinPopup(QWidget *parent = nullptr);
        ~ConnectJoinPopup();

    signals:
        void ChoiceMade(const QString &choice);

    private slots:
        void HostButtonClicked();
        void ConnectButtonClicked();

    private:
        Ui::ConnectJoinPopup *ui;
};

#endif // CONNECTJOINPOPUP_H
