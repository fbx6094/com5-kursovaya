#include <QCoreApplication>
#include <QDebug>
#include <QTcpSocket>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QTcpSocket socket;
    qDebug() << "Connecting to server...";
    socket.connectToHost("127.0.0.1", 5000);

    if (!socket.waitForConnected(3000)) {
        qCritical() << "Connection error:" << socket.errorString();
        return 1;
    }
    qDebug() << "Connected to server:" << socket.peerAddress().toString() << ":"
             << socket.peerPort();
    QByteArray msg = "TEST HELLO!";
    if (socket.write(msg) == -1) {
        qCritical() << "send error:" << socket.errorString();
        return 1;
    }
    if (!socket.waitForBytesWritten(3000)) {
        qCritical() << "msg was never sent:" << socket.errorString();
        return 1;
    }
    qDebug() << "Message sent to server:" << msg;
    socket.disconnectFromHost();
    return 0;
}