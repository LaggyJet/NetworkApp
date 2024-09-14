#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), hostUser(nullptr), clientUser(nullptr), isDarkMode(true)  {
    ui->setupUi(this);
    setWindowTitle("Network App");

    connect(ui->mainRegisterButton, &QPushButton::clicked, this, &MainWindow::MainPageRegister);
    connect(ui->mainLoginButton, &QPushButton::clicked, this, &MainWindow::MainPageLogin);
    connect(ui->exitButton, &QPushButton::clicked, []() { QCoreApplication::quit(); });
    connect(ui->backButton, &QPushButton::clicked, this, [this]() { ui->stackedWidget->setCurrentWidget(ui->mainPage); });
    connect(ui->toggleBackgroundColorButton, &QPushButton::clicked, this, &MainWindow::ToggleBackgroundColor);
    connect(ui->regPageButton, &QPushButton::clicked, this, &MainWindow::RegisterPageButton);
    connect(ui->logPageButton, &QPushButton::clicked, this, &MainWindow::LoginPageButton);

    LoadStyleSheet(":/styles/darkmode.qss");
}

MainWindow::~MainWindow() {
    delete ui;
    if (clientUser) 
        delete clientUser;
    if (hostUser) 
        delete hostUser;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    const int baseButtonWidth = 120, baseButtonHeight = 40;

    double scaleFactor = qMin(static_cast<double>(size().width()) / 250, static_cast<double>(size().height()) / 250);

    int buttonWidth = qMax(static_cast<int>(baseButtonWidth * scaleFactor), baseButtonWidth);
    int buttonHeight = qMax(static_cast<int>(baseButtonHeight * scaleFactor), baseButtonHeight);

    ui->mainRegisterButton->setFixedSize(buttonWidth, buttonHeight);
    ui->mainLoginButton->setFixedSize(buttonWidth, buttonHeight);
    ui->exitButton->setFixedSize(buttonWidth, buttonHeight);
    ui->regPageButton->setFixedSize(buttonWidth, buttonHeight);
    ui->logPageButton->setFixedSize(buttonWidth, buttonHeight);
    ui->toggleBackgroundColorButton->setFixedSize(buttonWidth / 2, buttonHeight / 2);
    ui->backButton->setFixedSize(buttonWidth / 2, buttonHeight / 2);

    ui->username->setFixedSize(buttonWidth, buttonHeight/2);
    ui->password->setFixedSize(buttonWidth, buttonHeight/2);

    QFont buttonFont = ui->mainRegisterButton->font();
    int newFontSize = static_cast<int>(10 * scaleFactor);
    buttonFont.setPointSize(newFontSize);
    ui->mainRegisterButton->setFont(buttonFont);
    ui->mainLoginButton->setFont(buttonFont);
    ui->exitButton->setFont(buttonFont);
    ui->regPageButton->setFont(buttonFont);
    ui->logPageButton->setFont(buttonFont);
    buttonFont.setPointSize(newFontSize / 1.5);
    ui->toggleBackgroundColorButton->setFont(buttonFont);
    ui->backButton->setFont(buttonFont);

    QFont lineEditFont = ui->username->font();
    lineEditFont.setPointSize(newFontSize * 0.8);
    ui->username->setFont(lineEditFont);
    ui->password->setFont(lineEditFont);
}

void MainWindow::ToggleBackgroundColor() {
    isDarkMode = !isDarkMode;
    LoadStyleSheet(isDarkMode ? ":/styles/darkmode.qss" : ":/styles/lightmode.qss");
    ui->toggleBackgroundColorButton->setText(isDarkMode ? "Light Mode" : "Dark Mode");
    resizeEvent(nullptr);
}

void MainWindow::LoadStyleSheet(const QString &fileName) {
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString style = file.readAll();
        qApp->setStyleSheet(style);
        file.close();
    }
}

void MainWindow::MainPageRegister() {
    ui->stackedWidget->setCurrentWidget(ui->regLogPage);
    ui->regPageButton->setVisible(true);
    ui->logPageButton->setVisible(false);
}

void MainWindow::MainPageLogin() {
    ui->stackedWidget->setCurrentWidget(ui->regLogPage);
    ui->regPageButton->setVisible(false);
    ui->logPageButton->setVisible(true);
}

void MainWindow::RegisterPageButton() {
    if (!clientUser) {
        clientUser = new ClientUser(this);
        QMessageBox::information(this, "Client", "Client started successfully.");
    }
    else
        QMessageBox::warning(this, "Client", "Client is already running.");
}

void MainWindow::LoginPageButton()
{
    if (!hostUser) {
        hostUser = new HostUser(this, 31337);
        connect(hostUser, &HostUser::NewClientConnected, this, &MainWindow::onNewClientConnected);
        connect(hostUser, &HostUser::ClientDisconnected, this, &MainWindow::onClientDisconnected);
        connect(hostUser, &HostUser::DataReceived, this, &MainWindow::onDataReceived);
        connect(hostUser, &HostUser::ErrorOccurred, this, &MainWindow::onErrorOccurred);
        QMessageBox::information(this, "Host", "Host started successfully.");
    }
    else
        QMessageBox::warning(this, "Host", "Host is already running.");
}

void MainWindow::onNewClientConnected()
{
    QMessageBox::information(this, "Client Connected", "A new client has connected.");
}

void MainWindow::onClientDisconnected()
{
    QMessageBox::information(this, "Client Disconnected", "A client has disconnected.");
}

void MainWindow::onDataReceived(const QString &data)
{
    QMessageBox::information(this, "Data Received", "Received data: " + data);
}

void MainWindow::onErrorOccurred(const QString &error)
{
    QMessageBox::critical(this, "Error", "Error: " + error);
}
