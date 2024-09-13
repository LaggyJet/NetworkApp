#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "socketmanager.h"
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    connect(ui->exitButton, &QPushButton::clicked, []() {SocketManager::CloseApp(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

