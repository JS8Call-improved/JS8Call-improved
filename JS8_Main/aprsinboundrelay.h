/**
 * @file aprsinboundrelay.h
 * @brief Defines the inbound APRS-IS relay handler for JS8Call.
 */
#ifndef APRSINBOUNDRELAY_H
#define APRSINBOUNDRELAY_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <functional>

class Configuration;

/**
 * @brief Handles inbound APRS-IS messages and enqueues JS8 relays.
 */
class AprsInboundRelay : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Lightweight lookup info for the heard list.
     */
    struct CallActivityInfo {
        bool heard = false;
        QDateTime lastHeardUtc;
    };

    using CallActivityLookup =
        std::function<CallActivityInfo(QString const &call)>;
    using NoticeFn =
        std::function<void(QDateTime const &utc, QString const &text)>;
    using EnqueueFn = std::function<void(QString const &message)>;

    /**
     * @brief Construct a new AprsInboundRelay handler.
     * @param config Configuration instance for enable/aging checks.
     * @param callLookup Callback to check heard list activity.
     * @param noticeFn Callback to post UI notices.
     * @param enqueueFn Callback to enqueue relay messages for transmit.
     * @param parent QObject parent.
     */
    AprsInboundRelay(Configuration const *config,
                     CallActivityLookup callLookup, NoticeFn noticeFn,
                     EnqueueFn enqueueFn, QObject *parent = nullptr);

public slots:
    /**
     * @brief Process an APRS-IS message for relay.
     * @param from APRS sender callsign.
     * @param to APRS destination callsign.
     * @param message APRS message payload (may include checksum).
     */
    void onMessageReceived(QString from, QString to, QString message);

private:
    Configuration const *m_config;
    CallActivityLookup m_callLookup;
    NoticeFn m_notice;
    EnqueueFn m_enqueue;
};

#endif
