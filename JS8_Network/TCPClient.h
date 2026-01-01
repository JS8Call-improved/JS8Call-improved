#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>

#include "JS8_Include/pimpl_h.h"

class TCPClient : public QObject {
    Q_OBJECT
  public:
    using port_type = quint16;

    explicit TCPClient(QObject *parent = nullptr);

  signals:

  public slots:
    Q_SLOT bool ensureConnected(QString host, port_type port, int msecs = 5000);
    Q_SLOT bool sendNetworkMessage(QString host, port_type port,
                                   QByteArray const &message, bool crlf = true,
                                   int msecs = 5000);

  private:
    class impl;
    pimpl<impl> m_;
};

#endif // TCPCLIENT_H
