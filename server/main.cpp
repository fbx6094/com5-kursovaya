#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTcpServer server;

    QObject::connect(&server, &QTcpServer::newConnection,
                     [&server]() {
        QTcpSocket *socket = server.nextPendingConnection();

        qDebug() << "Client connected:"
                 << socket->peerAddress().toString()
                 << ":" << socket->peerPort();

        QObject::connect(socket, &QTcpSocket::disconnected,
                         socket, &QTcpSocket::deleteLater);
    });

    if (!server.listen(QHostAddress::Any, 5000)) {
        qCritical() << "Server start error:"
                    << server.errorString();
        return 1;
    }

    qDebug() << "Server started on port"
             << server.serverPort();

    return app.exec();
}
