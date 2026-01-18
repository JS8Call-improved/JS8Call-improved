/**
 * @file HelpTextWindow.cpp
 * @brief Implementation of HelpTextWindow for displaying help text files
 */
#include "HelpTextWindow.h"
#include "JS8MessageBox.h"
#include "qt_helpers.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QString>
#include <QTextStream>

/**
 * @brief Construct a new HelpTextWindow object
 *
 * @param title Title of the help window
 * @param file_name Path to the help text file
 * @param font Font to use for displaying the help text
 * @param parent Parent widget
 */
HelpTextWindow::HelpTextWindow(QString const &title, QString const &file_name,
                               QFont const &font, QWidget *parent)
    : QLabel{parent, Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint} {
    QFile source{file_name};
    if (!source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        JS8MessageBox::warning_message(
            this, tr("Help file error"),
            tr("Cannot open \"%1\" for reading").arg(source.fileName()),
            tr("Error: %1").arg(source.errorString()));
        return;
    }
    setText(QTextStream{&source}.readAll());
    setWindowTitle(QApplication::applicationName() + " - " + title);
    setMargin(10);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setStyleSheet(font_as_stylesheet(font));
}
