#ifndef TRANSMITTEXTEDIT_H
#define TRANSMITTEXTEDIT_H

#include "qt_helpers.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

void setTextEditFont(QTextEdit *edit, QFont font);
void setTextEditStyle(QTextEdit *edit, QColor fg, QColor bg, QFont font);
void highlightBlock(QTextBlock block, QFont font, QColor foreground,
                    QColor background);

class TransmitTextEdit : public QTextEdit {
  public:
    TransmitTextEdit(QWidget *parent);

    static QPair<int, int> relativeTextCursorPosition(QTextCursor cursor) {
        auto c = QTextCursor(cursor);
        c.movePosition(QTextCursor::End);
        int last = c.position();

        auto cc = QTextCursor(cursor);
        int relstart = last - qMin(cc.selectionStart(), cc.selectionEnd());
        int relend = last - qMax(cc.selectionStart(), cc.selectionEnd());

        return {relstart, relend};
    }

    int charsSent() const { return m_sent; }
    void setCharsSent(int n);

    QString sentText() const { return m_textSent; }

    QString unsentText() const { return toPlainText().mid(charsSent()); }

    QString toPlainText() const;
    void setPlainText(const QString &text);
    void replaceUnsentText(const QString &text, bool keepCursor);
    void replacePlainText(const QString &text, bool keepCursor);

    void setFont(QFont f);
    void setFont(QFont f, QColor fg, QColor bg);
    void clear();

    bool isProtected() const { return m_protected; }
    void setProtected(bool protect);
    bool cursorShouldBeProtected(QTextCursor c);

    bool isEmpty() const { return toPlainText().isEmpty(); }
    bool isDirty() const { return m_dirty; }
    void setClean() { m_dirty = false; }

    void highlightBase();
    void highlightCharsSent();
    void highlight();

    bool eventFilter(QObject * /*o*/, QEvent *e);

  public slots:
    void on_selectionChanged();
    void on_textContentsChanged(int pos, int rem, int add);

  private:
    QString m_lastText;
    int m_sent;
    QString m_textSent;
    bool m_protected;
    bool m_dirty;
    QFont m_font;
    QColor m_fg;
    QColor m_bg;
};

#endif // TRANSMITTEXTEDIT_H
