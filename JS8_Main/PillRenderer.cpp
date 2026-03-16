/**
 * @file PillRenderer.cpp
 * @brief Implementation of PillRenderer — pill painting, tooltips,
 *        and letter-spacing.
 */

#include "PillRenderer.h"
#include "DirectedMessageHighlighter.h"
#include "TransmitTextEdit.h"

#include <QAbstractTextDocumentLayout>
#include <QHelpEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFragment>
#include <QTextLayout>
#include <QToolTip>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PillRenderer::PillRenderer(TransmitTextEdit *host, QObject *parent)
    : QObject(parent), m_host(host) {
    m_highlighter = new DirectedMessageHighlighter(host->document());
    m_defaultDocumentMargin = host->document()->documentMargin();
    m_defaultBlockFormat = host->document()->firstBlock().blockFormat();

    {
        QSignalBlocker blocker(host->document());
        host->beginInternalDocumentMutation();

        // Margin >= PillHPad so the first pill's left padding doesn't clip.
        host->document()->setDocumentMargin(PillRenderer::PillHPad);

        // Line height 140% gives pills vertical breathing room.
        QTextCursor cursor(host->document());
        cursor.select(QTextCursor::Document);
        QTextBlockFormat blockFmt;
        blockFmt.setLineHeight(140, QTextBlockFormat::ProportionalHeight);
        cursor.mergeBlockFormat(blockFmt);

        host->endInternalDocumentMutation();
    }
}

// ---------------------------------------------------------------------------
// Pill painting
// ---------------------------------------------------------------------------

void PillRenderer::paintPills(QPaintEvent * /*event*/) {
    if (!m_enabled)
        return;

    QPainter painter(m_host->viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    auto *doc = m_host->document();
    auto *docLayout = doc->documentLayout();
    QPointF offset(-m_host->horizontalScrollBar()->value(),
                   -m_host->verticalScrollBar()->value());

    QFontMetricsF fm(doc->defaultFont());
    qreal capH = fm.capHeight();

    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        auto *layout = block.layout();
        if (layout) {
            QRectF blockRect = docLayout->blockBoundingRect(block);
            blockRect.translate(offset);

            // Group pill rectangles by pill ID within each visual line.
            struct PillInfo {
                QColor color;
                QRectF rect;
                int lineIndex = 0;
            };
            QHash<quint64, PillInfo> pillGroups;

            auto pillFragmentKey = [](int groupId, int lineIndex) {
                quint64 pillId = static_cast<quint64>(
                    static_cast<quint32>(groupId));
                return (pillId << 32) | static_cast<quint32>(lineIndex);
            };

            for (const auto &range : layout->formats()) {
                auto colorVar = range.format.property(
                    DirectedMessageHighlighter::PillColorProperty);
                if (!colorVar.isValid())
                    continue;

                auto groupVar = range.format.property(
                    DirectedMessageHighlighter::PillGroupProperty);
                int groupId = groupVar.isValid() ? groupVar.toInt() : -1;

                QColor pillColor = colorVar.value<QColor>();
                int rangeStart = range.start;
                int rangeLen = range.length;

                for (int li = 0; li < layout->lineCount(); ++li) {
                    QTextLine line = layout->lineAt(li);
                    if (rangeStart >= line.textStart() + line.textLength())
                        continue;
                    if (rangeStart + rangeLen <= line.textStart())
                        break;

                    int lineRangeStart = qMax(rangeStart, line.textStart());
                    int lineRangeEnd = qMin(
                        rangeStart + rangeLen,
                        line.textStart() + line.textLength());

                    qreal x1 =
                        qRound(line.cursorToX(lineRangeStart) * 2.0) / 2.0;
                    qreal x2 =
                        qRound(line.cursorToX(lineRangeEnd) * 2.0) / 2.0;
                    QRectF lineRect = line.rect();

                    // Bias vertical centering toward cap-height; 0.35 blends
                    // between tight cap-height and full line-height.
                    qreal baseline = lineRect.y() + line.ascent();
                    qreal extra = (lineRect.height() - capH) * 0.35;
                    qreal textTop = baseline - capH - extra;
                    qreal textH = capH + 2.0 * extra;

                    QRectF rangeRect(blockRect.x() + qMin(x1, x2),
                                     blockRect.y() + textTop, qAbs(x2 - x1),
                                     textH);

                    quint64 fragmentKey =
                        pillFragmentKey(groupId >= 0 ? groupId : rangeStart,
                                        li);

                    if (pillGroups.contains(fragmentKey)) {
                        pillGroups[fragmentKey].rect =
                            pillGroups[fragmentKey].rect.united(rangeRect);
                    } else {
                        pillGroups[fragmentKey] = {pillColor, rangeRect, li};
                    }
                }
            }

            // Add padding around each pill fragment and keep overlap handling
            // scoped to the visual line where it is drawn.
            struct DrawnPill {
                QRectF baseRect;
                QRectF rect;
                QColor color;
            };
            QHash<int, QList<DrawnPill>> pillsByLine;
            for (auto it = pillGroups.constBegin(); it != pillGroups.constEnd();
                 ++it) {
                DrawnPill pill;
                pill.baseRect = it.value().rect;
                pill.rect = pill.baseRect.adjusted(
                    -PillRenderer::PillHPad,
                    -PillRenderer::PillVPad,
                    PillRenderer::PillHPad,
                    PillRenderer::PillVPad);
                pill.color = it.value().color;
                pillsByLine[it.value().lineIndex].append(pill);
            }

            QList<int> lineIndexes = pillsByLine.keys();
            std::sort(lineIndexes.begin(), lineIndexes.end());

            for (int lineIndex : lineIndexes) {
                auto &pills = pillsByLine[lineIndex];

                std::sort(pills.begin(), pills.end(),
                          [](const DrawnPill &a, const DrawnPill &b) {
                              return a.rect.left() < b.rect.left();
                          });

                // Shrink overlapping padding symmetrically, clamped to
                // PillMinHPad, only among pills that share this visual line.
                for (int i = 0; i + 1 < pills.size(); ++i) {
                    qreal overlap =
                        pills[i].rect.right() - pills[i + 1].rect.left() +
                        PillRenderer::PillMinGap;
                    if (overlap > 0) {
                        qreal shrink = overlap / 2.0;
                        pills[i].rect.setRight(
                            qMax(pills[i].rect.right() - shrink,
                                 pills[i].baseRect.right() +
                                     PillRenderer::PillMinHPad));
                        pills[i + 1].rect.setLeft(
                            qMin(pills[i + 1].rect.left() + shrink,
                                 pills[i + 1].baseRect.left() -
                                     PillRenderer::PillMinHPad));
                    }
                }

                for (auto &pill : pills) {
                    if (pill.rect.left() < 1.0)
                        pill.rect.setLeft(1.0);
                }

                for (const auto &pill : pills) {
                    qreal halfHeight = pill.rect.height() / 2.0;
                    qreal r = qMin(PillRenderer::PillMaxRadius, halfHeight);
                    r = qMax(r, PillRenderer::PillMinRadius);

                    painter.setBrush(pill.color);
                    painter.drawRoundedRect(pill.rect, r, r);
                }
            }
        }
        block = block.next();
    }
}

// ---------------------------------------------------------------------------
// Tooltip handling
// ---------------------------------------------------------------------------

bool PillRenderer::handleTooltip(QHelpEvent *helpEvent) {
    if (!m_enabled)
        return false;

    auto *doc = m_host->document();
    QPointF pos = helpEvent->pos();
    int charPos = doc->documentLayout()->hitTest(
        pos + QPointF(m_host->horizontalScrollBar()->value(),
                      m_host->verticalScrollBar()->value()),
        Qt::ExactHit);

    if (charPos >= 0) {
        QTextBlock block = doc->findBlock(charPos);
        if (block.isValid() && block.layout()) {
            int posInBlock = charPos - block.position();
            for (const auto &range : block.layout()->formats()) {
                if (posInBlock >= range.start &&
                    posInBlock < range.start + range.length) {
                    QString tip = range.format.toolTip();
                    if (!tip.isEmpty()) {
                        QToolTip::showText(helpEvent->globalPos(), tip,
                                           m_host);
                        return true;
                    }
                }
            }
        }
    }
    QToolTip::hideText();
    return false;
}

// ---------------------------------------------------------------------------
// Highlighting + letter-spacing
// ---------------------------------------------------------------------------

void PillRenderer::applyPillHighlighting() {
    refreshPillState();
}

// ---------------------------------------------------------------------------
// Enable / disable
// ---------------------------------------------------------------------------

void PillRenderer::setEnabled(bool enabled) {
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    m_highlighter->setEnabled(enabled);
    m_host->highlight();
}

void PillRenderer::setPillColors(const PillColorScheme &scheme) {
    m_highlighter->setPillColors(scheme);
}

void PillRenderer::setSelectedCallsign(const QString &callsign) {
    if (callsign.compare(m_selectedCallsign, Qt::CaseInsensitive) == 0)
        return;

    m_selectedCallsign = callsign.toUpper();
    m_highlighter->setSelectedCallsign(callsign);
    m_host->highlight();
}

void PillRenderer::refreshPillState() {
    auto *doc = m_host->document();
    if (!doc)
        return;

    Q_ASSERT(m_host->isInternalDocumentMutationActive());

    doc->setDocumentMargin(m_enabled ? PillRenderer::PillHPad
                                     : m_defaultDocumentMargin);
    applyLineHeight(m_enabled);
    m_highlighter->rehighlight();
    clearPillLetterSpacing();
    if (m_enabled)
        applyPillLetterSpacing();

    m_host->viewport()->update();
}

void PillRenderer::applyLineHeight(bool enabled) const {
    QTextBlock block = m_host->document()->firstBlock();
    while (block.isValid()) {
        QTextBlockFormat blockFmt = block.blockFormat();
        const int targetHeight =
            enabled ? 140 : m_defaultBlockFormat.lineHeight();
        const auto targetType = enabled ? QTextBlockFormat::ProportionalHeight
                                        : m_defaultBlockFormat.lineHeightType();

        if (blockFmt.lineHeight() != targetHeight ||
            blockFmt.lineHeightType() != targetType) {
            blockFmt.setLineHeight(targetHeight, targetType);
            QTextCursor cursor(block);
            cursor.setBlockFormat(blockFmt);
        }
        block = block.next();
    }
}

void PillRenderer::clearPillLetterSpacing() const {
    auto *doc = m_host->document();
    const qreal interPillMargin =
        2.0 * PillRenderer::PillHPad + PillRenderer::PillMinGap;
    const auto isPillSpacing = [interPillMargin](const QTextCharFormat &fmt) {
        if (fmt.fontLetterSpacingType() != QFont::AbsoluteSpacing)
            return false;
        return qFuzzyCompare(fmt.fontLetterSpacing() + 1.0,
                             PillRenderer::PillHPad + 1.0) ||
               qFuzzyCompare(fmt.fontLetterSpacing() + 1.0,
                             interPillMargin + 1.0);
    };

    const QFont defaultFont = doc->defaultFont();
    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;

            const QTextCharFormat currentFmt = fragment.charFormat();
            if (!isPillSpacing(currentFmt))
                continue;

            QTextCursor cursor(doc);
            cursor.setPosition(fragment.position());
            cursor.setPosition(fragment.position() + fragment.length(),
                               QTextCursor::KeepAnchor);

            QTextCharFormat resetFmt;
            resetFmt.setFontLetterSpacing(defaultFont.letterSpacing());
            resetFmt.setFontLetterSpacingType(defaultFont.letterSpacingType());
            cursor.mergeCharFormat(resetFmt);
        }
        block = block.next();
    }
}

void PillRenderer::applyPillLetterSpacing() const {
    auto *doc = m_host->document();
    const qreal pillMargin = PillRenderer::PillHPad;
    const qreal interPillMargin =
        2.0 * PillRenderer::PillHPad + PillRenderer::PillMinGap;

    struct SpaceInfo {
        int absPos;
        qreal spacing;
    };
    QList<SpaceInfo> spacesToAdjust;

    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        QTextLayout *layout = block.layout();
        if (layout) {
            QHash<int, int> groupEnds;
            QSet<int> pillStarts;
            for (const auto &range : layout->formats()) {
                auto colorVar = range.format.property(
                    DirectedMessageHighlighter::PillColorProperty);
                if (!colorVar.isValid())
                    continue;
                auto groupVar = range.format.property(
                    DirectedMessageHighlighter::PillGroupProperty);
                int groupId = groupVar.isValid() ? groupVar.toInt() : -1;
                int end = range.start + range.length;
                if (!groupEnds.contains(groupId) || end > groupEnds[groupId])
                    groupEnds[groupId] = end;
                pillStarts.insert(range.start);
            }

            const QString text = block.text();
            for (auto it = groupEnds.constBegin(); it != groupEnds.constEnd();
                 ++it) {
                const int end = it.value();
                if (end < text.length() && text.at(end) == QLatin1Char(' ')) {
                    const bool nextIsPill = pillStarts.contains(end + 1);
                    const qreal spacing =
                        nextIsPill ? interPillMargin : pillMargin;
                    spacesToAdjust.append({block.position() + end, spacing});
                }
            }
        }
        block = block.next();
    }

    for (const auto &si : spacesToAdjust) {
        QTextCursor cursor(doc);
        cursor.setPosition(si.absPos);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor);
        QTextCharFormat fmt;
        fmt.setFontLetterSpacing(si.spacing);
        fmt.setFontLetterSpacingType(QFont::AbsoluteSpacing);
        cursor.mergeCharFormat(fmt);
    }
}
