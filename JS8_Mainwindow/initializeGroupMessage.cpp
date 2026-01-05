#include "JS8_UI/mainwindow.h"

/** \file
 * @brief member function of the MainWindow class
 *  group messaging function
 */

void MainWindow::initializeGroupMessage() {
    if (!QApplication::applicationName().contains("dummy")) {
        return;
    }

    if (!m_config.my_groups().contains("@GROUP42")) {
        m_config.addGroup("@GROUP42");
    }

    auto dt = DriftingDateTime::currentDateTimeUtc().addSecs(-300);

    CommandDetail cmd = {};
    cmd.cmd = " MSG TO:";
    cmd.from = "KN4CRD";
    cmd.to = "@GROUP42";
    cmd.utcTimestamp = dt;
    cmd.submode = Varicode::JS8CallNormal;
    cmd.text = "@GROUP42 TEST MESSAGE TO GROUP";

    m_rxCommandQueue.append(cmd);

    processCommandActivity();

    dt = DriftingDateTime::currentDateTimeUtc().addSecs(-150);

    CommandDetail cmd1 = {};
    cmd1.cmd = " MSG TO:";
    cmd1.from = "KN4CRD";
    cmd1.to = "@GROUP42";
    cmd1.utcTimestamp = dt;
    cmd1.submode = Varicode::JS8CallNormal;
    cmd1.text = "@GROUP42 ANOTHER TEST MESSAGE TO GROUP";

    m_rxCommandQueue.append(cmd1);

    processCommandActivity();

    CommandDetail cmd2 = {};
    cmd2.cmd = " QUERY MSGS";
    cmd2.from = "W1AW";
    cmd2.to = "@GROUP42";
    cmd2.utcTimestamp = DriftingDateTime::currentDateTimeUtc();
    cmd2.submode = Varicode::JS8CallNormal;

    m_rxCommandQueue.append(cmd2);

    processCommandActivity();

    int mid = getNextGroupMessageIdForCallsign("@GROUP42", "W1AW");

    qCDebug(mainwindow_js8) << "Testing group messaging";
    qCDebug(mainwindow_js8) << "Test message ID: " << mid;

    std::string textString = "MSG ";
    textString += std::to_string(mid);

    qCDebug(mainwindow_js8) << "Text string: " << textString.c_str();

    CommandDetail cmd3 = {};
    cmd3.cmd = " QUERY";
    cmd3.from = "W1AW";
    cmd3.to = "@GROUP42";
    cmd3.utcTimestamp = DriftingDateTime::currentDateTimeUtc();
    cmd3.submode = Varicode::JS8CallNormal;
    cmd3.text = textString.c_str();

    m_rxCommandQueue.append(cmd3);

    processCommandActivity();

    dt = DriftingDateTime::currentDateTimeUtc().addSecs(-300);

    CommandDetail cmd4 = {};
    cmd4.cmd = " MSG TO:";
    cmd4.from = "KN4CRD";
    cmd4.to = "K4RWR";
    cmd4.utcTimestamp = dt;
    cmd4.submode = Varicode::JS8CallNormal;
    cmd4.text = "W1AW TEST MESSAGE TO STATION";

    m_rxCommandQueue.append(cmd4);

    processCommandActivity();

    dt = DriftingDateTime::currentDateTimeUtc().addSecs(-150);

    CommandDetail cmd5 = {};
    cmd5.cmd = " MSG TO:";
    cmd5.from = "KN4CRD";
    cmd5.to = "K4RWR";
    cmd5.utcTimestamp = dt;
    cmd5.submode = Varicode::JS8CallNormal;
    cmd5.text = "W1AW ANOTHER TEST MESSAGE TO STATION";

    m_rxCommandQueue.append(cmd5);

    processCommandActivity();

    CommandDetail cmd6 = {};
    cmd6.cmd = " QUERY MSGS";
    cmd6.from = "W1AW";
    cmd6.to = "K4RWR";
    cmd6.utcTimestamp = DriftingDateTime::currentDateTimeUtc();
    cmd6.submode = Varicode::JS8CallNormal;

    m_rxCommandQueue.append(cmd6);

    processCommandActivity();

    mid = getNextMessageIdForCallsign("W1AW");

    qCDebug(mainwindow_js8) << "Testing group messaging";
    qCDebug(mainwindow_js8) << "Test message ID: " << mid;

    textString = "MSG ";
    textString += std::to_string(mid);

    qCDebug(mainwindow_js8) << "Text string: " << textString.c_str();

    CommandDetail cmd7 = {};
    cmd7.cmd = " QUERY";
    cmd7.from = "W1AW";
    cmd7.to = "K4RWR";
    cmd7.utcTimestamp = DriftingDateTime::currentDateTimeUtc();
    cmd7.submode = Varicode::JS8CallNormal;
    cmd7.text = textString.c_str();

    m_rxCommandQueue.append(cmd7);

    processCommandActivity();

    displayActivity(true);
}
