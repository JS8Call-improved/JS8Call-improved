/**
 * @file AprsInboundRelay.cpp
 * @brief Implements inbound APRS-IS relay processing.
 */
/**
 * @brief Usage notes for APRS inbound relay.
 *
 * Create an AprsInboundRelay with callbacks for heard-list lookup, UI notices,
 * When function is enabled in settings JS8Call will listen for APRS messages
 * via APRS-IS When it detects a message to a station on the heard-list it
 * relays it as a message to the station with an @APRSIS MSG TO:<DEST> <TEXT> DE
 * <CALLINGSTATION> The destination station puts it in it's inbox.
 */
#include "AprsInboundRelay.h"
#include "JS8_Main/DriftingDateTime.h"
#include "JS8_UI/Configuration.h"

#include <QDebug>
#include <QLoggingCategory>

#include <utility>

#include "moc_AprsInboundRelay.cpp"

Q_DECLARE_LOGGING_CATEGORY(mainwindow_js8)

static QString extractMessageId(QString &message) {
    auto trimmed = message.trimmed();
    auto braceIndex = trimmed.lastIndexOf('{');
    if (braceIndex < 0) {
        return {};
    }

    auto suffix = trimmed.mid(braceIndex + 1);
    if (suffix.endsWith('}')) {
        suffix.chop(1);
    }

    if (suffix.isEmpty()) {
        return {};
    }

    for (auto const &ch : suffix) {
        if (!ch.isLetterOrNumber()) {
            return {};
        }
    }

    message = trimmed.left(braceIndex).trimmed();
    return suffix;
}

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
                                   AckFn ackFn, QObject *parent)
    : QObject(parent), m_config(config), m_callLookup(std::move(callLookup)),
      m_notice(std::move(noticeFn)), m_enqueue(std::move(enqueueFn)),
      m_ack(std::move(ackFn)) {}

/**
 * @brief Process an APRS-IS message for relay.
 * @param from APRS sender callsign.
 * @param to APRS destination callsign.
 * @param message APRS message payload (may include checksum).
 */
void AprsInboundRelay::onMessageReceived(QString from, QString to,
                                         QString message, QString messageId) {
    qCDebug(mainwindow_js8)
        << "APRS Message Received from" << from << "to" << to << ":" << message
        << "id" << messageId;

    // Explicitly log to ensure we see it
    qDebug() << "DEBUG: APRS Message Received from" << from << "to" << to << ":"
             << message << "id" << messageId;

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

    if (messageId.isEmpty()) {
        auto recoveredId = extractMessageId(message);
        if (!recoveredId.isEmpty()) {
            messageId = recoveredId;
            qDebug() << "DEBUG: APRS Message ID recovered" << messageId;
        }
    } else {
        extractMessageId(message);
    }

    qDebug() << "DEBUG: APRS Message after checksum strip:" << message;

    constexpr int kAckDedupSeconds = 120;
    bool isDuplicate = false;
    if (!messageId.isEmpty()) {
        auto dedupeKey = QString("%1|%2|%3").arg(from, to, messageId).toUpper();
        auto now = DriftingDateTime::currentDateTimeUtc();
        auto cutoff = now.addSecs(-kAckDedupSeconds);
        auto it = m_ackDedupCache.begin();
        while (it != m_ackDedupCache.end()) {
            if (it.value() < cutoff) {
                it = m_ackDedupCache.erase(it);
            } else {
                ++it;
            }
        }

        if (m_ackDedupCache.contains(dedupeKey)) {
            isDuplicate = true;
        } else {
            m_ackDedupCache.insert(dedupeKey, now);
        }

        if (m_ack) {
            m_ack(to, from, messageId);
        }
    }

    if (isDuplicate) {
        qCDebug(mainwindow_js8) << "APRS relay dedupe skip" << from << to
                                << messageId;
        return;
    }

    // Construct the relay message
    // @APRSIS MSG to:<DESTCALL> <MESSAGE> DE <SENDER>
    QString relayMsg =
        QString("@APRSIS MSG to:%1 %2 DE %3").arg(to).arg(message).arg(from);

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
