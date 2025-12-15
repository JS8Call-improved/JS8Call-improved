#ifndef WSJTX_MESSAGE_CLIENT_HPP__
#define WSJTX_MESSAGE_CLIENT_HPP__

#include <QObject>
#include <QTime>
#include <QDateTime>
#include <QString>
#include <QHostAddress>

#include "Radio.hpp"
#include "pimpl_h.hpp"

class QByteArray;
class QHostAddress;
class QColor;

//
// WSJTXMessageClient - WSJT-X compatible message client
// Based on WSJT-X MessageClient but adapted for JS8Call
//
class WSJTXMessageClient
  : public QObject
{
  Q_OBJECT;

public:
  using Frequency = Radio::Frequency;
  using port_type = quint16;

  // instantiate and initiate a host lookup on the server
  WSJTXMessageClient (QString const& id, QString const& version, QString const& revision,
                       QString const& server_name, port_type server_port,
                       QStringList const& network_interface_names,
                       int TTL, QObject * parent = nullptr);

  // query server details
  QHostAddress server_address () const;
  port_type server_port () const;

  // initiate a new server host lookup
  Q_SLOT void set_server (QString const& server_name, QStringList const& network_interface_names);

  // change the server port messages are sent to
  Q_SLOT void set_server_port (port_type server_port = 0u);

  // change the TTL
  Q_SLOT void set_TTL (int TTL);

  // enable incoming messages
  Q_SLOT void enable (bool);

  // outgoing messages
  Q_SLOT void status_update (Frequency, QString const& mode, QString const& dx_call, QString const& report
                             , QString const& tx_mode, bool tx_enabled, bool transmitting, bool decoding
                             , quint32 rx_df, quint32 tx_df, QString const& de_call, QString const& de_grid
                             , QString const& dx_grid, bool watchdog_timeout, QString const& sub_mode
                             , bool fast_mode, quint8 special_op_mode, quint32 frequency_tolerance
                             , quint32 tr_period, QString const& configuration_name
                             , QString const& tx_message);
  Q_SLOT void decode (bool is_new, QTime time, qint32 snr, float delta_time, quint32 delta_frequency
                      , QString const& mode, QString const& message, bool low_confidence
                      , bool off_air);
  Q_SLOT void decodes_cleared ();
  Q_SLOT void qso_logged (QDateTime time_off, QString const& dx_call, QString const& dx_grid
                          , Frequency dial_frequency, QString const& mode, QString const& report_sent
                          , QString const& report_received, QString const& tx_power, QString const& comments
                          , QString const& name, QDateTime time_on, QString const& operator_call
                          , QString const& my_call, QString const& my_grid
                          , QString const& exchange_sent, QString const& exchange_rcvd
                          , QString const& propmode);
  Q_SLOT void logged_ADIF (QByteArray const& ADIF_record);

  // signals for incoming messages
  Q_SIGNAL void clear_decodes (quint8 window);
  Q_SIGNAL void reply (QTime, qint32 snr, float delta_time, quint32 delta_frequency, QString const& mode
                       , QString const& message_text, bool low_confidence, quint8 modifiers);
  Q_SIGNAL void close ();
  Q_SIGNAL void replay ();
  Q_SIGNAL void halt_tx (bool auto_only);
  Q_SIGNAL void free_text (QString const&, bool send);
  Q_SIGNAL void location (QString const&);
  Q_SIGNAL void error (QString const&) const;

private:
  class impl;
  pimpl<impl> m_;
};

#endif

