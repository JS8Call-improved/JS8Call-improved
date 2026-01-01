#ifndef HELP_TEXT_WINDOW_HPP__
#define HELP_TEXT_WINDOW_HPP__

#include <QFont>
#include <QLabel>

class QString;

class HelpTextWindow final : public QLabel {
  public:
    HelpTextWindow(QString const &title, QString const &file_name,
                   QFont const & = QFont{}, QWidget *parent = nullptr);
};

#endif
