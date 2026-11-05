#pragma once
#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QTcpSocket;
class QTimer;
class QLabel;
class QSpinBox;

class ClientWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit ClientWindow(QWidget* parent = nullptr);
    ~ClientWindow() override;

   private:
    QLineEdit* hostEdit;
    QSpinBox* portSpinBox;
    QPushButton* connectButton;
    QPushButton* disconnectButton;
    QPushButton* startButton;
    QPushButton* stopButton;
    QLabel* statusLabel;
    QTcpSocket* socket;
    QTimer* timer;

   private slots:
    void connectToServer();
    void disconnectFromServer();
    void startStreaming();
    void stopStreaming();
    void sendFrame();
    void handleConnected();
    void handleDisconnected();
};