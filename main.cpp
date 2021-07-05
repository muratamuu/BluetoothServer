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
        qDebug() << "accept client";
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
    auto line = socket->readAll();
    qDebug() << "Recv:" << line;
    if (line == "ATZ\r") {
      socket->write(QString("\r\rELM327 v2.1\r\r>").toUtf8());
    } else if (line == "ATE0\r") {
      socket->write(QString("ATE0\r\rOK\r\r>").toUtf8());
    } else if (line == "ATAL\r") {
      socket->write(QString("OK\r\r>").toUtf8());
    } else if (line == "ATSP0\r") {
      socket->write(QString("OK\r\r>").toUtf8());
    } else if (line == "ATH1\r") {
      socket->write(QString("OK\r\r>").toUtf8());
    } else if (line == "0111") {  // ACCP
      static quint8 accp = 0;
      accp++;
      auto data = QString("7E8 04 41 11 %1 \r\r>").arg(accp%100, 2, 16, QChar('0')).toUtf8();
      socket->write(data);
    } else if (line == "010C") {  // RPM
      static quint16 rpm = 0;
      rpm++;
      auto data = QString("7E8 04 41 0C %1 %2 \r\r>").arg(rpm/256, 2, 16, QChar('0')).arg(rpm%256, 2, 16, QChar('0')).toUtf8();
      socket->write(data);
    } else if (line == "010D") {  // SP1
      static quint8 sp1 = 0;
      sp1++;
      auto data = QString("7E8 04 41 0D %1 \r7EA 04 41 0D %2 \r\r>").arg(sp1, 2, 16, QChar('0')).arg(sp1, 2, 16, QChar('0')).toUtf8();
      socket->write(data);
    }
  };
  server.Listen("00001101-0000-1000-8000-00805F9B34FD", "My Bluetooth Server");

  qDebug() << "Bluetooth Listen OK";

  return app.exec();
}

