#pragma once
#include <QByteArray>
#include <QMainWindow>
#include <QPixmap>
#include <QElapsedTimer>

class QTcpServer;
class QTcpSocket;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   private slots:
    void handleNewConnection();
    void handleReadyRead();

   protected:
    void resizeEvent(QResizeEvent* event) override;

   private:
    Ui::MainWindow* ui;
    QTcpServer* server;
    QPixmap curpixmap;
    QTcpSocket* clientSocket;
    QByteArray receiveBuffer;
    QElapsedTimer fpstimer;
    int fps = 0;
    quint64 receivedbytes = 0;
};
