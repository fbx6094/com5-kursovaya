#pragma once
#include <QMainWindow>
#include <QElapsedTimer>
#include <QSize>

class QLineEdit;
class QPushButton;
class QTcpSocket;
class QTimer;
class QLabel;
class QSpinBox;
class QComboBox;
class QSlider;
class QUdpSocket;

class ClientWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit ClientWindow(QWidget* parent = nullptr);
    ~ClientWindow() override;

   private:
    QElapsedTimer fpstimer;
    int fps;
    QLineEdit* hostEdit;
    QSpinBox* portSpinBox;
    QPushButton* connectButton;
    QPushButton* disconnectButton;
    QPushButton* startButton;
    QPushButton* stopButton;
    QLabel* statusLabel;
    QTcpSocket* socket;
    QComboBox* resolutionComboBox;
    QSlider* qualitySlider;
    QLabel* qualitylabel;
    QTimer* timer;
    QPushButton* findserverbtn;
    QUdpSocket* discoverysocket;
    QComboBox* serverlist;

   private slots:
    void connectToServer();
    void disconnectFromServer();
    void startStreaming();
    void stopStreaming();
    void sendFrame();
    void handleConnected();
    void handleDisconnected();
    void updatequalitylabel(int value);
    void findservers();
    void handlediscovery();
    void selectserver(int index);
};