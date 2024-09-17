#include "hostsettingpopup.h"
#include "ui_hostsettingpopup.h"

HostSettingPopup::HostSettingPopup(QWidget *parent) : QWidget(parent), ui(new Ui::HostSettingPopup) {
    ui->setupUi(this);
    setWindowTitle("Host Settings");
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setWindowModality(Qt::ApplicationModal);
    connect(ui->confirmButton, &QPushButton::clicked, this, &HostSettingPopup::ConfirmButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, [this]() { close();  });
    connect(ui->port, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->port, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
    connect(ui->chatCapacity, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->chatCapacity, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
    connect(ui->commandChar, &QLineEdit::returnPressed, this, [this]() { ErrorCheckBeforeSend(true); });
    connect(ui->commandChar, &QLineEdit::editingFinished, this, [this]() { ErrorCheckBeforeSend(false); });
}

HostSettingPopup::~HostSettingPopup() { delete ui; }

void HostSettingPopup::ConfirmButtonClicked() {
    emit HostSettings(ui->port->text().toUInt(), ui->chatCapacity->text().toUInt(), ui->commandChar->text()[0]);
    close();
}

void HostSettingPopup::ErrorCheckBeforeSend(bool enterUsed) {
    ui->portWarning->clear();
    ui->chatCapacityWarning->clear();
    ui->commandCharWarning->clear();
    if (ui->port->text().isEmpty()) {
        ui->portWarning->setText("Please set a port number");
        return;
    }
    if (ui->port->text().toInt() < 10000) {
        ui->portWarning->setText("Please set a port number higher than 10,000");
        return;
    }
    if (ui->chatCapacity->text().isEmpty()) {
        ui->chatCapacityWarning->setText("Please set a chat capacity");
        return;
    }
    if (ui->chatCapacity->text().toInt() < 1) {
        ui->chatCapacityWarning->setText("Please set a chat capacity higher than 0");
        return;
    }
    if (ui->commandChar->text().isEmpty()) {
        ui->commandCharWarning->setText("Please set a command character");
        return;
    }
    if (ui->commandChar->text().length() != 1) {
        ui->commandCharWarning->setText("Make sure to only enter 1 letter for the command character");
        return;
    }
    if (enterUsed)
        ConfirmButtonClicked();
}
