#include <QBuffer>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QTcpSocket>
#include <QTimer>

bool sendImage(QTcpSocket& socket) {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        qCritical() << "Screen not found";
        return false;
    }
    const QPixmap screenshot = screen->grabWindow(0);
    const QImage image = screenshot.toImage();

    QByteArray imageData;
    QBuffer buffer(&imageData);

    if (!buffer.open(QIODevice::WriteOnly) ||
        !image.save(&buffer, "JPEG", 80)) {
        qCritical() << "Failed to encode image to JPEG";
        return false;
    }

    const quint32 size = imageData.size();
    QByteArray package;
    package.append(reinterpret_cast<const char*>(&size), sizeof(size));
    package.append(imageData);
    if (socket.write(package) == -1) {
        qCritical() << "Failed to send data: " << socket.errorString();
        return false;
    }
    return socket.waitForBytesWritten(3000);
}

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 5000);

    if (!socket.waitForConnected(3000)) {
        qDebug() << "Connection failed: " << socket.errorString();
        return 1;
    }
    qDebug() << "Connected to server";

    QTimer timer;

    QObject::connect(&timer, &QTimer::timeout, [&socket]() {
        if (!sendImage(socket)) {
            qDebug() << "Failed to send image";
        } else {
            qDebug() << "Image sent successfully";
        }
    });
    QObject::connect(&socket, &QTcpSocket::disconnected, &app,
                     &QGuiApplication::quit);
    timer.start(17);
    return app.exec();
}