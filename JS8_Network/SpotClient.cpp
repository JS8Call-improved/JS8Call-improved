/**
 * @file SpotClient.cpp
 * @brief Implementation of SpotClient class
 */
#include "SpotClient.h"
#include "JS8_Include/pimpl_impl.h"
#include "JS8_Main/Message.h"
#include "moc_SpotClient.cpp"
#include <QHostInfo>
#include <QLoggingCategory>
#include <QNetworkDatagram>
#include <QQueue>
#include <QTimer>
#include <QUdpSocket>

Q_DECLARE_LOGGING_CATEGORY(spotclient_js8)

/******************************************************************************/
// Constants
/******************************************************************************/

namespace {
constexpr auto SEND_INTERVAL = std::chrono::seconds(60);
}

/******************************************************************************/
// Local Utilities
/******************************************************************************/

namespace {
template <typename T> bool changeValue(T &stored, T const &update) {
    if (stored == update)
        return false;
    stored = update;
    return true;
}
} // namespace

/******************************************************************************/
// Private Implementation
/******************************************************************************/

/**
 * @private
 * @brief Private implementation of the SpotClient class.
 * 
 */
class SpotClient::impl final : public QUdpSocket {
    Q_OBJECT

  public:
    // Constructor

    impl(QString const &name, quint16 const port, QString const &version,
         SpotClient *self)
        : QUdpSocket{self}, self_{self}, name_{name}, port_{port},
          version_{version}, send_{new QTimer{this}} {}

    // Intended to be called on the thread that starts us, which can be
    // the main thread, if we're not going to be moved to a background
    // thread.

    void start() {
        // Note that With UDP, error reporting is not guaranteed, which is not
        // the same as a guarantee of no error reporting. Typically, a packet
        // arriving on a port where there is no listener will trigger an ICMP
        // Port Unreachable message back to the sender, and some implementations
        // e.g., Windows, will report that to the application on the next
        // attempt to transmit to the same destination.

        connect(this, &QUdpSocket::errorOccurred, [this](SocketError const e) {
            if (e != ConnectionRefusedError) {
                Q_EMIT self_->error(errorString());
            }
        });

        // Start a host lookup for the name we were provided. If it succeeds,
        // use the first address in the list. If it fails, then we've missed
        // what was our one and only shot at this.

        QHostInfo::lookupHost(name_, this, [this](QHostInfo const &info) {
            if (auto const &list = info.addresses(); !list.isEmpty()) {
                host_ = list.first();

                qCDebug(spotclient_js8)
                    << "SpotClient Host:" << name_ << host_.toString();

                bind(host_.protocol() == IPv6Protocol ? QHostAddress::AnyIPv6
                                                      : QHostAddress::AnyIPv4);

                send_->start(SEND_INTERVAL);
            } else {
                Q_EMIT self_->error(
                    QString{"Host lookup failed: %1"}.arg(info.errorString()));
                valid_ = false;
                queue_.clear();
            }
        });

        // Empty the queue every time our timer goes off.

        connect(send_, &QTimer::timeout, this, [this]() {
            while (!queue_.isEmpty()) {
                writeDatagram(queue_.dequeue().toJson(), host_, port_);
            }
            sent_++;
        });
    }

    // Sent as the "BY" value on command and spot sends; contains the call
    // sign and grid of the local station, as set by setLocalStation().

    QVariantMap by() {
        return {
            {"CALLSIGN", QVariant(call_)},
            {"GRID", QVariant(grid_)},
        };
    }

    // Data members

    SpotClient *self_;
    QString name_;
    quint16 port_;
    QString version_;
    QTimer *send_;
    QHostAddress host_;
    QQueue<Message> queue_;
    bool valid_ = true;
    bool once_ = false;
    int sent_ = 0;
    QString call_;
    QString grid_;
    QString info_;
};

/******************************************************************************/
// Implementation
/******************************************************************************/

#include "SpotClient.moc"

// Constructor

/**
 * @brief Constructs a SpotClient object.
 * 
 * @param name The hostname of the spot server.
 * @param port The port number of the spot server.
 * @param version The version string of the client.
 * @param parent The parent QObject.
 */
SpotClient::SpotClient(QString const &name, quint16 const port,
                       QString const &version, QObject *parent)
    : QObject{parent}, m_{name, port, version, this} {}

void SpotClient::start() {
    if (!m_->once_) {
        m_->once_ = true;
        m_->start();
    }
}

/**
 * @brief Sets the local station information.
 * 
 * @param callsign The callsign of the local station.
 * @param grid The grid locator of the local station.
 * @param info Additional info about the local station.
 */
void SpotClient::setLocalStation(QString const &callsign, QString const &grid,
                                 QString const &info) {
    qCDebug(spotclient_js8) << "SpotClient Set Local Station:" << callsign
                            << "grid:" << grid << "info:" << info;

    auto const changed = changeValue(m_->call_, callsign) +
                         changeValue(m_->grid_, grid) +
                         changeValue(m_->info_, info);

    // Send local information to network on change, or once every 15 minutes.

    if (m_->valid_ && (changed || m_->sent_ % 15 == 0)) {
        m_->queue_.enqueue({"RX.LOCAL",
                            "",
                            {{"CALLSIGN", QVariant(callsign)},
                             {"GRID", QVariant(grid)},
                             {"INFO", QVariant(info)},
                             {"VERSION", QVariant(m_->version_)}}});
    }
}

/**
 * @brief Enqueues a command to be sent to the spot server.
 * 
 * @param cmd The command string.
 * @param from The sender callsign.
 * @param to The recipient callsign.
 * @param relayPath The relay path.
 * @param text The message text.
 * @param grid The grid locator.
 * @param extra Additional information.
 * @param submode The submode identifier.
 * @param dial The dial frequency.
 * @param offset The frequency offset.
 * @param snr The signal-to-noise ratio.
 */
void SpotClient::enqueueCmd(QString const &cmd, QString const &from,
                            QString const &to, QString const &relayPath,
                            QString const &text, QString const &grid,
                            QString const &extra, int const submode,
                            int const dial, int const offset, int const snr) {
    if (m_->valid_) {
        m_->queue_.enqueue({"RX.DIRECTED",
                            "",
                            {{"BY", QVariant(m_->by())},
                             {"CMD", QVariant(cmd)},
                             {"FROM", QVariant(from)},
                             {"TO", QVariant(to)},
                             {"PATH", QVariant(relayPath)},
                             {"TEXT", QVariant(text)},
                             {"GRID", QVariant(grid)},
                             {"EXTRA", QVariant(extra)},
                             {"FREQ", QVariant(dial + offset)},
                             {"DIAL", QVariant(dial)},
                             {"OFFSET", QVariant(offset)},
                             {"SNR", QVariant(snr)},
                             {"SPEED", QVariant(submode)}}});
    }
}

/**
 * @brief Enqueues a spot to be sent to the spot server.
 * 
 * @param callsign The callsign of the spotted station.
 * @param grid The grid locator of the spotted station.
 * @param submode The submode identifier.
 * @param dial The dial frequency.
 * @param offset The frequency offset.
 * @param snr The signal-to-noise ratio.
 */
void SpotClient::enqueueSpot(QString const &callsign, QString const &grid,
                             int const submode, int const dial,
                             int const offset, int const snr) {
    if (m_->valid_) {
        m_->queue_.enqueue({"RX.SPOT",
                            "",
                            {{"BY", QVariant(m_->by())},
                             {"CALLSIGN", QVariant(callsign)},
                             {"GRID", QVariant(grid)},
                             {"FREQ", QVariant(dial + offset)},
                             {"DIAL", QVariant(dial)},
                             {"OFFSET", QVariant(offset)},
                             {"SNR", QVariant(snr)},
                             {"SPEED", QVariant(submode)}}});
    }
}

/******************************************************************************/

Q_LOGGING_CATEGORY(spotclient_js8, "spotclient.js8", QtWarningMsg)
