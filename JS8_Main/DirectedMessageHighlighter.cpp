/**
 * @file DirectedMessageHighlighter.cpp
 * @brief Syntax highlighter for JS8 directed message routing tokens.
 *
 * Highlights addressing (recipient/relay chains), commands, storage targets,
 * and groups with colored pill-style backgrounds. Relay chains are rendered
 * as a single pill with the final recipient in bold. Rich contextual tooltips
 * describe the routing.
 */

#include "DirectedMessageHighlighter.h"
#include "DirectedMessageParser.h"
#include "DirectedMessageTooltipUtils.h"

#include <QTextDocument>

namespace {

QString explicitTargetTooltip(const QString &baseTip,
                              const QString &selectedCallsign,
                              const QString &explicitTarget) {
    if (selectedCallsign.isEmpty() || selectedCallsign == explicitTarget)
        return baseTip;

    return QString("%1. Overrides selected %2.")
        .arg(baseTip, selectedCallsign);
}

QString tokenTooltip(const DirectedMessageParser::Token &token,
                     const QString &upperText,
                     const QString &selectedCallsign) {
    switch (token.type) {
    case DirectedMessageParser::Token::Sender:
    case DirectedMessageParser::Token::Command:
        return token.tooltip;
    case DirectedMessageParser::Token::Recipient: {
        const QString recipient = upperText.mid(token.start, token.length);
        return explicitTargetTooltip(token.tooltip, selectedCallsign,
                                     recipient);
    }
    case DirectedMessageParser::Token::Group: {
        const QString group = upperText.mid(token.start, token.length);
        return explicitTargetTooltip(token.tooltip, selectedCallsign, group);
    }
    case DirectedMessageParser::Token::RelayChain:
        if (!token.partial)
            return token.tooltip;

        if (selectedCallsign.isEmpty()) {
            const QString recipient =
                (token.recipientHopIndex >= 0 &&
                 token.recipientHopIndex < token.hops.size())
                    ? upperText.mid(token.hops[token.recipientHopIndex].first,
                                    token.hops[token.recipientHopIndex].second)
                    : QStringLiteral("the final recipient");
            return QString("Partial relay to %1. First hop comes from the "
                           "selected callsign, but no station is currently "
                           "selected.")
                .arg(recipient);
        }

        QStringList hops;
        hops.append(selectedCallsign);
        for (const auto &hop : token.hops)
            hops.append(upperText.mid(hop.first, hop.second));
        return DirectedMessageTooltipUtils::routingTooltipFromTexts(hops);
    }

    return token.tooltip;
}

} // namespace

DirectedMessageHighlighter::DirectedMessageHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    m_addressColor = QColor("#2980b9"); // blue
    m_addressFontColor = Qt::white;
    m_commandColor = QColor("#e67e22"); // orange
    m_commandFontColor = Qt::white;
    m_groupColor = QColor("#16a085");   // teal
    m_groupFontColor = Qt::white;
    m_senderColor = QColor("#7f8c8d");  // grey/slate
    m_senderFontColor = Qt::white;
}

void DirectedMessageHighlighter::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void DirectedMessageHighlighter::setPillColors(const PillColorScheme &scheme) {
    m_addressColor = scheme.recipientBg;
    m_addressFontColor = scheme.recipientFg;
    m_commandColor = scheme.commandBg;
    m_commandFontColor = scheme.commandFg;
    m_groupColor = scheme.groupBg;
    m_groupFontColor = scheme.groupFg;
    m_senderColor = scheme.senderBg;
    m_senderFontColor = scheme.senderFg;
}

void DirectedMessageHighlighter::setSelectedCallsign(const QString &callsign) {
    m_selectedCallsign = callsign.toUpper();
}

QTextCharFormat DirectedMessageHighlighter::makePillFormat(
    QColor pillColor, QColor fg, QColor bg, int pillGroup,
    const QString &tooltip, bool bold) const {
    QTextCharFormat fmt;
    fmt.setForeground(fg);
    fmt.setBackground(bg);
    fmt.setProperty(PillColorProperty, pillColor);
    fmt.setProperty(PillGroupProperty, pillGroup);
    fmt.setToolTip(tooltip);
    if (bold)
        fmt.setFontWeight(QFont::Bold);
    return fmt;
}

void DirectedMessageHighlighter::highlightBlock(const QString &text) {
    if (!m_enabled)
        return;

    const QString upperText = text.toUpper();
    const auto tokens =
        DirectedMessageParser::parseTokensForLine(text, m_selectedCallsign);
    int pillGroup = 0;

    for (const auto &token : tokens) {
        QColor color;
        QColor fontColor;
        switch (token.type) {
        case DirectedMessageParser::Token::Sender:
            color = m_senderColor;
            fontColor = m_senderFontColor;
            break;
        case DirectedMessageParser::Token::Recipient:
            color = m_addressColor;
            fontColor = m_addressFontColor;
            break;
        case DirectedMessageParser::Token::RelayChain:
            color = m_addressColor;
            fontColor = m_addressFontColor;
            break;
        case DirectedMessageParser::Token::Command:
            color = m_commandColor;
            fontColor = m_commandFontColor;
            break;
        case DirectedMessageParser::Token::Group:
            color = m_groupColor;
            fontColor = m_groupFontColor;
            break;
        }

        const QString tooltip =
            tokenTooltip(token, upperText, m_selectedCallsign);
        setFormat(token.start, token.length,
                  makePillFormat(color, fontColor, color, pillGroup,
                                 tooltip));

        if (token.type == DirectedMessageParser::Token::RelayChain &&
            token.recipientHopIndex >= 0 &&
            token.recipientHopIndex < token.hops.size()) {
            const auto hop = token.hops[token.recipientHopIndex];
            setFormat(hop.first, hop.second,
                      makePillFormat(color, fontColor, color, pillGroup,
                                     tooltip, true));
        }

        pillGroup++;
    }
}
