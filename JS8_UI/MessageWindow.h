#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "JS8_Main/Message.h"

#include <QDialog>

namespace Ui {
class MessagePanel;
}

class MessageWindow : public QDialog {
    Q_OBJECT

  public:
    explicit MessageWindow(QWidget *parent = 0);
    ~MessageWindow();

  private:
    Ui::MessagePanel *ui;
};

#endif // MESSAGEWINDOW_H
