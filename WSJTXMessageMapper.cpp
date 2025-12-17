#include "WSJTXMessageMapper.hpp"
#include "mainwindow.h"
#include <limits>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(wsjtx_mapper_js8, "wsjtx.mapper.js8", QtWarningMsg)

/**
 * @brief Construct a WSJT-X message mapper
 *
 * Sets up signal connections to handle incoming WSJT-X protocol messages
 * and map them to JS8Call actions.
 *
 * @param client WSJT-X message client to send messages through
 * @param main_window Main window instance for accessing JS8Call state
 * @param parent Parent QObject
 */
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

/**
 * @brief Send a Status update message
 *
 * Maps JS8Call's status information to WSJT-X Status message format and
 * sends it through the WSJT-X message client.
 *
 * @param dial_freq Dial frequency (Hz)
 * @param offset Frequency offset (Hz)
 * @param mode Operating mode string
 * @param dx_call DX station callsign (selected station)
 * @param de_call My callsign
 * @param de_grid My grid square
 * @param dx_grid DX station grid square
 * @param tx_enabled Whether TX is enabled
 * @param transmitting Whether currently transmitting
 * @param decoding Whether currently decoding
 * @param tx_message Current TX message text
 */
void WSJTXMessageMapper::sendStatusUpdate (Radio::Frequency dial_freq, Radio::Frequency offset,
                                            QString const& mode, QString const& dx_call,
                                            QString const& de_call, QString const& de_grid,
                                            QString const& dx_grid, bool tx_enabled,
                                            bool transmitting, bool decoding, QString const& tx_message)
{
  qCDebug(wsjtx_mapper_js8) << "WSJTXMessageMapper: sendStatusUpdate called"
                             << "dial_freq:" << dial_freq << "offset:" << offset
                             << "mode:" << mode << "dx_call:" << dx_call
                             << "de_call:" << de_call << "de_grid:" << de_grid
                             << "dx_grid:" << dx_grid << "tx_enabled:" << tx_enabled
                             << "transmitting:" << transmitting << "decoding:" << decoding;
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

/**
 * @brief Send a Decode message
 *
 * Maps JS8Call's decode information to WSJT-X Decode message format and
 * sends it through the WSJT-X message client.
 *
 * @param is_new Whether this is a new decode
 * @param time Decode time
 * @param snr Signal-to-noise ratio (dB)
 * @param delta_time Time offset from expected time (seconds)
 * @param delta_frequency Frequency offset from nominal (Hz)
 * @param mode Operating mode string
 * @param message Decoded message text
 * @param low_confidence Whether decode confidence is low
 */
void WSJTXMessageMapper::sendDecode (bool is_new, QTime time, qint32 snr, float delta_time,
                                      quint32 delta_frequency, QString const& mode,
                                      QString const& message, bool low_confidence)
{
  qCDebug(wsjtx_mapper_js8) << "WSJTXMessageMapper: sendDecode called"
                             << "is_new:" << is_new << "time:" << time.toString("hh:mm:ss")
                             << "snr:" << snr << "delta_time:" << delta_time
                             << "delta_frequency:" << delta_frequency << "mode:" << mode
                             << "message:" << message << "low_confidence:" << low_confidence;
  client_->decode (is_new, time, snr, delta_time, delta_frequency, mode, message, low_confidence, false);
}

/**
 * @brief Send a QSO Logged message
 *
 * Maps JS8Call's QSO log information to WSJT-X QSOLogged message format and
 * sends it through the WSJT-X message client.
 *
 * @param time_off QSO end time
 * @param dx_call DX station callsign
 * @param dx_grid DX station grid square
 * @param dial_frequency Dial frequency (Hz)
 * @param mode Operating mode
 * @param report_sent Report sent to DX station
 * @param report_received Report received from DX station
 * @param my_call My callsign
 * @param my_grid My grid square
 */
void WSJTXMessageMapper::sendQSOLogged (QDateTime time_off, QString const& dx_call, QString const& dx_grid,
                                         Radio::Frequency dial_frequency, QString const& mode,
                                         QString const& report_sent, QString const& report_received,
                                         QString const& my_call, QString const& my_grid)
{
  qCDebug(wsjtx_mapper_js8) << "WSJTXMessageMapper: sendQSOLogged called"
                             << "time_off:" << time_off.toString(Qt::ISODate)
                             << "dx_call:" << dx_call << "dx_grid:" << dx_grid
                             << "dial_frequency:" << dial_frequency << "mode:" << mode
                             << "report_sent:" << report_sent << "report_received:" << report_received
                             << "my_call:" << my_call << "my_grid:" << my_grid;
  QDateTime time_on = time_off; // JS8Call doesn't track time_on separately
  QString operator_call = "";
  QString exchange_sent = "";
  QString exchange_rcvd = "";
  QString propmode = "";
  
  client_->qso_logged (time_off, dx_call, dx_grid, dial_frequency, mode, report_sent, report_received,
                       "", "", "", time_on, operator_call, my_call, my_grid,
                       exchange_sent, exchange_rcvd, propmode);
}

/**
 * @brief Handle incoming Reply message from WSJT-X protocol
 *
 * Maps WSJT-X Reply message to JS8Call action. This would trigger a reply
 * in JS8Call similar to double-clicking a decode.
 *
 * @param time Reply time (unused)
 * @param snr Signal-to-noise ratio (unused)
 * @param delta_time Time offset (unused)
 * @param delta_frequency Frequency offset (unused)
 * @param mode Operating mode (unused)
 * @param message_text Reply message text (unused)
 * @param low_confidence Whether decode confidence is low (unused)
 * @param modifiers Message modifiers (unused)
 */
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

/**
 * @brief Handle incoming Free Text message from WSJT-X protocol
 *
 * Maps WSJT-X Free Text message to JS8Call TX.SET_TEXT and optionally
 * TX.SEND_MESSAGE network messages.
 *
 * @param text Free text to send
 * @param send Whether to send immediately
 */
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

/**
 * @brief Handle incoming Halt TX message from WSJT-X protocol
 *
 * Maps WSJT-X Halt TX message to JS8Call TX halt action.
 *
 * @param auto_only If true, only halt auto sequences
 */
void WSJTXMessageMapper::handleHaltTx (bool auto_only)
{
  // Stop transmission
  if (main_window_ && !auto_only) {
    // TODO: Stop TX immediately
    // main_window_->sendNetworkMessage("TX.HALT", "");
  }
}

/**
 * @brief Handle incoming Location message from WSJT-X protocol
 *
 * Maps WSJT-X Location message to JS8Call STATION.SET_GRID network message.
 *
 * @param location Grid square or location string
 */
void WSJTXMessageMapper::handleLocation (QString const& location)
{
  // Map to JS8Call STATION.SET_GRID
  if (main_window_) {
    main_window_->sendNetworkMessage("STATION.SET_GRID", location);
  }
}

