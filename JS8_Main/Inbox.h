#pragma once

/**
 * (C) 2018 Jordan Sherer <kn4crd@gmail.com> - All Rights Reserved
 * (C) 2026 Chris Olson <chriskristin.olson@icloud.com> - All Rights Reserved
 **/

#include "Message.h"

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>

class Inbox {
  public:
    explicit Inbox(QString path);
    ~Inbox();

    bool isOpen();
    bool open();
    void close();
    QString error();

    // Low-level
    int count(QString type, QString query, QString match);
    QList<QPair<int, Message>> values(QString type, QString query, QString match,
                                      int offset, int limit);
    Message value(int key);
    int append(Message value);
    bool set(int key, Message value);
    bool del(int key);

    // High-level
    QList<QPair<int, Message>> fetchForCall(const QString& callPattern);
    int getLookaheadMessageIdForCallsign(const QString& callsign, int afterMsgId);
    int countUnreadFrom(QString from);
    QPair<int, Message> firstUnreadFrom(QString from);

    QMap<QString, int> getGroupMessageCounts();
    bool markGroupMsgDeliveredForCallsign(int msgId, QString callsign);
    int getNextGroupMessageIdForCallsign(const QString& group_name, const QString& callsign);
    int getLookaheadGroupMessageIdForCallsign(const QString& group_name,
                                              const QString& callsign, int afterMsgId);
    int countUnreadForCallsign(const QString& callsign);
    int countGroupUnreadForCallsign(const QString& group_name, const QString& callsign);

  private:
    QString path_;
    struct sqlite3* db_;

    // Schema + one-time v1 -> v2 migration. inbox_v2 is the only schema this
    // (or any future) version of the software creates or writes to; inbox_v1
    // (the legacy JSON-blob table) is only ever read, and only during the
    // one-time migration below, if an older version of the software left one
    // behind. We never write to inbox_v1 again, and never re-create it.
    // It will be deprecated in a future release when migration is deemed complete.
    bool ensureSchemaAndMigrate();
    bool createV2Schema();
    bool hasTable(const char* tableName);
    bool migrateV1ToV2();
};
