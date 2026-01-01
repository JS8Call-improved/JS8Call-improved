#ifndef PSK_REPORTER_HPP_
#define PSK_REPORTER_HPP_

#include "JS8_Include/pimpl_h.h"
#include "JS8_Main/Radio.h"
#include <QObject>

class QString;
class Configuration;
class Bands;

class PSKReporter final : public QObject {
    Q_OBJECT

  public:
    explicit PSKReporter(Configuration const *, QString const &program_info);

    ~PSKReporter();

    void start();

    void reconnect();

    void setLocalStation(QString const &call, QString const &grid,
                         QString const &antenna);

    void addRemoteStation(QString const &call, QString const &grid,
                          Radio::Frequency freq, QString const &mode, int snr,
                          QDateTime const &utcTimestamp);

    //
    // Flush any pending spots to PSK Reporter
    //
    void sendReport(bool last = false);

    Q_SIGNAL void errorOccurred(QString const &reason);

  private:
    class impl;
    pimpl<impl> m_;
};

#endif
