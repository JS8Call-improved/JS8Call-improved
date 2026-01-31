/**
 * \file MessageWindow.cpp
 * @brief implementation of the message window dialog for the UI
 */

#include "MessageWindow.h"
#include "ui_MessageWindow.h"

MessageWindow::MessageWindow(QWidget *parent)
    : QDialog(parent), ui(new Ui::MessagePanel) {
    ui->setupUi(this);
}

MessageWindow::~MessageWindow() { delete ui; }

