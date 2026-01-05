#include "JS8_UI/mainwindow.h"

/** \file
 * @brief member function of the MainWindow class
 *  queries the GitHub release assets, trigged from the help menu to determine
 *  if the software version is the latest
 */

void MainWindow::checkVersion(bool const alertOnUpToDate) {
    auto m = new QNetworkAccessManager(this);
    connect(m, &QNetworkAccessManager::finished, this,
            [this, alertOnUpToDate](QNetworkReply *reply) {
                if (reply->error()) {
                    qCDebug(mainwindow_js8) << "Checking for Updates Error:"
                                            << reply->errorString();
                    return;
                }

                QString content = reply->readAll().trimmed();

                auto const currentVersion =
                    QVersionNumber::fromString(version());
                auto const networkVersion = QVersionNumber::fromString(content);

                qCDebug(mainwindow_js8) << "Checking Version" << currentVersion
                                        << "with" << networkVersion;

                if (currentVersion < networkVersion) {

                    SelfDestructMessageBox *m = new SelfDestructMessageBox(
                        60, "New Updates Available",
                        QString("A new version (%1) of JS8Call is now "
                                "available. Please see the <a "
                                "href='https://github.com/JS8Call-improved/"
                                "JS8Call-improved/releases'>GitHub "
                                "Releases</a> for more details.")
                            .arg(content),
                        QMessageBox::Information, QMessageBox::Ok,
                        QMessageBox::Ok, false, this);

                    m->show();

                } else if (alertOnUpToDate) {

                    SelfDestructMessageBox *m = new SelfDestructMessageBox(
                        60, "No Updates Available",
                        QString("Your version (%1) of JS8Call is up-to-date.")
                            .arg(version()),
                        QMessageBox::Information, QMessageBox::Ok,
                        QMessageBox::Ok, false, this);

                    m->show();
                }
            });

    qCDebug(mainwindow_js8) << "Checking for Updates...";
    QUrl url("https://github.com/JS8Call-improved/JS8Call-improved/releases/"
             "latest/download/version.txt");
    QNetworkRequest r(url);
    m->get(r);
}
