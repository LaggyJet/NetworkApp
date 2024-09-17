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
}

ConnectSettingPopup::~ConnectSettingPopup() { delete ui; }

void ConnectSettingPopup::ConfirmButtonClicked() {
    emit ConnectSettings(ui->ip->text(), ui->port->text().toUInt());
    close();
}
