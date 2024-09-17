#ifndef HOSTSETTINGPOPUP_H
#define HOSTSETTINGPOPUP_H

#include <QWidget>

namespace Ui {
    class HostSettingPopup;
}

class HostSettingPopup : public QWidget {
    Q_OBJECT

    public:
        explicit HostSettingPopup(QWidget *parent = nullptr);
        ~HostSettingPopup();

    signals:
        void HostSettings(const uint16_t &port, const uint16_t &chatCap, const QChar &commandChar);

    private slots:
        void ConfirmButtonClicked();

    private:
        Ui::HostSettingPopup *ui;
};

#endif // HOSTSETTINGPOPUP_H
