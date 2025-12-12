#include "WSJTXMessageMapper.hpp"
#include "mainwindow.h"
#include <limits>

WSJTXMessageMapper::WSJTXMessageMapper (WSJTXMessageClient * client, MainWindow * main_window, QObject * parent)
  : QObject {parent}
  , client_ {client}
  , main_window_ {main_window}
{
  connect (client_, &WSJTXMessageClient::reply, this, &WSJTXMessageMapper::handleReply);
  connect (client_, &WSJTXMessageClient::free_text, this, &WSJTXMessageMapper::handleFreeText);
  connect (client_, &WSJTXMessageClient::halt_tx, this, &WSJTXMessageMapper::handleHaltTx);
  connect (client_, &WSJTXMessageClient::location, this, &WSJTXMessageMapper::handleLocation);
}

void WSJTXMessageMapper::sendStatusUpdate (Radio::Frequency dial_freq, Radio::Frequency offset,
                                            QString const& mode, QString const& dx_call,
                                            QString const& de_call, QString const& de_grid,
                                            QString const& dx_grid, bool tx_enabled,
                                            bool transmitting, bool decoding, QString const& tx_message)
{
  Radio::Frequency freq = dial_freq + offset;
  QString submode = mode; // JS8Call submode
  bool fast_mode = false; // Map from JS8Call speed
  quint8 special_op_mode = 0; // NONE
  quint32 frequency_tolerance = std::numeric_limits<quint32>::max ();
  quint32 tr_period = std::numeric_limits<quint32>::max ();
  QString configuration_name = "";
  
  client_->status_update (freq, mode, dx_call, "", mode, tx_enabled, transmitting, decoding,
                          static_cast<quint32>(offset), static_cast<quint32>(offset),
                          de_call, de_grid, dx_grid, false, submode, fast_mode,
                          special_op_mode, frequency_tolerance, tr_period, configuration_name, tx_message);
}

void WSJTXMessageMapper::sendDecode (bool is_new, QTime time, qint32 snr, float delta_time,
                                      quint32 delta_frequency, QString const& mode,
                                      QString const& message, bool low_confidence)
{
  client_->decode (is_new, time, snr, delta_time, delta_frequency, mode, message, low_confidence, false);
}

void WSJTXMessageMapper::sendQSOLogged (QDateTime time_off, QString const& dx_call, QString const& dx_grid,
                                         Radio::Frequency dial_frequency, QString const& mode,
                                         QString const& report_sent, QString const& report_received,
                                         QString const& my_call, QString const& my_grid)
{
  QDateTime time_on = time_off; // JS8Call doesn't track time_on separately
  QString operator_call = "";
  QString exchange_sent = "";
  QString exchange_rcvd = "";
  QString propmode = "";
  
  client_->qso_logged (time_off, dx_call, dx_grid, dial_frequency, mode, report_sent, report_received,
                       "", "", "", time_on, operator_call, my_call, my_grid,
                       exchange_sent, exchange_rcvd, propmode);
}

void WSJTXMessageMapper::handleReply (QTime /*time*/, qint32 /*snr*/, float /*delta_time*/, quint32 /*delta_frequency*/,
                                       QString const& /*mode*/, QString const& /*message_text*/,
                                       bool /*low_confidence*/, quint8 /*modifiers*/)
{
  // Map WSJT-X Reply to JS8Call action
  // This would trigger a reply in JS8Call similar to double-clicking a decode
  // Implementation depends on MainWindow API - for now, just send as network message
  if (main_window_) {
    // TODO: Map to appropriate JS8Call action
    // main_window_->sendNetworkMessage("REPLY", message_text);
  }
}

void WSJTXMessageMapper::handleFreeText (QString const& text, bool send)
{
  // Map to JS8Call TX.SET_TEXT message
  if (main_window_) {
    main_window_->sendNetworkMessage("TX.SET_TEXT", text);
    if (send) {
      main_window_->sendNetworkMessage("TX.SEND_MESSAGE", text);
    }
  }
}

void WSJTXMessageMapper::handleHaltTx (bool auto_only)
{
  // Stop transmission
  if (main_window_ && !auto_only) {
    // TODO: Stop TX immediately
    // main_window_->sendNetworkMessage("TX.HALT", "");
  }
}

void WSJTXMessageMapper::handleLocation (QString const& location)
{
  // Map to JS8Call STATION.SET_GRID
  if (main_window_) {
    main_window_->sendNetworkMessage("STATION.SET_GRID", location);
  }
}

