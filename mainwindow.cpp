#include "mainwindow.h"
#include <cstring>
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

void MainWindow::handleReadyRead(){
    receiveBuffer.append(clientSocket->readAll());

    if (receiveBuffer.size()< static_cast<int>(sizeof(quint32))){
        return;
    }

    quint32 imageSize = 0;
    std::memcpy(&imageSize, receiveBuffer.constData(), sizeof(imageSize));

    const int headerSize = sizeof(imageSize);

    if (imageSize > 20*1024*1024) {
        statusBar()->showMessage("Ошибка: размер изображения слишком большой");
        receiveBuffer.clear();
        return;
    }

    if (receiveBuffer.size() < headerSize + static_cast<int>(imageSize)) {
        return;
    }
    const QByteArray imageData = receiveBuffer.mid(headerSize, imageSize);
    receiveBuffer.remove(0, headerSize + imageSize);

    QPixmap pixmap;
    if (!pixmap.loadFromData(imageData, "PNG")) {
        statusBar()->showMessage("Ошибка: не удалось загрузить PNG");
        return;
    }

    ui->frameLabel->setPixmap(
        pixmap.scaled(
            ui->frameLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        )
    );

    statusBar() -> showMessage(QString("Кадр получен: %1 байт").arg(imageSize));

}