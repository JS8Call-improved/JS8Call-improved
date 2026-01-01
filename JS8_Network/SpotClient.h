#ifndef JS8SPOTCLIENT_H
#define JS8SPOTCLIENT_H

#include "JS8_Include/pimpl_h.h"
#include <QObject>
#include <QString>

class SpotClient final : public QObject {
    Q_OBJECT

  public:
    SpotClient(QString const &host, quint16 port, QString const &version,
               QObject *parent = nullptr);

    void start();

    void setLocalStation(QString const &callsign, QString const &grid,
                         QString const &info);

    void enqueueCmd(QString const &cmd, QString const &from, QString const &to,
                    QString const &relayPath, QString const &text,
                    QString const &grid, QString const &extra, int submode,
                    int dial, int offset, int snr);

    void enqueueSpot(QString const &callsign, QString const &grid, int submode,
                     int dial, int offset, int snr);

    Q_SIGNAL void error(QString const &) const;

  private:
    class impl;
    pimpl<impl> m_;
};

#endif // JS8SPOTCLIENT_H
