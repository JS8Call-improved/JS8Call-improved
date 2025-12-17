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

/**
 * @brief WSJT-X compatible message client for JS8Call
 *
 * This class implements the WSJT-X UDP binary protocol for inter-application
 * communication. It allows JS8Call to send and receive messages compatible
 * with WSJT-X and other applications that support the WSJT-X protocol.
 *
 * Based on WSJT-X MessageClient but adapted for JS8Call.
 */
class WSJTXMessageClient
  : public QObject
{
  Q_OBJECT;

public:
  using Frequency = Radio::Frequency;
  using port_type = quint16;

  /**
   * @brief Construct a WSJT-X message client
   *
   * Instantiates the client and initiates a host lookup on the server.
   *
   * @param id Application identifier (e.g., "JS8Call")
   * @param version Application version string
   * @param revision Application revision string
   * @param server_name Server hostname or IP address (supports multicast)
   * @param server_port UDP port number for communication
   * @param network_interface_names List of network interface names for multicast
   * @param TTL Time-to-live for multicast packets
   * @param parent Parent QObject
   */
  WSJTXMessageClient (QString const& id, QString const& version, QString const& revision,
                       QString const& server_name, port_type server_port,
                       QStringList const& network_interface_names,
                       int TTL, QObject * parent = nullptr);

  /**
   * @brief Get the server address
   * @return QHostAddress of the server, or null if not resolved
   */
  QHostAddress server_address () const;
  
  /**
   * @brief Get the server port
   * @return UDP port number
   */
  port_type server_port () const;

  /**
   * @brief Set the server address and network interfaces
   *
   * Initiates a new server host lookup and updates the network interfaces
   * used for multicast transmission.
   *
   * @param server_name Server hostname or IP address
   * @param network_interface_names List of network interface names for multicast
   */
  Q_SLOT void set_server (QString const& server_name, QStringList const& network_interface_names);

  /**
   * @brief Set the server port
   * @param server_port UDP port number (0 to disable)
   */
  Q_SLOT void set_server_port (port_type server_port = 0u);

  /**
   * @brief Set the TTL for multicast packets
   * @param TTL Time-to-live value
   */
  Q_SLOT void set_TTL (int TTL);

  /**
   * @brief Enable or disable incoming message processing
   * @param flag true to enable, false to disable
   */
  Q_SLOT void enable (bool);

  /**
   * @brief Send a Status message
   *
   * Sends the current station status including frequency, mode, callsigns,
   * and operational state.
   *
   * @param f Operating frequency (Hz)
   * @param mode Operating mode string
   * @param dx_call DX station callsign (selected station)
   * @param report Report string
   * @param tx_mode TX mode string
   * @param tx_enabled Whether TX is enabled
   * @param transmitting Whether currently transmitting
   * @param decoding Whether currently decoding
   * @param rx_df Receive frequency offset (Hz)
   * @param tx_df Transmit frequency offset (Hz)
   * @param de_call My callsign
   * @param de_grid My grid square
   * @param dx_grid DX station grid square
   * @param watchdog_timeout Whether watchdog has timed out
   * @param sub_mode Sub-mode string
   * @param fast_mode Whether fast mode is enabled
   * @param special_op_mode Special operating mode
   * @param frequency_tolerance Frequency tolerance (Hz)
   * @param tr_period Transmit/receive period (seconds)
   * @param configuration_name Configuration name
   * @param tx_message Current TX message text
   */
  Q_SLOT void status_update (Frequency, QString const& mode, QString const& dx_call, QString const& report
                             , QString const& tx_mode, bool tx_enabled, bool transmitting, bool decoding
                             , quint32 rx_df, quint32 tx_df, QString const& de_call, QString const& de_grid
                             , QString const& dx_grid, bool watchdog_timeout, QString const& sub_mode
                             , bool fast_mode, quint8 special_op_mode, quint32 frequency_tolerance
                             , quint32 tr_period, QString const& configuration_name
                             , QString const& tx_message);
  
  /**
   * @brief Send a Decode message
   *
   * Sends information about a decoded message.
   *
   * @param is_new Whether this is a new decode
   * @param time Decode time
   * @param snr Signal-to-noise ratio (dB)
   * @param delta_time Time offset from expected time (seconds)
   * @param delta_frequency Frequency offset from nominal (Hz)
   * @param mode Operating mode string
   * @param message Decoded message text
   * @param low_confidence Whether decode confidence is low
   * @param off_air Whether this is an off-air decode
   */
  Q_SLOT void decode (bool is_new, QTime time, qint32 snr, float delta_time, quint32 delta_frequency
                      , QString const& mode, QString const& message, bool low_confidence
                      , bool off_air);
  
  /**
   * @brief Send a Clear Decodes message
   *
   * Notifies clients to clear the decode window.
   */
  Q_SLOT void decodes_cleared ();
  
  /**
   * @brief Send a QSO Logged message
   *
   * Sends information about a logged QSO.
   *
   * @param time_off QSO end time
   * @param dx_call DX station callsign
   * @param dx_grid DX station grid square
   * @param dial_frequency Dial frequency (Hz)
   * @param mode Operating mode
   * @param report_sent Report sent to DX station
   * @param report_received Report received from DX station
   * @param tx_power Transmit power
   * @param comments QSO comments
   * @param name DX station name
   * @param time_on QSO start time
   * @param operator_call Operator callsign
   * @param my_call My callsign
   * @param my_grid My grid square
   * @param exchange_sent Exchange sent
   * @param exchange_rcvd Exchange received
   * @param propmode Propagation mode
   */
  Q_SLOT void qso_logged (QDateTime time_off, QString const& dx_call, QString const& dx_grid
                          , Frequency dial_frequency, QString const& mode, QString const& report_sent
                          , QString const& report_received, QString const& tx_power, QString const& comments
                          , QString const& name, QDateTime time_on, QString const& operator_call
                          , QString const& my_call, QString const& my_grid
                          , QString const& exchange_sent, QString const& exchange_rcvd
                          , QString const& propmode);
  
  /**
   * @brief Send a Logged ADIF message
   *
   * Sends the ADIF record for a logged QSO. This is sent in addition to
   * the QSOLogged message and uses the WSJT-X LoggedADIF message format.
   *
   * @param ADIF_record ADIF formatted record for the logged QSO
   */
  Q_SLOT void logged_ADIF (QByteArray const& ADIF_record);

  /**
   * @name Signals for incoming messages
   * @{
   */
  
  /**
   * @brief Emitted when a Clear Decodes message is received
   * @param window Window number to clear (0 = all windows)
   */
  Q_SIGNAL void clear_decodes (quint8 window);
  
  /**
   * @brief Emitted when a Reply message is received
   * @param time Reply time
   * @param snr Signal-to-noise ratio (dB)
   * @param delta_time Time offset (seconds)
   * @param delta_frequency Frequency offset (Hz)
   * @param mode Operating mode
   * @param message_text Reply message text
   * @param low_confidence Whether decode confidence is low
   * @param modifiers Message modifiers
   */
  Q_SIGNAL void reply (QTime, qint32 snr, float delta_time, quint32 delta_frequency, QString const& mode
                       , QString const& message_text, bool low_confidence, quint8 modifiers);
  
  /**
   * @brief Emitted when a Close message is received
   */
  Q_SIGNAL void close ();
  
  /**
   * @brief Emitted when a Replay message is received
   */
  Q_SIGNAL void replay ();
  
  /**
   * @brief Emitted when a Halt TX message is received
   * @param auto_only If true, only halt auto sequences
   */
  Q_SIGNAL void halt_tx (bool auto_only);
  
  /**
   * @brief Emitted when a Free Text message is received
   * @param text Free text to send
   * @param send Whether to send immediately
   */
  Q_SIGNAL void free_text (QString const&, bool send);
  
  /**
   * @brief Emitted when a Location message is received
   * @param location Grid square or location string
   */
  Q_SIGNAL void location (QString const&);
  
  /**
   * @brief Emitted when an error occurs
   * @param message Error message
   */
  Q_SIGNAL void error (QString const&) const;
  
  /** @} */

private:
  class impl;
  pimpl<impl> m_;
};

#endif

