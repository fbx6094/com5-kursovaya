#include "clientwindow.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
      timer(new QTimer(this)) {
    setWindowTitle("Streaming client");
    resize(400, 250);
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(5000);

    auto* formLayout = new QFormLayout;
    formLayout->addRow("IP сервера:", hostEdit);
    formLayout->addRow("Порт:", portSpinBox);

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

void ClientWindow::handleConnected() {
    connectButton->setEnabled(false);
    disconnectButton->setEnabled(true);

    startButton->setEnabled(true);
    stopButton->setEnabled(false);

    statusLabel->setText("Подключён к серверу: " +
                         socket->peerAddress().toString());
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

void ClientWindow::sendFrame(){

}