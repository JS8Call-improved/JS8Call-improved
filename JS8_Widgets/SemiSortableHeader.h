#ifndef JS8CALL_SEMISORTABLEHEADER_H
#define JS8CALL_SEMISORTABLEHEADER_H

#include <QHeaderView>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QSet>
#include <QStyleOptionHeader>
#include <QTableWidget>
#include <QVariant>

//
// Class SemiSortableHeader
//
// QHeaderView derivative that allow selective disabling of sorting on specified columns.
//
class SemiSortableHeader : public QHeaderView {

public:
  explicit SemiSortableHeader(Qt::Orientation ori, QWidget *parent = nullptr)
      : QHeaderView(ori, parent) {
    setSectionsClickable(true);
    setSortIndicatorShown(true);
    setSortIndicatorClearable(false);
    setMouseTracking(true);
  }

  void attachTo(QTableWidget *table) {
    table_ = table;
    table_->setHorizontalHeader(this);

    // Keep sorting enabled so the style draws the indicator
    table_->setSortingEnabled(true);

    // Initialize to a sensible default: first sortable column, descending
    sortCol_ = firstSortableSection();
    sortOrder_ = Qt::DescendingOrder;

    if (sortCol_ >= 0) {
      QSignalBlocker b(this);
      setSortIndicator(sortCol_, sortOrder_);
    }

    connect(this, &QHeaderView::sectionClicked, this,
            &SemiSortableHeader::onSectionClicked, Qt::UniqueConnection);
  }

  // New multi-column API
  void setNonSortableColumns(const QSet<int> &cols) {
    nonSortable_ = cols;
    normalizeSortIfNeeded();
    viewport()->update();
  }

  void addNonSortableColumn(int col) {
    if (col >= 0) {
      nonSortable_.insert(col);
      normalizeSortIfNeeded();
      viewport()->update();
    }
  }

  void removeNonSortableColumn(int col) {
    nonSortable_.remove(col);
    normalizeSortIfNeeded();
    viewport()->update();
  }

  bool isSortableColumn(int col) const {
    return col >= 0 && !nonSortable_.contains(col);
  }

private slots:
  void onSectionClicked(int column) {
    if (!table_)
      return;
    if (!isSortableColumn(column))
      return;

    if (column == sortCol_) {
      sortOrder_ = (sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder
                                                      : Qt::AscendingOrder;
    } else {
      sortCol_ = column;
      sortOrder_ = Qt::AscendingOrder;
    }

    // Update indicator without re-triggering click logic
    {
      QSignalBlocker b(this);
      setSortIndicator(sortCol_, sortOrder_);
    }

    // Prevent Qt's built-in sorting from running while we sort
    table_->setSortingEnabled(false);
    table_->sortItems(sortCol_, sortOrder_);
    table_->setSortingEnabled(true);
  }

protected:
  void paintSection(QPainter *painter, const QRect &rect,
                    int logicalIndex) const override {
    // Normal columns: keep Qt default painting
    if (isSortableColumn(logicalIndex)) {
      QHeaderView::paintSection(painter, rect, logicalIndex);
      return;
    }

    // Dead columns: custom paint (no hover/pressed, no sort arrow), but keep
    // label
    QStyleOptionHeader opt;
    initStyleOption(&opt);

    opt.rect = rect;
    opt.section = logicalIndex;

    if (auto *m = model()) {
      opt.text = m->headerData(logicalIndex, orientation(), Qt::DisplayRole)
                     .toString();

      const QVariant align =
          m->headerData(logicalIndex, orientation(), Qt::TextAlignmentRole);
      if (align.isValid())
        opt.textAlignment = Qt::Alignment(align.toInt());

      const QVariant deco =
          m->headerData(logicalIndex, orientation(), Qt::DecorationRole);
      if (deco.isValid())
        opt.icon = deco.value<QIcon>();
    }

    // Section position affects borders/separators
    const int v = visualIndex(logicalIndex);
    const int n = count();
    if (n <= 1)
      opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (v == 0)
      opt.position = QStyleOptionHeader::Beginning;
    else if (v == n - 1)
      opt.position = QStyleOptionHeader::End;
    else
      opt.position = QStyleOptionHeader::Middle;

    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Sunken;

    // Ensure no sort arrow is painted on dead columns
    opt.sortIndicator = QStyleOptionHeader::None;

    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
  }

  void mousePressEvent(QMouseEvent *e) override {
    const int col = logicalIndexAt(e->pos());
    if (!isSortableColumn(col)) {
      e->accept(); // swallow: no indicator change, no click signal
      return;
    }
    QHeaderView::mousePressEvent(e);
  }

private:
  int firstSortableSection() const {
    for (int logical = 0; logical < count(); ++logical) {
      if (isSortableColumn(logical))
        return logical;
    }
    return -1; // none sortable
  }

  void normalizeSortIfNeeded() {
    // If current sort column became non-sortable, move to first sortable
    if (sortCol_ >= 0 && !isSortableColumn(sortCol_)) {
      sortCol_ = firstSortableSection();
      sortOrder_ = Qt::DescendingOrder;
      if (sortCol_ >= 0) {
        QSignalBlocker b(this);
        setSortIndicator(sortCol_, sortOrder_);
      } else {
        QSignalBlocker b(this);
        setSortIndicator(-1, Qt::AscendingOrder); // clear indicator
      }
    }
  }

private:
  QPointer<QTableWidget> table_;
  QSet<int> nonSortable_;

  int sortCol_ = -1;
  Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
};

#endif // JS8CALL_SEMISORTABLEHEADER_H
