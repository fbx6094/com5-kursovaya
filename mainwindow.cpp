#include "mainwindow.h"

#include <QHostAddress>
#include <QPixmap>
#include <QTcpServer>
#include <QTcpSocket>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      server(new QTcpServer(this)),
      clientSocket(nullptr) {
    ui->setupUi(this);

    ui->frameLabel->setText("Ожидание подключения...");
    ui->frameLabel->setAlignment(Qt::AlignCenter);
    ui->frameLabel->setScaledContents(false);

    connect(server, &QTcpServer::newConnection, this,
            &MainWindow::handleNewConnection);

    if (!server->listen(QHostAddress::AnyIPv4, 5000)) {
        ui->frameLabel->setText("Ошибка запуска сервера:\n" +
                                server->errorString());
        return;
    }

    statusBar()->showMessage("Сервер запущен на порту 5000");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::handleNewConnection() {
    if (clientSocket != nullptr) {
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
    }

    clientSocket = server->nextPendingConnection();

    connect(clientSocket, &QTcpSocket::readyRead, this,
            &MainWindow::handleReadyRead);

    connect(clientSocket, &QTcpSocket::disconnected, this, [this]() {
        clientSocket->deleteLater();
        clientSocket = nullptr;
        statusBar()->showMessage("Клиент отключён");
    });

    statusBar()->showMessage("Клиент подключён: " +
                             clientSocket->peerAddress().toString());
}

void MainWindow::handleReadyRead() {
    const QByteArray imageData = clientSocket->readAll();

    QPixmap pixmap;
    if (!pixmap.loadFromData(imageData)) {
        const QString text = QString::fromUtf8(imageData);

        ui->frameLabel->setPixmap(QPixmap());
        ui->frameLabel->setText("Получен текст:\n" + text);
        ui->frameLabel->setAlignment(Qt::AlignCenter);

        statusBar()->showMessage("Получены текстовые данные");
        return;
    }

    ui->frameLabel->setPixmap(pixmap.scaled(
        ui->frameLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}