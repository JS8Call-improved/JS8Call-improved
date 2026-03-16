#ifndef TRANSMITTEXTEDIT_H
#define TRANSMITTEXTEDIT_H

#include "JS8_Include/pimpl_h.h"
#include "qt_helpers.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

class PillRenderer;
class ScopedDocumentMutation;

void setTextEditFont(QTextEdit *edit, QFont font);
void setTextEditStyle(QTextEdit *edit, QColor fg, QColor bg, QFont font);
void highlightBlock(QTextBlock block, QFont font, QColor foreground,
                    QColor background);

class TransmitTextEdit : public QTextEdit {
    friend class PillRenderer;
    friend class ScopedDocumentMutation;

  public:
    explicit TransmitTextEdit(QWidget *parent);
    ~TransmitTextEdit();

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
    void undo();
    void redo();

    void setFont(QFont f);
    void setFont(QFont f, QColor fg, QColor bg);
    void clear();

    bool isProtected() const { return m_protected; }
    void setProtected(bool protect);
    bool cursorShouldBeProtected(QTextCursor c);

    bool isEmpty() const { return toPlainText().isEmpty(); }
    bool isDirty() const { return m_dirty; }
    void setClean() { m_dirty = false; }

    PillRenderer *pillRenderer() const { return m_pillRenderer; }

    void highlightBase();
    void highlightCharsSent();
    void highlight();

    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *e) override;
    bool eventFilter(QObject * /*o*/, QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

  public slots:
    void on_selectionChanged();
    void on_textContentsChanged(int pos, int rem, int add);

  private:
    class source_mirror;

    void beginInternalDocumentMutation();
    void endInternalDocumentMutation();
    bool isInternalDocumentMutationActive() const;

    QString m_lastText;
    int m_sent;
    QString m_textSent;
    bool m_protected;
    bool m_dirty = false;
    QFont m_font;
    QColor m_fg;
    QColor m_bg;
    PillRenderer *m_pillRenderer;
    bool m_isHighlighting = false;
    pimpl<source_mirror> m_sourceMirror;
};

#endif // TRANSMITTEXTEDIT_H
