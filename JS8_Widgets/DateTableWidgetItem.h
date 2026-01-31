#ifndef JS8CALL_DATETABLEWIDGETITEM_H
#define JS8CALL_DATETABLEWIDGETITEM_H

#include <QTableWidgetItem>

class DateItem : public QTableWidgetItem {
public:
  using QTableWidgetItem::QTableWidgetItem;

  bool operator<(const QTableWidgetItem &other) const override {
    return data(Qt::UserRole).toLongLong() < other.data(Qt::UserRole).toLongLong();
  }
};
#endif // JS8CALL_DATETABLEWIDGETITEM_H
