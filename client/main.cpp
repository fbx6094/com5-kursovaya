#include <QBuffer>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QTcpSocket>
#include <QScreen>
#include <QPixmap>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 5000);

    if (!socket.waitForConnected(3000)) {
        qDebug() << "Connection failed: " << socket.errorString();
        return 1;
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen == nullptr){
        qCritical() <<"Screen not found";
        return 1;
    }

    const QPixmap screenshot = screen->grabWindow(0);
    const QImage image = screenshot.toImage();


    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG",80);

    const quint32 size = imageData.size();

    QByteArray package;
    package.append(reinterpret_cast<const char*>(&size), sizeof(size));
    package.append(imageData);

    if (socket.write(package) == -1 || !socket.waitForBytesWritten(3000)) {
        qDebug() << "Failed to send data: " << socket.errorString();
        return 1;
    }

    qDebug() << "Send image of " << imageData.size() << "bytes";
    socket.disconnectFromHost();
    return 0;
}