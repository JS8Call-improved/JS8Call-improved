#ifndef DIRECTEDMESSAGEHIGHLIGHTER_H
#define DIRECTEDMESSAGEHIGHLIGHTER_H

#include <QColor>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextFormat>

/// Background and foreground colors for each pill category.
struct PillColorScheme {
    QColor recipientBg, recipientFg;
    QColor commandBg, commandFg;
    QColor groupBg, groupFg;
    QColor senderBg, senderFg;
};

class DirectedMessageHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
  public:
    explicit DirectedMessageHighlighter(QTextDocument *parent = nullptr);

    void setEnabled(bool enabled);
    void setPillColors(const PillColorScheme &scheme);
    void setSelectedCallsign(const QString &callsign);

    // Custom QTextFormat properties for pill identification.
    static constexpr int PillColorProperty = QTextFormat::UserProperty + 1;
    static constexpr int PillGroupProperty = QTextFormat::UserProperty + 2;

  protected:
    void highlightBlock(const QString &text) override;

  private:
    QTextCharFormat makePillFormat(QColor pillColor, QColor fg, QColor bg,
                                   int pillGroup, const QString &tooltip,
                                   bool bold = false) const;

    QColor m_addressColor;      // blue — recipient + relay chains
    QColor m_addressFontColor;  // font color for recipient pills
    QColor m_commandColor;      // orange — commands + MSG TO: storage
    QColor m_commandFontColor;  // font color for command pills
    QColor m_groupColor;        // teal — @GROUP
    QColor m_groupFontColor;    // font color for group pills
    QColor m_senderColor;       // grey/slate — sender callsign prefix
    QColor m_senderFontColor;   // font color for sender pills

    QString m_selectedCallsign;
    bool m_enabled = true;
};

#endif // DIRECTEDMESSAGEHIGHLIGHTER_H
