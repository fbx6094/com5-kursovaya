#include "mainwindow.h"
#include "common/protocol.h"
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QPixmap>
#include <QSizePolicy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVBoxLayout>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      server(new QTcpServer(this)),
      discoverysocket(new QUdpSocket(this)),
      clientSocket(nullptr) {
    ui->setupUi(this);
    ui->fpslabel->setText("fps: 0");

    fpstimer.start();

    ui->frameLabel->setMinimumSize(0, 0);
    ui->frameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    ui->frameLabel->setText("Ожидание подключения...");
    ui->frameLabel->setAlignment(Qt::AlignCenter);
    ui->frameLabel->setScaledContents(false);

    auto* layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(ui->frameLabel);

    connect(server, &QTcpServer::newConnection, this,
            &MainWindow::handleNewConnection);

    connect(discoverysocket, &QUdpSocket::readyRead, this,
            &MainWindow::handlediscovery);

    if (!server->listen(QHostAddress::AnyIPv4, Protocol::tcpPort)) {
        ui->frameLabel->setText("Ошибка запуска сервера:\n" +
                                server->errorString());
        return;
    }
    if (!discoverysocket->bind(
            QHostAddress::AnyIPv4, Protocol::udpPort,
            QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        statusBar()->showMessage("ошибка запуска автопоиска" +
                                 discoverysocket->errorString());
    }

    statusBar()->showMessage("Сервер запущен на порту " + QString::number(Protocol::tcpPort));
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if (!curpixmap.isNull()) {
        ui->frameLabel->setPixmap(curpixmap.scaled(ui->frameLabel->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
    }
}

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
        ui->frameLabel->setText("Ожидание подключения...");
    });

    statusBar()->showMessage("Клиент подключён: " +
                             clientSocket->peerAddress().toString());
}

void MainWindow::handleReadyRead() {
    receiveBuffer.append(clientSocket->readAll());
    const int headerSize = sizeof(Protocol::packetHeader);
    while (true) {
        if (receiveBuffer.size() < headerSize) {
            return;
        }
        Protocol::packetHeader imageSize = 0;
        std::memcpy(&imageSize, receiveBuffer.constData(), sizeof(imageSize));
        if (imageSize == 0 || imageSize > Protocol::maxframeSize) {
            statusBar()->showMessage(
                QString("Ошибка: некорректный размер изображения: %1")
                    .arg(imageSize));
            receiveBuffer.clear();
            return;
        }
        const quint64 pkgsize = static_cast<quint64>(headerSize) + imageSize;

        if (receiveBuffer.size() < pkgsize) {
            return;
        }

        const QByteArray imageData = receiveBuffer.mid(headerSize, imageSize);
        receiveBuffer.remove(0, pkgsize);

        if (!curpixmap.loadFromData(imageData, "JPEG")) {
            statusBar()->showMessage(
                "Ошибка: не удалось загрузить изображение");
            continue;
        }
        receivedbytes += imageSize;

        ui->frameLabel->setPixmap(curpixmap.scaled(ui->frameLabel->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));

        fps++;
        const quint64 elapsed = fpstimer.elapsed();
        const double bitrate = receivedbytes * 8 / (elapsed * 1000.0);
        if (elapsed >= 1000) {
            const double fpsvalue = fps * 1000.0 / static_cast<double>(elapsed);

            ui->fpslabel->setText(QString("FPS: %1 | Bitrate: %2 mbit/s")
                                      .arg(fpsvalue, 0, 'f', 1)
                                      .arg(bitrate, 0, 'f', 1));
            fps = 0;
            fpstimer.restart();
            receivedbytes = 0;
        }
    }
}

void MainWindow::handlediscovery() {
    while (discoverysocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = discoverysocket->receiveDatagram();

        if (datagram.data() != Protocol::discoveryRequest) {
            continue;
        }

        const QByteArray responce = QByteArray(Protocol::discoveryResponce) + "|" + QByteArray::number(Protocol::tcpPort);

        discoverysocket->writeDatagram(responce, datagram.senderAddress(),
                                       datagram.senderPort());
    }
}