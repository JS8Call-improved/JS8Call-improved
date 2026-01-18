/**
 * @file networkMessage.cpp
 * @brief member function of the UI_Constructor class
 * API commands for external control and data retrieval.
 *  sends data to external clients via the Network Message API
 * @defgroup API Network Message API
 */

#include "JS8_UI/mainwindow.h"

/**
 * @brief Processes an incoming API network message
 * This function acts as the primary router for the JS8Call API. It handles
 * RIG, STATION, RX, and TX commands by either updating the application
 * state or querying current values to send back to the client.
 * @param message The network message to process
 */
void UI_Constructor::networkMessage(Message const &message) {
    auto type = message.type();

    if (type == "PING") {
        return;
    }

    auto id = message.id();

    qCDebug(mainwindow_js8) << "try processing network message" << type << id;

    // Inspired by FLDigi
    // TODO: MAIN.RX - Turn on RX
    // TODO: MAIN.TX - Transmit
    // TODO: MAIN.PTT - PTT
    // TODO: MAIN.TUNE - Tune
    // TODO: MAIN.HALT - Halt
    // TODO: MAIN.AUTO - Auto
    // TODO: MAIN.SPOT - Spot
    // TODO: MAIN.HB - HB

    // RIG.GET_FREQ - Get the current Frequency
    // RIG.SET_FREQ - Set the current Frequency
    /**
     * @name RIG Commands
     * @{
     */

    /**
     * @brief RIG.GET_FREQ: Retrieves the current dial and offset frequencies.
     *
     * If the WSJT-X protocol is enabled, this also triggers a status update
     * via the @ref m_wsjtxMessageMapper.
     */
    if (type == "RIG.GET_FREQ") {
        // Send WSJT-X Status message if protocol is enabled
        if (m_wsjtxMessageMapper && m_config.wsjtx_protocol_enabled()) {
            QString dx_call = callsignSelected();
            QString dx_grid = "";
            if (!dx_call.isEmpty() && m_callActivity.contains(dx_call)) {
                dx_grid = m_callActivity[dx_call].grid;
            }
            QString tx_message = m_transmitting ? m_currentMessage : "";

            m_wsjtxMessageMapper->sendStatusUpdate(
                dialFrequency(), freq(),
                "JS8", // mode
                dx_call, m_config.my_callsign(), m_config.my_grid(), dx_grid,
                true, // tx_enabled
                m_transmitting,
                m_decoderBusy || m_monitoring, // decoding
                tx_message);
        }

        // Send native JSON message only if not conflicting with WSJT-X
        bool skip_json = false;
        if (m_config.wsjtx_protocol_enabled() &&
            m_config.wsjtx_server_port() == m_config.udp_server_port() &&
            m_config.wsjtx_server_name() == m_config.udp_server_name()) {
            skip_json = true;
        }

        if (!skip_json) {
            sendNetworkMessage(
                "RIG.FREQ", "",
                {{"_ID", id},
                 {"FREQ", QVariant((quint64)dialFrequency() + freq())},
                 {"DIAL", QVariant((quint64)dialFrequency())},
                 {"OFFSET", QVariant((quint64)freq())}});
        }
        return;
    }
    /**
     * @brief RIG.SET_FREQ: Updates the rig dial frequency and/or frequency
     * offset.
     */
    if (type == "RIG.SET_FREQ") {
        auto params = message.params();
        if (params.contains("DIAL")) {
            bool ok = false;
            auto f = params["DIAL"].toInt(&ok);
            if (ok) {
                setRig(f);
                displayDialFrequency();
            }
        }
        if (params.contains("OFFSET")) {
            bool ok = false;
            auto f = params["OFFSET"].toInt(&ok);
            if (ok) {
                setFreqOffsetForRestore(f, false);
            }
        }
    }
    /** @} */ // End RIG Commands

    // STATION.GET_CALLSIGN - Get the current callsign
    // STATION.GET_GRID - Get the current grid locator
    // STATION.SET_GRID - Set the current grid locator
    // STATION.GET_INFO - Get the current station qth
    // STATION.SET_INFO - Set the current station qth
    /**
     * @name STATION Commands
     * @{
     */

    /** @brief STATION.GET_CALLSIGN: Returns the configured station callsign. */
    if (type == "STATION.GET_CALLSIGN") {
        sendNetworkMessage("STATION.CALLSIGN", m_config.my_callsign(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.GET_GRID: Returns the current Maidenhead grid locator. */
    if (type == "STATION.GET_GRID") {
        sendNetworkMessage("STATION.GRID", m_config.my_grid(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.SET_GRID: Updates the dynamic grid locator for the
     * station. */
    if (type == "STATION.SET_GRID") {
        m_config.set_dynamic_location(message.value());
        sendNetworkMessage("STATION.GRID", m_config.my_grid(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.GET_INFO: Retrieves the station information (QTH). */
    if (type == "STATION.GET_INFO") {
        sendNetworkMessage("STATION.INFO", m_config.my_info(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.SET_INFO: Updates the dynamic station information (QTH).
     */
    if (type == "STATION.SET_INFO") {
        m_config.set_dynamic_station_info(message.value());
        sendNetworkMessage("STATION.INFO", m_config.my_info(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.GET_STATUS: Retrieves the current station status message.
     */
    if (type == "STATION.GET_STATUS") {
        sendNetworkMessage("STATION.STATUS", m_config.my_status(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief STATION.SET_STATUS: Updates the dynamic station status message.
     */
    if (type == "STATION.SET_STATUS") {
        m_config.set_dynamic_station_status(message.value());
        sendNetworkMessage("STATION.STATUS", m_config.my_status(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @} */ // End STATION Commands

    // RX.GET_CALL_ACTIVITY
    // RX.GET_CALL_SELECTED
    // RX.GET_BAND_ACTIVITY
    // RX.GET_TEXT
    /**
     * @name RX Commands
     * @{
     */

    /**
     * @brief RX.GET_CALL_ACTIVITY: Returns a list of active callsigns.
     * Filters results based on the `callsign_aging` configuration.
     */
    if (type == "RX.GET_CALL_ACTIVITY") {
        auto now = DriftingDateTime::currentDateTimeUtc();
        int callsignAging = m_config.callsign_aging();
        QVariantMap calls = {
            {"_ID", id},
        };

        foreach (auto cd, m_callActivity.values()) {
            if (callsignAging &&
                cd.utcTimestamp.secsTo(now) / 60 >= callsignAging) {
                continue;
            }
            QVariantMap detail;
            detail["SNR"] = QVariant(cd.snr);
            detail["GRID"] = QVariant(cd.grid);
            detail["UTC"] = QVariant(cd.utcTimestamp.toMSecsSinceEpoch());
            calls[cd.call] = QVariant(detail);
        }

        sendNetworkMessage("RX.CALL_ACTIVITY", "", calls);
        return;
    }
    /** @brief RX.GET_CALL_SELECTED: Returns the currently selected callsign. */
    if (type == "RX.GET_CALL_SELECTED") {
        sendNetworkMessage("RX.CALL_SELECTED", callsignSelected(),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /**
     * @brief RX.GET_BAND_ACTIVITY: Returns recent band activity details.
     * Includes frequency, offset, text, SNR, and UTC timestamp for each entry.
     */
    if (type == "RX.GET_BAND_ACTIVITY") {
        QVariantMap offsets = {
            {"_ID", id},
        };
        for (auto const [offset, activity] : m_bandActivity.asKeyValueRange()) {
            if (activity.isEmpty())
                continue;

            auto const d = activity.last();

            offsets[QString("%1").arg(offset)] = QVariant(QVariantMap{
                {"FREQ", QVariant(d.dial + d.offset)},
                {"DIAL", QVariant(d.dial)},
                {"OFFSET", QVariant(d.offset)},
                {"TEXT", QVariant(d.text)},
                {"SNR", QVariant(d.snr)},
                {"UTC", QVariant(d.utcTimestamp.toMSecsSinceEpoch())}});
        }

        sendNetworkMessage("RX.BAND_ACTIVITY", "", offsets);
        return;
    }
    /** @brief RX.GET_TEXT: Retrieves the current RX text buffer. */
    if (type == "RX.GET_TEXT") { /** RX.GET_TEXT */
        sendNetworkMessage("RX.TEXT", ui->textEditRX->toPlainText().right(1024),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @} */ // End RX Commands

    // TX.GET_TEXT
    // TX.SET_TEXT
    // TX.SEND_MESSAGE
    /**
     * @name TX Commands
     * @{
     */
    /** @brief TX.GET_TEXT: Retrieves the current TX text buffer. */
    if (type == "TX.GET_TEXT") {
        sendNetworkMessage("TX.TEXT",
                           ui->extFreeTextMsgEdit->toPlainText().right(1024),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief TX.SET_TEXT: Updates the TX text buffer with new content. */
    if (type == "TX.SET_TEXT") {
        addMessageText(message.value(), true);
        sendNetworkMessage("TX.TEXT",
                           ui->extFreeTextMsgEdit->toPlainText().right(1024),
                           {
                               {"_ID", id},
                           });
        return;
    }
    /** @brief TX.SEND_MESSAGE: Enqueues a message for transmission. */
    if (type == "TX.SEND_MESSAGE") {
        auto text = message.value();
        if (!text.isEmpty()) {
            enqueueMessage(PriorityNormal, text, -1, nullptr);
            processTxQueue();
            return;
        }
    }
    /** @} */ // End TX Commands

    // MODE.GET_SPEED
    // MODE.SET_SPEED
    /**
     * @name MODE Commands
     * @{
     */
    /** @brief MODE.GET_SPEED: Retrieves the current transmission speed mode. */
    if (type == "MODE.GET_SPEED") {
        sendNetworkMessage("MODE.SPEED", "",
                           {
                               {"_ID", id},
                               {"SPEED", m_nSubMode},
                           });
        return;
    }
    /** @brief MODE.SET_SPEED: Updates the transmission speed mode. */
    if (type == "MODE.SET_SPEED") {
        auto ok = false;
        auto const speed =
            message.params().value("SPEED", QVariant(m_nSubMode)).toInt(&ok);
        if (ok) {
            if (speed == Varicode::JS8CallNormal)
                ui->actionModeJS8Normal->setChecked(true);
            else if (speed == Varicode::JS8CallFast)
                ui->actionModeJS8Fast->setChecked(true);
            else if (speed == Varicode::JS8CallTurbo)
                ui->actionModeJS8Turbo->setChecked(true);
            else if (speed == Varicode::JS8CallSlow)
                ui->actionModeJS8Slow->setChecked(true);
            else if (speed == Varicode::JS8CallUltra)
                ui->actionModeJS8Ultra->setChecked(true);
            setupJS8();
        }
        sendNetworkMessage("MODE.SPEED", "",
                           {
                               {"_ID", id},
                               {"SPEED", m_nSubMode},
                           });
        return;
    }
    /** @} */ // End MODE Commands

    // INBOX.GET_MESSAGES
    // INBOX.STORE_MESSAGE
    /**
     * @name INBOX Commands
     * @{
     */
    /** @brief INBOX.GET_MESSAGES: Retrieves messages for a specified callsign.
     */
    if (type == "INBOX.GET_MESSAGES") {
        QString selectedCall =
            message.params().value("CALLSIGN", "").toString();
        if (selectedCall.isEmpty()) {
            selectedCall = "%";
        }

        Inbox inbox(inboxPath());
        if (!inbox.open()) {
            return;
        }

        QList<QPair<int, Message>> msgs;
        msgs.append(
            inbox.values("STORE", "$.params.TO", selectedCall, 0, 1000));
        msgs.append(
            inbox.values("READ", "$.params.FROM", selectedCall, 0, 1000));
        foreach (auto pair, inbox.values("UNREAD", "$.params.FROM",
                                         selectedCall, 0, 1000)) {
            msgs.append(pair);
        }
        std::stable_sort(
            msgs.begin(), msgs.end(),
            [](QPair<int, Message> const &a, QPair<int, Message> const &b) {
                return QVariant::compare(a.second.params().value("UTC"),
                                         b.second.params().value("UTC")) ==
                       QPartialOrdering::Greater;
            });

        QVariantList l;
        foreach (auto pair, msgs) {
            l << pair.second.toVariantMap();
        }

        sendNetworkMessage("INBOX.MESSAGES", "",
                           {
                               {"_ID", id},
                               {"MESSAGES", l},
                           });
        return;
    }
    /** @brief INBOX.STORE_MESSAGE: Stores a message in the inbox for a
     * callsign. */
    if (type == "INBOX.STORE_MESSAGE") {
        QString selectedCall =
            message.params().value("CALLSIGN", "").toString();
        if (selectedCall.isEmpty()) {
            return;
        }

        QString text = message.params().value("TEXT", "").toString();
        if (text.isEmpty()) {
            return;
        }

        CommandDetail d = {};
        d.cmd = " MSG ";
        d.to = selectedCall;
        d.from = m_config.my_callsign();
        d.relayPath = d.from;
        d.text = text;
        d.utcTimestamp = DriftingDateTime::currentDateTimeUtc();
        d.submode = m_nSubMode;

        auto mid = addCommandToStorage("STORE", d);

        sendNetworkMessage("INBOX.MESSAGE", "",
                           {
                               {"_ID", id},
                               {"ID", mid},
                           });
        return;
    }
    /** @} */ // End INBOX Commands

    // WINDOW.RAISE
    /**
     * @name WINDOW Commands
     * @{
     */
    /** @brief WINDOW.RAISE: Brings the main application window to the
     * foreground. */
    if (type == "WINDOW.RAISE") {
        setWindowState(Qt::WindowActive);
        activateWindow();
        raise();
        return;
    }

    qCDebug(mainwindow_js8) << "Unable to process networkMessage:" << type;
}
/** @} */ // end of WINDOW Commands
/** @} */ // end of API
