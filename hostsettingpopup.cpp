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
}

HostSettingPopup::~HostSettingPopup() { delete ui; }

void HostSettingPopup::ConfirmButtonClicked() {
    emit HostSettings(ui->port->text().toUInt(), ui->chatCapacity->text().toUInt(), ui->commandChar->text()[0]);
    close();
}
