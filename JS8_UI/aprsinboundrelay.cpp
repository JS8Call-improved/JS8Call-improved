/**
 * @file aprsinboundrelay.cpp
 * @brief Implements inbound APRS-IS relay processing.
 */
/**
 * @brief Usage notes for APRS inbound relay.
 *
 * Create an AprsInboundRelay with callbacks for heard-list lookup, UI notices,
 * When function is enabled in settings JS8Call will listen for APRS messages via APRS-IS
 * When it detects a message to a station on the heard-list it relays it as a message to the station
 * with an @APRSIS MSG TO:<DEST> <TEXT> DE <CALLINGSTATION>
 * The destination station puts it in it's inbox. 
 */
#include "aprsinboundrelay.h"

#include "Configuration.h"
#include "JS8_Main/DriftingDateTime.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <utility>

#include "moc_aprsinboundrelay.cpp"

Q_DECLARE_LOGGING_CATEGORY(mainwindow_js8)

/**
 * @brief Construct a new AprsInboundRelay handler.
 * @param config Configuration instance for enable/aging checks.
 * @param callLookup Callback to check heard list activity.
 * @param noticeFn Callback to post UI notices.
 * @param enqueueFn Callback to enqueue relay messages for transmit.
 * @param parent QObject parent.
 */
AprsInboundRelay::AprsInboundRelay(Configuration const *config,
                                   CallActivityLookup callLookup,
                                   NoticeFn noticeFn, EnqueueFn enqueueFn,
                                   QObject *parent)
    : QObject(parent),
      m_config(config),
      m_callLookup(std::move(callLookup)),
      m_notice(std::move(noticeFn)),
      m_enqueue(std::move(enqueueFn)) {}

/**
 * @brief Process an APRS-IS message for relay.
 * @param from APRS sender callsign.
 * @param to APRS destination callsign.
 * @param message APRS message payload (may include checksum).
 */
void AprsInboundRelay::onMessageReceived(QString from, QString to,
                                         QString message) {
    qCDebug(mainwindow_js8) << "APRS Message Received from" << from << "to"
                            << to << ":" << message;

    // Explicitly log to ensure we see it
    qDebug() << "DEBUG: APRS Message Received from" << from << "to" << to << ":"
             << message;

    if (!m_config || !m_config->spot_to_aprs_relay()) {
        qDebug() << "DEBUG: APRS relay disabled";
        return;
    }

    CallActivityInfo info;
    if (m_callLookup) {
        info = m_callLookup(to);
    }

    // Check if we have heard the destination station
    if (!info.heard) {
        qDebug() << "DEBUG: Destination not in heard list:" << to;
        return;
    }

    // Check if the station is "active" if aging is enabled
    if (m_config->callsign_aging() > 0) {
        if (info.lastHeardUtc.secsTo(DriftingDateTime::currentDateTimeUtc()) >
            m_config->callsign_aging() * 60) {
            qDebug() << "DEBUG: Destination aged out:" << to;
            return;
        }
    }

    // Strip APRS message checksum (format: {number})
    // Handles cases with or without closing brace, and optional whitespace
    QRegularExpression aprsChecksumRe("\\{\\d+\\}?\\s*$");
    message.remove(aprsChecksumRe);
    message = message.trimmed();

    qDebug() << "DEBUG: APRS Message after checksum strip:" << message;

    // Construct the relay message
    // @APRSIS MSG to:<DESTCALL> <MESSAGE> DE <SENDER>
    QString relayMsg = QString("@APRSIS MSG to:%1 %2 DE %3")
                           .arg(to)
                           .arg(message)
                           .arg(from);

    qCDebug(mainwindow_js8)
        << "Relaying APRS message from" << from << "to" << to << ":" << message;

    if (m_notice) {
        m_notice(DriftingDateTime::currentDateTimeUtc(),
                 QString("APRS-IS Relay: %1 -> %2: %3")
                     .arg(from)
                     .arg(to)
                     .arg(message));
    }

    if (m_enqueue) {
        m_enqueue(relayMsg);
    }
}
