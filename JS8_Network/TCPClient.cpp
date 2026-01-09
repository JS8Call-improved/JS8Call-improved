/**
 * @file TCPClient.cpp
 * @brief Implementation of TCPClient class
 */
#include "TCPClient.h"

#include <QDebug>
#include <QTcpSocket>

#include "JS8_Include/pimpl_impl.h"

#include "moc_TCPClient.cpp"

/**
 * @private
 * @brief Private implementation of the TCPClient class.
 * 
 */
class TCPClient::impl : public QTcpSocket {
    Q_OBJECT

  public:
    using port_type = quint16;

    impl(TCPClient *self) : self_{self} {}

    ~impl() {}

    bool isConnected(QString host, port_type port) {
        if (host_ != host || port_ != port) {
            disconnectFromHost();
            return false;
        }
        return state() == QTcpSocket::ConnectedState;
    }

    void connectToHostPort(QString host, port_type port) {
        host_ = host;
        port_ = port;

        QTcpSocket::connectToHost(host_, port_);
    }

    qint64 send(QByteArray const &message, bool crlf) {
        return write(message + (crlf ? "\r\f" : ""));
    }

    TCPClient *self_;
    QString host_;
    port_type port_;
};

#include "TCPClient.moc"

/**
 * @brief Construct a new TCPClient::TCPClient object
 * 
 * @param parent 
 */
TCPClient::TCPClient(QObject *parent) : QObject(parent), m_{this} {}

/**
 * @brief Ensures that the TCP client is connected to the specified host and port.
 * 
 * @param host The hostname or IP address to connect to.
 * @param port The port number to connect to.
 * @param msecs The timeout in milliseconds to wait for the connection.
 * @return true if connected successfully, false otherwise.
 */
bool TCPClient::ensureConnected(QString host, port_type port, int msecs) {
    if (!m_->isConnected(host, port)) {
        m_->connectToHostPort(host, port);
    }

    return m_->waitForConnected(msecs);
}

/**
 * @brief Sends a network message to the specified host and port.
 * 
 * @param host The hostname or IP address to send the message to.
 * @param port The port number to send the message to.
 * @param message The message data to send.
 * @param crlf Whether to append CRLF to the message.
 * @param msecs The timeout in milliseconds to wait for the connection.
 * @return true if the message was sent successfully, false otherwise.
 */
bool TCPClient::sendNetworkMessage(QString host, port_type port,
                                   QByteArray const &message, bool crlf,
                                   int msecs) {
    if (!ensureConnected(host, port, msecs)) {
        return false;
    }

    qint64 n = m_->send(message, crlf);
    if (n <= 0) {
        return false;
    }

    return m_->flush();
}
