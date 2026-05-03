/** @file
 * @brief member function of the UI_Constructor class
 *  process HB ACK rate limiting
 */

#include "JS8_UI/mainwindow.h"

void UI_Constructor::processHeartbeatRateLimit(const QString &callsign) {
    // Feature disabled in settings
    if (!m_config.hb_rate_limit()) {
        return;
    }

    constexpr int BLOCK_WINDOW_SECS = 55 * 60;

    HBBlockingDB db(hbBlockingPath());
    if (!db.open()) {
        qCDebug(mainwindow_js8)
            << "HBBlockingDB failed to open:" << db.error();
        return;
    }

    auto now  = DriftingDateTime::currentDateTimeUtc();
    auto last = db.getTimestamp(callsign);

    if (last.isValid() && last.secsTo(now) < BLOCK_WINDOW_SECS) {
        qCDebug(mainwindow_js8)
            << "HB rate-limit triggered for" << callsign
            << "- only" << last.secsTo(now) << "s since last HB";

        m_config.addToHbBlacklist(callsign);
        db.deleteTimestamp(callsign);
        return;
    }

    // First contact or window expired — record/refresh the timestamp
    db.upsertTimestamp(callsign, now);
}
