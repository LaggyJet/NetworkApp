#include "connectjoinpopup.h"
#include "ui_connectjoinpopup.h"

ConnectJoinPopup::ConnectJoinPopup(QWidget *parent) : QWidget(parent), ui(new Ui::ConnectJoinPopup) {
    ui->setupUi(this);
    setWindowTitle("Connect or Host a room");
    setFixedSize(size());
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setWindowModality(Qt::ApplicationModal);

    connect(ui->hostButton, &QPushButton::clicked, this, &ConnectJoinPopup::HostButtonClicked);
    connect(ui->connectButton, &QPushButton::clicked, this, &ConnectJoinPopup::ConnectButtonClicked);
}

ConnectJoinPopup::~ConnectJoinPopup() { delete ui; }

void ConnectJoinPopup::HostButtonClicked() {
    emit ChoiceMade(ui->hostButton->text());
    close();
}

void ConnectJoinPopup::ConnectButtonClicked() {
    emit ChoiceMade(ui->connectButton->text());
    close();
}
