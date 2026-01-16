/**
 * @file APRSISClient.cpp
 * @brief Implementation of APRS-IS client for JS8Call
 */
#include "APRSISClient.h"
#include <QLoggingCategory>
#include <QRandomGenerator>

#include <cmath>

#include "DriftingDateTime.h"
#include "varicode.h"

Q_DECLARE_LOGGING_CATEGORY(aprsisclient_js8)

const int PACKET_TIMEOUT_SECONDS = 300;

/**
 * @brief Construct a new APRSISClient::APRSISClient object
 * 
 * @param host 
 * @param port 
 * @param parent 
 */
APRSISClient::APRSISClient(QString const host, quint16 const port,
                           QObject *parent)
    : QTcpSocket{parent}, m_timer{this}, m_incomingRelayEnabled{false},
      m_isLoggedIn{false} {
    setServer(host, port);

    connect(&m_timer, &QTimer::timeout, this, &APRSISClient::sendReports);
    m_timer.start(std::chrono::minutes(1));

    connect(this, &QTcpSocket::connected, this, &APRSISClient::onSocketConnected);
    connect(this, &QTcpSocket::readyRead, this, &APRSISClient::onSocketReadyRead);
    connect(this, &QTcpSocket::disconnected, this,
            &APRSISClient::onSocketDisconnected);
    connect(this, &QTcpSocket::errorOccurred, this, &APRSISClient::onSocketError);
}

/**
 * @brief Compute APRS-IS passcode for a callsign.
 *
 * @param callsign
 * @return quint32
 */
quint32 APRSISClient::hashCallsign(QString callsign) {
    // based on: https://github.com/hessu/aprsc/blob/master/src/passcode.c
    QByteArray rootCall =
        QString(callsign.split("-").first().toUpper()).toLocal8Bit() + '\0';
    quint32 hash = 0x73E2;

    qsizetype i = 0;
    qsizetype const len = rootCall.length();

    while (i + 1 < len) {
        hash ^= rootCall.at(i) << 8;
        hash ^= rootCall.at(i + 1);
        i += 2;
    }

    return hash & 0x7FFF;
}

/**
 * @brief Create a login frame for APRS-IS.
 *
 * @param callsign
 * @param filter
 * @return QString
 */
QString APRSISClient::loginFrame(QString callsign, QString filter) {
    auto loginFrame = QString("user %1 pass %2 ver %3 %4\n");
    loginFrame = loginFrame.arg(callsign);
    loginFrame = loginFrame.arg(hashCallsign(callsign));
    loginFrame = loginFrame.arg("JS8Call");
    if (!filter.isEmpty()) {
        loginFrame = loginFrame.arg(filter);
    } else {
        loginFrame = loginFrame.arg("");
    }
    return loginFrame;
}

/**
 * @brief Find all matches of a regular expression in a string
 * 
 * @param re 
 * @param content 
 * @return QList<QStringList> 
 */
QList<QStringList> findall(QRegularExpression re, QString content) {
    qsizetype pos = 0;
    QList<QStringList> all;

    while (pos < content.length()) {
        auto match = re.match(content, pos);
        if (!match.hasMatch()) {
            break;
        }

        all.append(match.capturedTexts());
        pos = match.capturedEnd();
    }

    return all;
}

/**
 * @brief Floor division for long integers
 * 
 * @param num 
 * @param den 
 * @return long 
 */
inline long floordiv(long num, long den) {
    if (0 < (num ^ den))
        return num / den;
    else {
        ldiv_t res = ldiv(num, den);
        return (res.rem) ? res.quot - 1 : res.quot;
    }
}

// convert an arbitrary length grid locator to a high precision lat/lon
/**
 * @brief Convert grid locator to degrees
 * 
 * @param locator 
 * @return QPair<float, float> 
 */
QPair<float, float> APRSISClient::grid2deg(QString locator) {
    QString grid = locator.toUpper();

    float lat = -90;
    float lon = -90;

    auto lats = findall(QRegularExpression("([A-X])([A-X])"), grid);
    auto lons = findall(QRegularExpression("(\\d)(\\d)"), grid);

    int valx[22];
    int valy[22];

    int i = 0;
    int tot = 0;
    char A = 'A';
    foreach (QStringList matched, lats) {
        char x = matched.at(1).at(0).toLatin1();
        char y = matched.at(2).at(0).toLatin1();

        valx[i * 2] = x - A;
        valy[i * 2] = y - A;

        i++;
        tot++;
    }

    i = 0;
    foreach (QStringList matched, lons) {
        int x = matched.at(1).toInt();
        int y = matched.at(2).toInt();
        valx[i * 2 + 1] = x;
        valy[i * 2 + 1] = y;

        i++;
        tot++;
    }

    for (int i = 0; i < tot; i++) {
        int x = valx[i];
        int y = valy[i];

        int z = i - 1;
        float scale = pow(10, floordiv(-(z - 1), 2)) * pow(24, floordiv(-z, 2));

        lon += scale * x;
        lat += scale * y;
    }

    lon *= 2;

    return {lat, lon};
}

// convert an arbitrary length grid locator to a high precision lat/lon in aprs
// format
/**
 * @brief Convert grid locator to APRS format
 * 
 * @param grid 
 * @return QPair<QString, QString> 
 */
QPair<QString, QString> APRSISClient::grid2aprs(QString grid) {
    auto geo = APRSISClient::grid2deg(grid);
    auto lat = geo.first;
    auto lon = geo.second;

    QString latDir = "N";
    if (lat < 0) {
        lat *= -1;
        latDir = "S";
    }

    QString lonDir = "E";
    if (lon < 0) {
        lon *= -1;
        lonDir = "W";
    }

    double iLat, fLat, iLon, fLon, iLatMin, fLatMin, iLonMin, fLonMin, iLatSec,
        iLonSec;
    fLat = modf(lat, &iLat);
    fLon = modf(lon, &iLon);

    fLatMin = modf(fLat * 60, &iLatMin);
    fLonMin = modf(fLon * 60, &iLonMin);

    iLatSec = round(fLatMin * 60);
    iLonSec = round(fLonMin * 60);

    if (iLatSec == 60) {
        iLatMin += 1;
        iLatSec = 0;
    }

    if (iLonSec == 60) {
        iLonMin += 1;
        iLonSec = 0;
    }

    if (iLatMin == 60) {
        iLat += 1;
        iLatMin = 0;
    }

    if (iLonMin == 60) {
        iLon += 1;
        iLonMin = 0;
    }

    double aprsLat = iLat * 100 + iLatMin + (iLatSec / 60.0);
    double aprsLon = iLon * 100 + iLonMin + (iLonSec / 60.0);

    return {QString("%1%2").arg(aprsLat, 7, 'f', 2, QChar('0')).arg(latDir),
            QString("%1%2").arg(aprsLon, 8, 'f', 2, QChar('0')).arg(lonDir)};
}

/**
 * @brief Strip SSID from callsign
 * 
 * @param call 
 * @return QString 
 */
QString APRSISClient::stripSSID(QString call) {
    return QString(call.split("-").first().toUpper());
}

/**
 * @brief Replace callsign suffix with SSID
 * 
 * @param call 
 * @param base 
 * @return QString 
 */
QString APRSISClient::replaceCallsignSuffixWithSSID(QString call,
                                                    QString base) {
    if (call != base) {
        QRegularExpression re("[/](?<ssid>(\\d+))");
        auto matcher = re.globalMatch(call);
        if (matcher.hasNext()) {
            auto match = matcher.next();
            auto ssid = match.captured("ssid");
            call = base + "-" + ssid;
        } else {
            call = base;
        }
    }
    return call;
}

/**
 * @brief Enable or disable persistent inbound relay connections.
 *
 * When enabled, the client keeps an active connection so inbound APRS
 * messages can be received and relayed. When disabled, connections are
 * closed once the outbound queue drains.
 *
 * @param enabled
 */
void APRSISClient::setIncomingRelayEnabled(bool enabled) {
    qCDebug(aprsisclient_js8) << "APRSISClient::setIncomingRelayEnabled(" << enabled << ")";
    if (m_incomingRelayEnabled == enabled)
        return;

    m_incomingRelayEnabled = enabled;

    if (m_incomingRelayEnabled) {
        // if enabled, trigger a connection immediately
        processQueue(false);
    } else {
        // if disabled, and queue is empty, disconnect
        if (state() == QTcpSocket::ConnectedState && m_frameQueue.isEmpty()) {
            disconnectFromHost();
        }
    }
}

/**
 * @brief Enqueue a spot frame for APRS-IS
 * 
 * @param by_call 
 * @param from_call 
 * @param grid 
 * @param comment 
 */
void APRSISClient::enqueueSpot(QString by_call, QString from_call, QString grid,
                               QString comment) {
    if (!isPasscodeValid()) {
        return;
    }

    auto geo = APRSISClient::grid2aprs(grid);
    auto spotFrame = QString("%1>APJ8CL,qAS,%2:=%3/%4G#JS8 %5\n");
    spotFrame = spotFrame.arg(from_call);
    spotFrame = spotFrame.arg(by_call);
    spotFrame = spotFrame.arg(geo.first);
    spotFrame = spotFrame.arg(geo.second);
    spotFrame = spotFrame.arg(comment.left(42));
    enqueueRaw(spotFrame);
}

/**
 * @brief Enqueue a third-party message frame for APRS-IS
 * 
 * @param by_call 
 * @param from_call 
 * @param text 
 */
void APRSISClient::enqueueThirdParty(QString by_call, QString from_call,
                                     QString text) {
    if (!isPasscodeValid()) {
        return;
    }

    auto frame = QString("%1>APJ8CL,qAS,%2:%3\n");
    frame = frame.arg(from_call);
    frame = frame.arg(by_call);
    frame = frame.arg(text);

    enqueueRaw(frame);
}

/**
 * @brief Enqueue a raw APRS frame for APRS-IS
 * 
 * @param aprsFrame 
 */
void APRSISClient::enqueueRaw(QString aprsFrame) {
    m_frameQueue.enqueue({aprsFrame, DriftingDateTime::currentDateTimeUtc()});
}

/**
 * @brief Process the APRS-IS frame queue
 * 
 * @param disconnect 
 */
void APRSISClient::processQueue(bool disconnect) {
    // don't process queue if we haven't set our local callsign
    if (m_localCall.isEmpty()) {
        qCDebug(aprsisclient_js8) << "APRSISClient Abort ProcessQueue: No Local Call";
        return;
    }

    // don't process queue if there's no host
    if (m_host.isEmpty() || m_port == 0) {
        // no host, so let's clear the queue and exit
        m_frameQueue.clear();
        return;
    }

    // if we're not connected, connect
    // the queue will be processed in onSocketConnected
    if (state() != QTcpSocket::ConnectedState) {
        qCDebug(aprsisclient_js8)
            << "APRSISClient Connecting:" << m_host << m_port;
        connectToHost(m_host, m_port);
        return;
    }

    // if we're connected but not logged in, wait
    if (!m_isLoggedIn) {
        return;
    }

    // process queue
    QQueue<QPair<QString, QDateTime>> delayed;

    while (!m_frameQueue.isEmpty()) {
        auto pair = m_frameQueue.head();
        auto frame = pair.first;
        auto timestamp = pair.second;

        // if the packet is older than the timeout, drop it.
        if (timestamp.secsTo(DriftingDateTime::currentDateTimeUtc()) >
            PACKET_TIMEOUT_SECONDS) {
            qCDebug(aprsisclient_js8)
                << "APRSISClient Packet Timeout:" << frame;
            m_frameQueue.dequeue();
            continue;
        }

        // random delay 25% of the time for throttling (a skip will add 60
        // seconds to the processing time)
        if (m_skipPercent > 0 && QRandomGenerator::global()->generate() % 100 <=
                                     (m_skipPercent * 100)) {
            qCDebug(aprsisclient_js8)
                << "APRSISClient Throttle: Skipping Frame";
            delayed.enqueue(m_frameQueue.dequeue());
            continue;
        }

        QByteArray data = frame.toLocal8Bit();
        if (write(data) == -1) {
            qCDebug(aprsisclient_js8)
                << "APRSISClient Write Error:" << errorString();
            return;
        }

        qCDebug(aprsisclient_js8) << "APRSISClient Write:" << data;
        m_frameQueue.dequeue();
    }

    // enqueue the delayed frames for later processing
    while (!delayed.isEmpty()) {
        m_frameQueue.enqueue(delayed.dequeue());
    }

    if (disconnect && !m_incomingRelayEnabled) {
        disconnectFromHost();
    }
}

/**
 * @brief Handle APRS-IS socket connection and login handshake.
 */
void APRSISClient::onSocketConnected() {
    qCDebug(aprsisclient_js8) << "APRSISClient Connected";

    QString loginFrameStr;
    if (m_incomingRelayEnabled) {
        // Include filter in login frame
        loginFrameStr = loginFrame(m_localCall, "filter t/m");
        qCDebug(aprsisclient_js8) << "APRSISClient Login with filter t/m";
    } else {
        loginFrameStr = loginFrame(m_localCall);
    }

    if (write(loginFrameStr.toLocal8Bit()) == -1) {
        qCDebug(aprsisclient_js8)
            << "APRSISClient Write Login Error:" << errorString();
        return;
    }

    // assume logged in for now, real verification is harder with async
    // but typically APRS-IS sends a line starting with #
    // we'll handle that in readyRead if needed, but for now allow writing
    m_isLoggedIn = true;

    // process any pending queue items
    processQueue(!m_incomingRelayEnabled);
}

/**
 * @brief Process incoming APRS-IS data and emit parsed messages.
 */
void APRSISClient::onSocketReadyRead() {
    while (canReadLine()) {
        auto line = QString::fromUtf8(readLine()).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        qCDebug(aprsisclient_js8) << "APRSISClient Read:" << line;

        if (line.startsWith("#")) {
            // server comment / login response
            continue;
        }

        // Parse message for relay
        // Format: SOURCE>PATH::DEST     :MESSAGE
        // Regex: ^([^>]+)>[^:]+::([A-Z0-9 ]{9}):(.*)$
        static QRegularExpression msgRe(
            "^([^>]+)>[^:]+::([A-Z0-9 ]{9}):(.*)$");
        auto match = msgRe.match(line);
        if (match.hasMatch()) {
            auto from = match.captured(1);
            auto to = match.captured(2).trimmed();
            auto msg = match.captured(3);

            qCDebug(aprsisclient_js8) << "APRSISClient Parsed Message:"
                                      << "From:" << from
                                      << "To:" << to
                                      << "Msg:" << msg;

            emit messageReceived(from, to, msg);
        } else {
             qCDebug(aprsisclient_js8) << "APRSISClient: No Regex Match for:" << line;
        }
    }
}

/**
 * @brief Handle APRS-IS socket disconnects and reconnect as needed.
 */
void APRSISClient::onSocketDisconnected() {
    qCDebug(aprsisclient_js8) << "APRSISClient Disconnected";
    m_isLoggedIn = false;

    if (m_incomingRelayEnabled) {
        // retry connection if we're supposed to be persistent
        QTimer::singleShot(5000, this, [this]() {
            if (m_incomingRelayEnabled && state() != QTcpSocket::ConnectedState) {
                processQueue(false);
            }
        });
    }
}

/**
 * @brief Log APRS-IS socket errors.
 * @param socketError Underlying socket error.
 */
void APRSISClient::onSocketError(QAbstractSocket::SocketError) {
    qCDebug(aprsisclient_js8) << "APRSISClient Error:" << errorString();
}

Q_LOGGING_CATEGORY(aprsisclient_js8, "aprsisclient.js8", QtWarningMsg)
