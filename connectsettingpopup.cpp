#include "connectsettingpopup.h"
#include "ui_connectsettingpopup.h"

ConnectSettingPopup::ConnectSettingPopup(QWidget *parent) : QWidget(parent), ui(new Ui::ConnectSettingPopup) {
    ui->setupUi(this);
    setWindowTitle("Connect Settings");
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setWindowModality(Qt::ApplicationModal);
    connect(ui->confirmButton, &QPushButton::clicked, this, &ConnectSettingPopup::ConfirmButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, [this]() { close(); });
    connect(ui->ip, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->ip, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
    connect(ui->port, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->port, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
}

ConnectSettingPopup::~ConnectSettingPopup() { delete ui; }

void ConnectSettingPopup::ConfirmButtonClicked() {
    emit ConnectSettings(ui->ip->text(), ui->port->text().toUInt());
    close();
}

void ConnectSettingPopup::ErrorCheckBeforeSend(bool enterUsed) {
    ui->ipWarning->clear();
    ui->portWarning->clear();
    if (ui->ip->text().isEmpty()) {
        ui->ipWarning->setText("Please enter the IP address");
        return;
    }
    if (ui->port->text().isEmpty()) {
        ui->portWarning->setText("Please set a port number");
        return;
    }
    if (ui->port->text().toInt() < 10000) {
        ui->portWarning->setText("Please set a port number higher than 10,000");
        return;
    }
    if (enterUsed)
        ConfirmButtonClicked();
}
