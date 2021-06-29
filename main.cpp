#include <QCoreApplication>
#include <QDebug>
#include <QBluetoothSocket>
#include <QBluetoothServer>

class BluetoothServer : public QObject
{
public:
  std::function<void(QBluetoothSocket*)> OnRecv;

  BluetoothServer()
  : OnRecv([](auto){})
  , server_{QBluetoothServiceInfo::RfcommProtocol}
  , serviceInfo_{} {
    setupServer();
  }

  ~BluetoothServer() {
    foreach (auto* client, clients_)
      client->abort();
    clients_.clear();
    serviceInfo_.unregisterService();
    server_.close();
  }

  void Listen(const QString& serviceUuid, const QString& serviceName) {
    serviceInfo_ = server_.listen(QBluetoothUuid(serviceUuid), serviceName);
  }

private:
  void setupServer() {
    connect(&server_, &QBluetoothServer::newConnection, [this] {
      forever {
        auto* socket = server_.nextPendingConnection();
        if (!socket) break;
        setupClient(socket);
        clients_.append(socket);
      }
    });
    connect(&server_, static_cast<void(QBluetoothServer::*)(QBluetoothServer::Error)>(&QBluetoothServer::error),
        [](QBluetoothServer::Error err) { qDebug() << err; });
  }

  void setupClient(QBluetoothSocket* socket) {
    connect(socket, &QBluetoothSocket::readyRead, [this, socket] { OnRecv(socket); });
    connect(socket, &QBluetoothSocket::disconnected, [this, socket] { clients_.removeOne(socket); });
    connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
        [] (QBluetoothSocket::SocketError err) { qDebug() << err; });
  }

  QBluetoothServer server_;
  QBluetoothServiceInfo serviceInfo_;
  QList<QBluetoothSocket*> clients_;
};

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);

  BluetoothServer server;
  server.OnRecv = [](auto* socket) {
    qDebug() << socket->readAll();
  };
  server.Listen("00001101-0000-1000-8000-00805F9B34FD", "My Bluetooth Server");

  qDebug() << "Bluetooth Listen OK";

  return app.exec();
}

