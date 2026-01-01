#include "WSJTXMessageClient.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QHostInfo>
#include <QLoggingCategory>
#include <QNetworkInterface>
#include <QQueue>
#include <QTextStream>
#include <QTimer>
#include <QUdpSocket>

#include "JS8_Include/pimpl_impl.h"
#include "JS8_Main/qt_helpers.h"
#include "NetworkMessage.h"

#include "moc_WSJTXMessageClient.cpp"

Q_LOGGING_CATEGORY(wsjtx_js8, "wsjtx.js8", QtWarningMsg)

// some trace macros
#define TRACE_UDP(MSG)

/**
 * @brief Helper function to dump binary payload in human-readable format
 *
 * Creates a hex dump of the message payload for debugging purposes.
 * Shows the magic number, message size, and first 64 bytes in hex and ASCII.
 *
 * @param message Binary message to dump
 * @return Formatted string with hex dump
 */
static QString dump_payload(QByteArray const &message) {
    QString result;
    QTextStream stream(&result);

    // Show first 4 bytes as magic number (if present)
    if (message.size() >= 4) {
        quint32 magic = 0;
        // Read magic number in big-endian format
        magic = (static_cast<quint8>(message[0]) << 24) |
                (static_cast<quint8>(message[1]) << 16) |
                (static_cast<quint8>(message[2]) << 8) |
                static_cast<quint8>(message[3]);
        stream << QString("Magic: 0x%1 ").arg(magic, 8, 16, QChar('0'));
    }

    // Show size and hex dump of first 64 bytes
    stream << QString("Size: %1 bytes\n").arg(message.size());
    stream << "Hex dump (first 64 bytes):\n";
    int dump_size = qMin(64, message.size());
    for (int i = 0; i < dump_size; i += 16) {
        stream << QString("%1: ").arg(i, 4, 16, QChar('0'));
        for (int j = 0; j < 16 && (i + j) < dump_size; ++j) {
            stream << QString("%1 ").arg(static_cast<quint8>(message[i + j]), 2,
                                         16, QChar('0'));
        }
        stream << " | ";
        for (int j = 0; j < 16 && (i + j) < dump_size; ++j) {
            char c = message[i + j];
            stream << (QChar::isPrint(c) ? QChar(c) : QChar('.'));
        }
        stream << "\n";
    }

    return result;
}

class WSJTXMessageClient::impl : public QUdpSocket {
    Q_OBJECT;

  public:
    impl(QString const &id, QString const &version, QString const &revision,
         port_type server_port, int TTL, WSJTXMessageClient *self)
        : self_{self}, enabled_{false}, id_{id}, version_{version},
          revision_{revision}, dns_lookup_id_{-1}, server_port_{server_port},
          TTL_{TTL},
          schema_{2} // use 2 prior to negotiation not 1 which is broken
          ,
          heartbeat_timer_{new QTimer{this}} {
        connect(heartbeat_timer_, &QTimer::timeout, this, &impl::heartbeat);
        connect(this, &QIODevice::readyRead, this, &impl::pending_datagrams);

        heartbeat_timer_->start(NetworkMessage::pulse * 1000);
    }

    ~impl() {
        closedown();
        if (dns_lookup_id_ != -1) {
            QHostInfo::abortHostLookup(dns_lookup_id_);
        }
    }

    enum StreamStatus { Fail, Short, OK };

    void set_server(QString const &server_name,
                    QStringList const &network_interface_names);
    Q_SLOT void host_info_results(QHostInfo);
    void start();
    void parse_message(QByteArray const &);
    void pending_datagrams();
    void heartbeat();
    void closedown();
    StreamStatus check_status(QDataStream const &) const;
    void send_message(QByteArray const &, bool queue_if_pending = true,
                      bool allow_duplicates = false);
    void send_message(QDataStream const &out, QByteArray const &message,
                      bool queue_if_pending = true,
                      bool allow_duplicates = false) {
        if (OK == check_status(out)) {
            send_message(message, queue_if_pending, allow_duplicates);
        } else {
            Q_EMIT self_->error("Error creating UDP message");
        }
    }

    WSJTXMessageClient *self_;
    bool enabled_;
    QString id_;
    QString version_;
    QString revision_;
    int dns_lookup_id_;
    QHostAddress server_;
    port_type server_port_;
    int TTL_;
    std::vector<QNetworkInterface> network_interfaces_;
    quint32 schema_;
    QTimer *heartbeat_timer_;
    std::vector<QHostAddress> blocked_addresses_;

    // hold messages sent before host lookup completes asynchronously
    QQueue<QByteArray> pending_messages_;
    QByteArray last_message_;
};

#include "WSJTXMessageClient.moc"

void WSJTXMessageClient::impl::set_server(
    QString const &server_name, QStringList const &network_interface_names) {
    server_.setAddress(server_name);
    network_interfaces_.clear();
    for (auto const &net_if_name : network_interface_names) {
        network_interfaces_.push_back(
            QNetworkInterface::interfaceFromName(net_if_name));
    }

    if (server_.isNull() && server_name.size()) // DNS lookup required
    {
        // queue a host address lookup
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        dns_lookup_id_ = QHostInfo::lookupHost(
            server_name, this, &WSJTXMessageClient::impl::host_info_results);
#else
        dns_lookup_id_ = QHostInfo::lookupHost(
            server_name, this, SLOT(host_info_results(QHostInfo)));
#endif
    } else {
        start();
    }
}

void WSJTXMessageClient::impl::host_info_results(QHostInfo host_info) {
    if (host_info.lookupId() != dns_lookup_id_)
        return;
    dns_lookup_id_ = -1;
    if (QHostInfo::NoError != host_info.error()) {
        Q_EMIT self_->error("UDP server DNS lookup failed: " +
                            host_info.errorString());
        return;
    } else {
        auto const &server_addresses = host_info.addresses();
        if (server_addresses.size()) {
            server_ = server_addresses[0];
        }
    }
    start();
}

void WSJTXMessageClient::impl::start() {
    if (server_.isNull()) {
        Q_EMIT self_->close();
        pending_messages_.clear(); // discard
        return;
    }

    if (is_broadcast_address(server_)) {
        Q_EMIT self_->error(
            "IPv4 broadcast not supported, please specify the loop-back "
            "address, a server host address, or multicast group address");
        pending_messages_.clear(); // discard
        return;
    }

    if (blocked_addresses_.end() != std::find(blocked_addresses_.begin(),
                                              blocked_addresses_.end(),
                                              server_)) {
        Q_EMIT self_->error("UDP server blocked, please try another");
        pending_messages_.clear(); // discard
        return;
    }

    TRACE_UDP("Trying server:" << server_.toString());
    QHostAddress interface_addr{IPv6Protocol == server_.protocol()
                                    ? QHostAddress::AnyIPv6
                                    : QHostAddress::AnyIPv4};

    if (localAddress() != interface_addr) {
        if (UnconnectedState != state() || state()) {
            close();
        }
        // bind to an ephemeral port on the selected interface and set
        // up for sending datagrams
        bind(interface_addr);
        // set multicast TTL to limit scope when sending to multicast
        // group addresses
        setSocketOption(MulticastTtlOption, TTL_);
    }

    // send initial heartbeat which allows schema negotiation
    heartbeat();

    // clear any backlog
    while (pending_messages_.size()) {
        send_message(pending_messages_.dequeue(), true, false);
    }
}

void WSJTXMessageClient::impl::pending_datagrams() {
    while (hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(pendingDatagramSize());
        QHostAddress sender_address;
        port_type sender_port;
        if (0 <= readDatagram(datagram.data(), datagram.size(), &sender_address,
                              &sender_port)) {
            TRACE_UDP("message received from:" << sender_address
                                               << "port:" << sender_port);
            parse_message(datagram);
        }
    }
}

void WSJTXMessageClient::impl::parse_message(QByteArray const &msg) {
    try {
        NetworkMessage::Reader in{msg};
        if (OK == check_status(in)) {
            if (schema_ < in.schema()) // one time record of server's
                                       // negotiated schema
            {
                schema_ = in.schema();
            }

            if (!enabled_) {
                TRACE_UDP("message processing disabled for id:" << in.id());
                return;
            }

            switch (in.type()) {
            case NetworkMessage::Reply: {
                QTime time;
                qint32 snr;
                float delta_time;
                quint32 delta_frequency;
                QByteArray mode;
                QByteArray message;
                bool low_confidence{false};
                quint8 modifiers{0};
                in >> time >> snr >> delta_time >> delta_frequency >> mode >>
                    message >> low_confidence >> modifiers;
                if (check_status(in) != Fail) {
                    Q_EMIT self_->reply(time, snr, delta_time, delta_frequency,
                                        QString::fromUtf8(mode),
                                        QString::fromUtf8(message),
                                        low_confidence, modifiers);
                }
            } break;

            case NetworkMessage::Clear: {
                quint8 window{0};
                in >> window;
                if (check_status(in) != Fail) {
                    Q_EMIT self_->clear_decodes(window);
                }
            } break;

            case NetworkMessage::Close:
                if (check_status(in) != Fail) {
                    last_message_.clear();
                    Q_EMIT self_->close();
                }
                break;

            case NetworkMessage::Replay:
                if (check_status(in) != Fail) {
                    last_message_.clear();
                    Q_EMIT self_->replay();
                }
                break;

            case NetworkMessage::HaltTx: {
                bool auto_only{false};
                in >> auto_only;
                if (check_status(in) != Fail) {
                    Q_EMIT self_->halt_tx(auto_only);
                }
            } break;

            case NetworkMessage::FreeText: {
                QByteArray message;
                bool send{true};
                in >> message >> send;
                if (check_status(in) != Fail) {
                    Q_EMIT self_->free_text(QString::fromUtf8(message), send);
                }
            } break;

            case NetworkMessage::Location: {
                QByteArray location;
                in >> location;
                if (check_status(in) != Fail) {
                    Q_EMIT self_->location(QString::fromUtf8(location));
                }
            } break;

            default:
                if (NetworkMessage::Heartbeat != in.type()) {
                    TRACE_UDP("ignoring message type:" << in.type());
                }
                break;
            }
        } else {
            TRACE_UDP("ignored message for id:" << in.id());
        }
    } catch (std::exception const &e) {
        Q_EMIT self_->error(
            QString{"WSJTXMessageClient exception: %1"}.arg(e.what()));
    } catch (...) {
        Q_EMIT self_->error("Unexpected exception in WSJTXMessageClient");
    }
}

/**
 * @brief Send a Heartbeat message
 *
 * Periodically sends Heartbeat messages to announce this application's
 * presence and capabilities to WSJT-X protocol clients.
 */
void WSJTXMessageClient::impl::heartbeat() {
    if (server_port_ && !server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::Heartbeat, id_,
                                    schema_};
        out << NetworkMessage::Builder::schema_number // maximum schema number
                                                      // accepted
            << version_.toUtf8() << revision_.toUtf8();
        qCDebug(wsjtx_js8) << "WSJT-X: Sending Heartbeat message"
                           << "id:" << id_ << "schema:" << schema_
                           << "version:" << version_ << "revision:" << revision_
                           << "to:" << server_.toString()
                           << "port:" << server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        send_message(out, message, false, true);
    }
}

/**
 * @brief Send a Close message and clean up
 *
 * Sends a Close message to notify clients that this application is shutting
 * down.
 */
void WSJTXMessageClient::impl::closedown() {
    if (server_port_ && !server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::Close, id_,
                                    schema_};
        qCDebug(wsjtx_js8) << "WSJT-X: Sending Close message"
                           << "id:" << id_ << "schema:" << schema_
                           << "to:" << server_.toString()
                           << "port:" << server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        send_message(out, message, false);
    }
}

void WSJTXMessageClient::impl::send_message(QByteArray const &message,
                                            bool queue_if_pending,
                                            bool allow_duplicates) {
    if (server_port_) {
        if (!server_.isNull()) {
            if (allow_duplicates ||
                message != last_message_) // avoid duplicates
            {
                if (is_multicast_address(server_)) {
                    // send datagram on each selected network interface
                    std::for_each(
                        network_interfaces_.begin(), network_interfaces_.end(),
                        [&](QNetworkInterface const &net_if) {
                            setMulticastInterface(net_if);
                            writeDatagram(message, server_, server_port_);
                        });
                } else {
                    writeDatagram(message, server_, server_port_);
                }
                last_message_ = message;
            }
        } else if (queue_if_pending) {
            pending_messages_.enqueue(message);
        }
    }
}

auto WSJTXMessageClient::impl::check_status(QDataStream const &stream) const
    -> StreamStatus {
    auto stat = stream.status();
    StreamStatus result{Fail};
    switch (stat) {
    case QDataStream::ReadPastEnd:
        result = Short;
        break;

    case QDataStream::ReadCorruptData:
        Q_EMIT self_->error("Message serialization error: read corrupt data");
        break;

    case QDataStream::WriteFailed:
        Q_EMIT self_->error("Message serialization error: write error");
        break;

    default:
        result = OK;
        break;
    }
    return result;
}

WSJTXMessageClient::WSJTXMessageClient(
    QString const &id, QString const &version, QString const &revision,
    QString const &server_name, port_type server_port,
    QStringList const &network_interface_names, int TTL, QObject *self)
    : QObject{self}, m_{id, version, revision, server_port, TTL, this} {
    connect(&*m_
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            ,
            static_cast<void (impl::*)(impl::SocketError)>(&impl::error),
            [this](impl::SocketError e)
#else
           , &impl::errorOccurred, [this] (impl::SocketError e)
#endif
            {
#if defined(Q_OS_WIN)
                if (e != impl::NetworkError &&
                    e != impl::ConnectionRefusedError) {
#else
                                       {
                                         Q_UNUSED (e);
#endif
                    Q_EMIT error(m_->errorString());
                }
            });
    m_->set_server(server_name, network_interface_names);
}

QHostAddress WSJTXMessageClient::server_address() const { return m_->server_; }

auto WSJTXMessageClient::server_port() const -> port_type {
    return m_->server_port_;
}

void WSJTXMessageClient::set_server(
    QString const &server_name, QStringList const &network_interface_names) {
    m_->set_server(server_name, network_interface_names);
}

void WSJTXMessageClient::set_server_port(port_type server_port) {
    m_->server_port_ = server_port;
}

void WSJTXMessageClient::set_TTL(int TTL) {
    m_->TTL_ = TTL;
    m_->setSocketOption(QAbstractSocket::MulticastTtlOption, m_->TTL_);
}

/**
 * @brief Enable or disable incoming message processing
 * @param flag true to enable, false to disable
 */
void WSJTXMessageClient::enable(bool flag) { m_->enabled_ = flag; }

/**
 * @brief Send a Status message
 *
 * Sends the current station status including frequency, mode, callsigns,
 * and operational state to WSJT-X protocol clients.
 *
 * @param f Operating frequency (Hz)
 * @param mode Operating mode string
 * @param dx_call DX station callsign (selected station)
 * @param report Report string
 * @param tx_mode TX mode string
 * @param tx_enabled Whether TX is enabled
 * @param transmitting Whether currently transmitting
 * @param decoding Whether currently decoding
 * @param rx_df Receive frequency offset (Hz)
 * @param tx_df Transmit frequency offset (Hz)
 * @param de_call My callsign
 * @param de_grid My grid square
 * @param dx_grid DX station grid square
 * @param watchdog_timeout Whether watchdog has timed out
 * @param sub_mode Sub-mode string
 * @param fast_mode Whether fast mode is enabled
 * @param special_op_mode Special operating mode
 * @param frequency_tolerance Frequency tolerance (Hz)
 * @param tr_period Transmit/receive period (seconds)
 * @param configuration_name Configuration name
 * @param tx_message Current TX message text
 */
void WSJTXMessageClient::status_update(
    Frequency f, QString const &mode, QString const &dx_call,
    QString const &report, QString const &tx_mode, bool tx_enabled,
    bool transmitting, bool decoding, quint32 rx_df, quint32 tx_df,
    QString const &de_call, QString const &de_grid, QString const &dx_grid,
    bool watchdog_timeout, QString const &sub_mode, bool fast_mode,
    quint8 special_op_mode, quint32 frequency_tolerance, quint32 tr_period,
    QString const &configuration_name, QString const &tx_message) {
    if (m_->server_port_ && !m_->server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::Status, m_->id_,
                                    m_->schema_};
        out << f << mode.toUtf8() << dx_call.toUtf8() << report.toUtf8()
            << tx_mode.toUtf8() << tx_enabled << transmitting << decoding
            << rx_df << tx_df << de_call.toUtf8() << de_grid.toUtf8()
            << dx_grid.toUtf8() << watchdog_timeout << sub_mode.toUtf8()
            << fast_mode << special_op_mode << frequency_tolerance << tr_period
            << configuration_name.toUtf8() << tx_message.toUtf8();
        qCDebug(wsjtx_js8) << "WSJT-X: Sending Status message"
                           << "freq:" << f << "mode:" << mode
                           << "dx_call:" << dx_call
                           << "tx_enabled:" << tx_enabled
                           << "transmitting:" << transmitting
                           << "decoding:" << decoding << "de_call:" << de_call
                           << "de_grid:" << de_grid << "dx_grid:" << dx_grid
                           << "sub_mode:" << sub_mode
                           << "to:" << m_->server_.toString()
                           << "port:" << m_->server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        m_->send_message(out, message);
    }
}

/**
 * @brief Send a Decode message
 *
 * Sends information about a decoded message to WSJT-X protocol clients.
 *
 * @param is_new Whether this is a new decode
 * @param time Decode time
 * @param snr Signal-to-noise ratio (dB)
 * @param delta_time Time offset from expected time (seconds)
 * @param delta_frequency Frequency offset from nominal (Hz)
 * @param mode Operating mode string
 * @param message_text Decoded message text
 * @param low_confidence Whether decode confidence is low
 * @param off_air Whether this is an off-air decode
 */
void WSJTXMessageClient::decode(bool is_new, QTime time, qint32 snr,
                                float delta_time, quint32 delta_frequency,
                                QString const &mode,
                                QString const &message_text,
                                bool low_confidence, bool off_air) {
    if (m_->server_port_ && !m_->server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::Decode, m_->id_,
                                    m_->schema_};
        out << is_new << time << snr << delta_time << delta_frequency
            << mode.toUtf8() << message_text.toUtf8() << low_confidence
            << off_air;
        qCDebug(wsjtx_js8) << "WSJT-X: Sending Decode message"
                           << "is_new:" << is_new
                           << "time:" << time.toString("hh:mm:ss")
                           << "snr:" << snr << "delta_time:" << delta_time
                           << "delta_frequency:" << delta_frequency
                           << "mode:" << mode << "message:" << message_text
                           << "low_confidence:" << low_confidence
                           << "off_air:" << off_air
                           << "to:" << m_->server_.toString()
                           << "port:" << m_->server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        m_->send_message(out, message);
    }
}

/**
 * @brief Send a Clear Decodes message
 *
 * Notifies WSJT-X protocol clients to clear the decode window.
 */
void WSJTXMessageClient::decodes_cleared() {
    if (m_->server_port_ && !m_->server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::Clear, m_->id_,
                                    m_->schema_};
        qCDebug(wsjtx_js8) << "WSJT-X: Sending Clear message"
                           << "id:" << m_->id_ << "schema:" << m_->schema_
                           << "to:" << m_->server_.toString()
                           << "port:" << m_->server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        m_->send_message(out, message);
    }
}

/**
 * @brief Send a QSO Logged message
 *
 * Sends information about a logged QSO to WSJT-X protocol clients.
 *
 * @param time_off QSO end time
 * @param dx_call DX station callsign
 * @param dx_grid DX station grid square
 * @param dial_frequency Dial frequency (Hz)
 * @param mode Operating mode
 * @param report_sent Report sent to DX station
 * @param report_received Report received from DX station
 * @param tx_power Transmit power
 * @param comments QSO comments
 * @param name DX station name
 * @param time_on QSO start time
 * @param operator_call Operator callsign
 * @param my_call My callsign
 * @param my_grid My grid square
 * @param exchange_sent Exchange sent
 * @param exchange_rcvd Exchange received
 * @param propmode Propagation mode
 */
void WSJTXMessageClient::qso_logged(
    QDateTime time_off, QString const &dx_call, QString const &dx_grid,
    Frequency dial_frequency, QString const &mode, QString const &report_sent,
    QString const &report_received, QString const &tx_power,
    QString const &comments, QString const &name, QDateTime time_on,
    QString const &operator_call, QString const &my_call,
    QString const &my_grid, QString const &exchange_sent,
    QString const &exchange_rcvd, QString const &propmode) {
    if (m_->server_port_ && !m_->server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::QSOLogged,
                                    m_->id_, m_->schema_};
        out << time_off << dx_call.toUtf8() << dx_grid.toUtf8()
            << dial_frequency << mode.toUtf8() << report_sent.toUtf8()
            << report_received.toUtf8() << tx_power.toUtf8()
            << comments.toUtf8() << name.toUtf8() << time_on
            << operator_call.toUtf8() << my_call.toUtf8() << my_grid.toUtf8()
            << exchange_sent.toUtf8() << exchange_rcvd.toUtf8()
            << propmode.toUtf8();
        qCDebug(wsjtx_js8) << "WSJT-X: Sending QSOLogged message"
                           << "time_off:" << time_off.toString(Qt::ISODate)
                           << "dx_call:" << dx_call << "dx_grid:" << dx_grid
                           << "dial_frequency:" << dial_frequency
                           << "mode:" << mode << "report_sent:" << report_sent
                           << "report_received:" << report_received
                           << "my_call:" << my_call << "my_grid:" << my_grid
                           << "to:" << m_->server_.toString()
                           << "port:" << m_->server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        m_->send_message(out, message);
    }
}

/**
 * @brief Send a Logged ADIF message
 *
 * Sends the ADIF record for a logged QSO. This is sent in addition to
 * the QSOLogged message and uses the WSJT-X LoggedADIF message format
 * (type 12). The ADIF record is formatted with a header that includes
 * the ADIF version and program ID for compatibility with WSJT-X clients.
 *
 * @param ADIF_record ADIF formatted record for the logged QSO (without header)
 */
void WSJTXMessageClient::logged_ADIF(QByteArray const &ADIF_record) {
    if (m_->server_port_ && !m_->server_.isNull()) {
        QByteArray message;
        NetworkMessage::Builder out{&message, NetworkMessage::LoggedADIF,
                                    m_->id_, m_->schema_};
        // Format ADIF with header like WSJT-X does
        // Use WSJT-X as programid for compatibility with clients expecting
        // WSJT-X format
        QByteArray ADIF{"\n<adif_ver:5>3.1.0\n<programid:6>WSJT-X\n<EOH>\n" +
                        ADIF_record + " <EOR>"};
        out << ADIF;
        qCDebug(wsjtx_js8) << "WSJT-X: Sending LoggedADIF message"
                           << "ADIF length:" << ADIF.length()
                           << "to:" << m_->server_.toString()
                           << "port:" << m_->server_port_;
        qCDebug(wsjtx_js8) << dump_payload(message);
        m_->send_message(out, message);
    }
}
