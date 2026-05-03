#pragma once

#include <QDateTime>
#include <QString>

/**
 * @class HBBlockingDB
 * @brief Tracks per-callsign heartbeat timestamps for rate-limiting ACK replies.
 *
 * Stores the last-seen HB timestamp for each callsign in a dedicated SQLite database
 * (hb_blocking.db3). If a station HBs again within the 55 minute  block window, it is
 * added to the HB blacklist and its record purged from the database
 */

struct sqlite3;
struct sqlite3_stmt;

class HBBlockingDB {
public:
    explicit HBBlockingDB(const QString &path);
    ~HBBlockingDB();

    bool open();
    void close();
    bool isOpen() const;
    QString error() const;

    bool     upsertTimestamp(const QString &callsign, const QDateTime &ts);
    QDateTime getTimestamp(const QString &callsign);
    bool     deleteTimestamp(const QString &callsign);

private:
    QString  path_;
    sqlite3 *db_;
};
