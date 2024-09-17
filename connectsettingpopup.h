#ifndef CONNECTSETTINGPOPUP_H
#define CONNECTSETTINGPOPUP_H

#include <QWidget>

namespace Ui {
    class ConnectSettingPopup;
}

class ConnectSettingPopup : public QWidget {
    Q_OBJECT

    public:
        explicit ConnectSettingPopup(QWidget *parent = nullptr);
        ~ConnectSettingPopup();

    signals:
        void ConnectSettings(const QString &ip, const uint16_t &port);

    private slots:
        void ConfirmButtonClicked();
        void ErrorCheckBeforeSend(bool enterUsed);

    private:
        Ui::ConnectSettingPopup *ui;
};

#endif // CONNECTSETTINGPOPUP_H
