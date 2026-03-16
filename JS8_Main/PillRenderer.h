/**
 * @file PillRenderer.h
 * @brief Pill-shaped visual highlights for directed message tokens.
 */

#ifndef PILLRENDERER_H
#define PILLRENDERER_H

#include <QObject>
#include <QString>
#include <QTextBlockFormat>

class QHelpEvent;
class QPaintEvent;
class DirectedMessageHighlighter;
class TransmitTextEdit;
struct PillColorScheme;

/**
 * @class PillRenderer
 * @brief Renders pill-shaped backgrounds for a QTextEdit host.
 *
 * Owns a DirectedMessageHighlighter and drives it during each highlight cycle.
 * Paints rounded-rectangle pill backgrounds behind highlighted tokens, handles
 * tooltips on hover, and adjusts letter-spacing for proper pill padding.
 */
class PillRenderer : public QObject {
    Q_OBJECT

  public:
    // Pill geometry constants — used by paintPills() and applyPillHighlighting().
    static constexpr qreal PillHPad      = 8.0;   ///< Horizontal padding.
    static constexpr qreal PillVPad      = 3.0;   ///< Vertical padding.
    static constexpr qreal PillMaxRadius = 11.0;   ///< Maximum corner radius.
    static constexpr qreal PillMinRadius = 7.0;    ///< Minimum corner radius.
    static constexpr qreal PillMinHPad   = 5.0;   ///< Floor during overlap resolution.
    static constexpr qreal PillMinGap    = 5.0;    ///< Minimum gap between adjacent pills.

    /**
     * @brief Construct a PillRenderer attached to the given text edit.
     * @param host   TransmitTextEdit whose document is highlighted and whose
     *               viewport receives pill painting.
     * @param parent QObject parent (typically the host itself).
     */
    explicit PillRenderer(TransmitTextEdit *host, QObject *parent = nullptr);

    /**
     * @brief Paint pill backgrounds behind highlighted tokens.
     *        Call *before* the base-class paintEvent so pills appear behind text.
     */
    void paintPills(QPaintEvent *event);

    /**
     * @brief Handle a tooltip event by hit-testing pill format ranges.
     * @return true if a tooltip was shown, false otherwise.
     */
    bool handleTooltip(QHelpEvent *helpEvent);

    /**
     * @brief Run the syntax highlighter and adjust letter-spacing around pills.
     */
    void applyPillHighlighting();

    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setPillColors(const PillColorScheme &scheme);
    void setSelectedCallsign(const QString &callsign);

  private:
    void refreshPillState();
    void applyLineHeight(bool enabled) const;
    void clearPillLetterSpacing() const;
    void applyPillLetterSpacing() const;

    TransmitTextEdit *m_host;
    DirectedMessageHighlighter *m_highlighter;
    qreal m_defaultDocumentMargin = 0.0;
    QTextBlockFormat m_defaultBlockFormat;
    bool m_enabled = true;
    QString m_selectedCallsign;
};

#endif // PILLRENDERER_H
