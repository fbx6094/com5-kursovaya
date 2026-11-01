#include <QBuffer>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTcpSocket>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", 5000);

    if (!socket.waitForConnected(3000)) {
        qDebug() << "Connection failed: " << socket.errorString();
        return 1;
    }

    QImage image(640, 480, QImage::Format_RGB32);
    image.fill(Qt::darkBlue);

    QPainter painter(&image);
    painter.setPen(Qt::white);
    painter.drawText(image.rect(), Qt::AlignCenter, "Test frame text");
    painter.end();

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

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