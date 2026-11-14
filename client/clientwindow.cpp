#include "clientwindow.h"

#include <QBuffer>
#include <QComboBox>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTimer>
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
      timer(new QTimer(this)) {
    setWindowTitle("Streaming client");
    resize(400, 250);
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(5000);

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