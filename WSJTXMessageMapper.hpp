#ifndef WSJTX_MESSAGE_MAPPER_HPP__
#define WSJTX_MESSAGE_MAPPER_HPP__

#include <QObject>
#include <QTime>
#include <QDateTime>
#include <QString>

#include "WSJTXMessageClient.hpp"
#include "Radio.hpp"

class MainWindow;

class WSJTXMessageMapper : public QObject
{
  Q_OBJECT

public:
  explicit WSJTXMessageMapper (WSJTXMessageClient * client, MainWindow * main_window, QObject * parent = nullptr);

  // Map JS8Call events to WSJT-X messages
  void sendStatusUpdate (Radio::Frequency dial_freq, Radio::Frequency offset, 
                         QString const& mode, QString const& dx_call,
                         QString const& de_call, QString const& de_grid,
                         QString const& dx_grid, bool tx_enabled, 
                         bool transmitting, bool decoding, QString const& tx_message);
  
  void sendDecode (bool is_new, QTime time, qint32 snr, float delta_time,
                   quint32 delta_frequency, QString const& mode,
                   QString const& message, bool low_confidence);
  
  void sendQSOLogged (QDateTime time_off, QString const& dx_call, QString const& dx_grid,
                      Radio::Frequency dial_frequency, QString const& mode,
                      QString const& report_sent, QString const& report_received,
                      QString const& my_call, QString const& my_grid);

private slots:
  void handleReply (QTime, qint32 snr, float delta_time, quint32 delta_frequency,
                    QString const& mode, QString const& message_text,
                    bool low_confidence, quint8 modifiers);
  void handleFreeText (QString const& text, bool send);
  void handleHaltTx (bool auto_only);
  void handleLocation (QString const& location);

private:
  WSJTXMessageClient * client_;
  MainWindow * main_window_;
};

#endif

