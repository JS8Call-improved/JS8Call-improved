#ifndef WSJTX_MESSAGE_MAPPER_HPP__
#define WSJTX_MESSAGE_MAPPER_HPP__

#include <QObject>
#include <QTime>
#include <QDateTime>
#include <QString>

#include "WSJTXMessageClient.hpp"
#include "Radio.hpp"

class MainWindow;

/**
 * @brief Maps JS8Call events to WSJT-X protocol messages
 *
 * This class translates JS8Call's internal event format to the WSJT-X binary
 * UDP protocol format, allowing JS8Call to communicate with WSJT-X and
 * other applications that support the WSJT-X protocol.
 */
class WSJTXMessageMapper : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Construct a WSJT-X message mapper
   *
   * @param client WSJT-X message client to send messages through
   * @param main_window Main window instance for accessing JS8Call state
   * @param parent Parent QObject
   */
  explicit WSJTXMessageMapper (WSJTXMessageClient * client, MainWindow * main_window, QObject * parent = nullptr);

  /**
   * @brief Send a Status update message
   *
   * Maps JS8Call's status information to WSJT-X Status message format.
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
  void sendStatusUpdate (Radio::Frequency dial_freq, Radio::Frequency offset, 
                         QString const& mode, QString const& dx_call,
                         QString const& de_call, QString const& de_grid,
                         QString const& dx_grid, bool tx_enabled, 
                         bool transmitting, bool decoding, QString const& tx_message);
  
  /**
   * @brief Send a Decode message
   *
   * Maps JS8Call's decode information to WSJT-X Decode message format.
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
  void sendDecode (bool is_new, QTime time, qint32 snr, float delta_time,
                   quint32 delta_frequency, QString const& mode,
                   QString const& message, bool low_confidence);
  
  /**
   * @brief Send a QSO Logged message
   *
   * Maps JS8Call's QSO log information to WSJT-X QSOLogged message format.
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

