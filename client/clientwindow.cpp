#include "clientwindow.h"

#include <QAbstractSocket>
#include <QBuffer>
#include <QComboBox>
#include <QFormLayout>
#include <QHostAddress>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkDatagram>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#include <QVBoxLayout>
#include <QWidget>

ClientWindow::ClientWindow(QWidget* parent)
    : QMainWindow(parent),
      hostEdit(new QLineEdit("127.0.0.1", this)),
      portSpinBox(new QSpinBox(this)),
      connectButton(new QPushButton("Подключиться", this)),
      disconnectButton(new QPushButton("Отключиться", this)),
      startButton(new QPushButton("Начать передачу", this)),
      stopButton(new QPushButton("Остановить передачу", this)),
      statusLabel(new QLabel("Не подключён", this)),
      socket(new QTcpSocket(this)),
      resolutionComboBox(new QComboBox(this)),
      qualitySlider(new QSlider(Qt::Horizontal, this)),
      qualitylabel(new QLabel(this)),
      findserverbtn(new QPushButton("Найти серверы", this)),
      discoverysocket(new QUdpSocket(this)),
      serverlist(new QComboBox(this)),
      timer(new QTimer(this)) {
    setWindowTitle("Streaming client");
    resize(400, 250);
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(5000);

    discoverysocket->bind(
        QHostAddress::AnyIPv4, 0,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    resolutionComboBox->addItems(
        {"640x480", "800x600", "1024x768", "1280x720", "1920x1080"});
    resolutionComboBox->setEditable(true);
    resolutionComboBox->setCurrentText("1280x720");

    qualitySlider->setRange(1, 100);
    qualitySlider->setValue(75);

    updatequalitylabel(qualitySlider->value());

    auto* formLayout = new QFormLayout;
    formLayout->addRow("IP сервера:", hostEdit);
    formLayout->addRow("Порт:", portSpinBox);
    formLayout->addRow("Разрешение = ", resolutionComboBox);
    formLayout->addRow("Качество JPEG = ", qualitySlider);
    formLayout->addRow("", qualitylabel);
    formLayout->addRow("Серверы:", serverlist);
    formLayout->addRow("", findserverbtn);

    auto* layout = new QVBoxLayout;
    layout->addLayout(formLayout);
    layout->addWidget(connectButton);
    layout->addWidget(disconnectButton);
    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(statusLabel);

    auto* centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    disconnectButton->setEnabled(false);
    startButton->setEnabled(false);
    stopButton->setEnabled(false);

    timer->setInterval(17);
    fpstimer.start();

    connect(connectButton, &QPushButton::clicked, this,
            &ClientWindow::connectToServer);
    connect(disconnectButton, &QPushButton::clicked, this,
            &ClientWindow::disconnectFromServer);
    connect(startButton, &QPushButton::clicked, this,
            &ClientWindow::startStreaming);
    connect(stopButton, &QPushButton::clicked, this,
            &ClientWindow::stopStreaming);
    connect(timer, &QTimer::timeout, this, &ClientWindow::sendFrame);
    connect(socket, &QTcpSocket::connected, this,
            &ClientWindow::handleConnected);
    connect(socket, &QTcpSocket::disconnected, this,
            &ClientWindow::handleDisconnected);
    connect(qualitySlider, &QSlider::valueChanged, this,
            &ClientWindow::updatequalitylabel);
    connect(findserverbtn, &QPushButton::clicked, this,
            &ClientWindow::findservers);
    connect(discoverysocket, &QUdpSocket::readyRead, this,
            &ClientWindow::handlediscovery);
    connect(serverlist, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ClientWindow::selectserver);
}

ClientWindow::~ClientWindow() = default;

void ClientWindow::connectToServer() {
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        return;
    }

    statusLabel->setText("Подключение...");

    socket->connectToHost(hostEdit->text(),
                          static_cast<quint16>(portSpinBox->value()));
}

void ClientWindow::disconnectFromServer() {
    stopStreaming();

    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
    }
}

void ClientWindow::updatequalitylabel(int value) {
    qualitylabel->setText(QString("Качество JPEG = %1").arg(value));
}

void ClientWindow::handleConnected() {
    connectButton->setEnabled(false);
    disconnectButton->setEnabled(true);

    startButton->setEnabled(true);
    stopButton->setEnabled(false);

    statusLabel->setText("Подключён к серверу: " +
                         socket->peerAddress().toString());

    startStreaming();
}

void ClientWindow::handleDisconnected() {
    timer->stop();

    connectButton->setEnabled(true);
    disconnectButton->setEnabled(false);

    startButton->setEnabled(false);
    stopButton->setEnabled(false);

    statusLabel->setText("Отсоединен от сервера");
}

void ClientWindow::startStreaming() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    timer->start();
    startButton->setEnabled(false);
    stopButton->setEnabled(true);

    statusLabel->setText("Стрим запущен");
}

void ClientWindow::stopStreaming() {
    timer->stop();
    startButton->setEnabled(socket->state() == QAbstractSocket::ConnectedState);
    stopButton->setEnabled(false);
    statusLabel->setText("Стрим остановлен");
}

void ClientWindow::sendFrame() {
    QElapsedTimer frametimer;
    frametimer.start();
    if (socket->state() != QAbstractSocket::ConnectedState) {
        stopStreaming();
        return;
    }

    // Один кадр сейчас обычно занимает около 160 КБ.
    // Не даём TCP-очереди накапливать больше примерно двух кадров.
    constexpr qint64 maxQueuedBytes = 350000;
    if (socket->bytesToWrite() > maxQueuedBytes) {
        return;
    }

    QScreen* screen = QGuiApplication::primaryScreen();

    if (screen == nullptr) {
        statusLabel->setText("Экран не найден");
        stopStreaming();
        return;
    }

    const QImage image = screen->grabWindow(0).toImage();

    const QString resolutionText = resolutionComboBox->currentText();
    const QStringList parts = resolutionText.split('x');
    if (parts.size() != 2) {
        statusLabel->setText("Неверное разрешение: " + resolutionText);
        stopStreaming();
        return;
    }
    bool widthok = false;
    bool heightok = false;
    const int width = parts[0].toInt(&widthok);
    const int height = parts[1].toInt(&heightok);
    if (!widthok || !heightok || width <= 0 || height <= 0) {
        statusLabel->setText("Неверное разрешение: ");
        stopStreaming();
        return;
    }

    const QImage scaledImage = image.scaled(width, height, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);

    QByteArray imagedata;
    QBuffer buffer(&imagedata);

    if (!buffer.open(QIODevice::WriteOnly) ||
        !scaledImage.save(&buffer, "JPEG", qualitySlider->value())) {
        statusLabel->setText("err saving image");
        stopStreaming();
        return;
    }

    const quint32 imageSize = imagedata.size();
    QByteArray pkg;
    pkg.append(reinterpret_cast<const char*>(&imageSize), sizeof(imageSize));
    pkg.append(imagedata);
    if (socket->write(pkg) == -1) {
        statusLabel->setText("err sending image" + socket->errorString());
        stopStreaming();
        return;
    }
    fps++;

    const qint64 elapsed = fpstimer.elapsed();
    if (elapsed >= 1000) {
        const double clientFps = fps * 1000.0 / static_cast<double>(elapsed);

        statusLabel->setText(
            QString("Клиент FPS: %1").arg(clientFps, 0, 'f', 1));

        fps = 0;
        fpstimer.restart();
    }
}

void ClientWindow::findservers() {
    serverlist->clear();

    const QByteArray request = "DISCOVER_SCREEN_SERVER";

    if (discoverysocket->writeDatagram(request, QHostAddress::Broadcast, 5001) ==
        -1) {
        statusLabel->setText("Ошибка поиска серверов: " +
                             discoverysocket->errorString());
        return;
    }

    statusLabel->setText("Поиск серверов...");
}

void ClientWindow::handlediscovery() {
    while (discoverysocket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = discoverysocket->receiveDatagram();

        if (datagram.data() != "DISCOVER_SCREEN_SERVER") {
            const QString response = QString::fromUtf8(datagram.data());

            const QStringList parts = response.split('|');
            if (parts.size() == 2) {
                bool portok = false;
                const int port = parts[1].toInt(&portok);

                if (portok) {
                    const QHostAddress senderaddress = datagram.senderAddress();
                    QString host;
                    if (senderaddress.protocol() ==
                        QAbstractSocket::IPv4Protocol) {
                        host = senderaddress.toString();
                    } else if (senderaddress.toIPv4Address() != 0) {
                        host = QHostAddress(senderaddress.toIPv4Address())
                                   .toString();
                    } else {
                        host = senderaddress.toString();
                    }
                    const QString servername =
                        host + ":" + QString::number(port);

                    int index = serverlist->findText(servername);

                    if (index == -1) {
                        serverlist->addItem(servername);
                        index = serverlist->count() - 1;
                    }
                    serverlist->setItemData(index, host, Qt::UserRole);
                    serverlist->setItemData(index, port, Qt::UserRole + 1);
                    selectserver(index);

                    statusLabel->setText("Найден сервер: " + servername);
                }
            }
        }
    }
}

void ClientWindow::selectserver(int index) {
    if (index < 0) {
        return;
    }

    const QString host = serverlist->itemData(index, Qt::UserRole).toString();
    const int port = serverlist->itemData(index, Qt::UserRole + 1).toInt();
    if (host.isEmpty() || port <= 0) {
        return;
    }
    hostEdit->setText(host);
    portSpinBox->setValue(port);

    statusLabel->setText(QString("Выбран сервер: %1:%2").arg(host).arg(port));
}