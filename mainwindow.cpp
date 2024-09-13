#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QCoreApplication>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("Network App");

    connect(ui->exitButton, &QPushButton::clicked, []() { QCoreApplication::quit(); });
    connect(ui->toggleBackgroundColorButton, &QPushButton::clicked, this, &MainWindow::ToggleBackgroundColor);

    isDarkMode = true;
    LoadStyleSheet(":/styles/darkmode.qss");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    const int baseButtonWidth = 120, baseButtonHeight = 40;

    double scaleFactor = qMin(static_cast<double>(size().width()) / 250, static_cast<double>(size().height()) / 250);
    int buttonWidth = qMax(static_cast<int>(baseButtonWidth * scaleFactor), baseButtonWidth);
    int buttonHeight = qMax(static_cast<int>(baseButtonHeight * scaleFactor), baseButtonHeight);

    ui->registerButton->setFixedSize(buttonWidth, buttonHeight);
    ui->loginButton->setFixedSize(buttonWidth, buttonHeight);
    ui->exitButton->setFixedSize(buttonWidth, buttonHeight);
    ui->toggleBackgroundColorButton->setFixedSize(buttonWidth / 2, buttonHeight / 2);

    QFont buttonFont = ui->registerButton->font();
    int newFontSize = static_cast<int>(10 * scaleFactor);
    buttonFont.setPointSize(newFontSize);
    ui->registerButton->setFont(buttonFont);
    ui->loginButton->setFont(buttonFont);
    ui->exitButton->setFont(buttonFont);
    buttonFont.setPointSize(newFontSize / 1.5);
    ui->toggleBackgroundColorButton->setFont(buttonFont);
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
