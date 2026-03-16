/**
 * @file DirectedMessageParser.cpp
 * @brief Shared parser for directed-message pills.
 */

#include "DirectedMessageParser.h"

#include "DirectedMessageTooltipUtils.h"
#include "Varicode.h"

#include <algorithm>
#include <QRegularExpression>

namespace {

struct PillCommandDef {
    const char *command;
    const char *tooltip;
    bool includesArg;
    bool allowsBare;
};

static const PillCommandDef s_commandDefs[] = {
    // Directed-message commands.
    {"AGN?", "Request: Please repeat", false, false},
    {"QSL?", "Query: Did you receive?", false, false},
    {"HW CPY?", "Query: How do you copy me?", false, false},
    {"SNR?", "Query: Request signal report", false, false},
    {"INFO?", "Query: Request station information", false, false},
    {"GRID?", "Query: Request grid locator", false, false},
    {"STATUS?", "Query: Request station status", false, false},
    {"QUERY MSGS?", "Query: Do you have stored messages for me?", false,
     false},
    {"HEARING?", "Query: What stations do you hear?", false, false},
    {"MSG TO:", "Store message at relay for %1", true, false},
    {"HEARTBEAT SNR", "Heartbeat signal report", false, false},
    {"QUERY MSGS", "Query: Do you have stored messages for me?", false,
     false},
    {"QUERY CALL", "Query: Can you reach %1?", true, false},
    {"QUERY MSG", "Query: Deliver stored message %1", true, false},
    {"QUERY", "Generic query", false, false},
    {"DIT DIT", "Two dits - casual sign-off", false, false},
    {"STATUS", "Station status message", false, false},
    {"HEARING", "Stations heard list", false, false},
    {"CMD", "Generic command", false, false},
    {"MSG", "Send a complete message", false, false},
    {"NACK", "Negative acknowledge", false, false},
    {"ACK", "Acknowledge receipt", false, false},
    {"SNR", "Signal report value", false, false},
    {"QSL", "Confirm: I received your message", false, false},
    {"INFO", "Station information", false, false},
    {"GRID", "My grid locator is %1", true, false},
    {"73", "Best regards - end of contact", false, false},
    {"YES", "Affirmative", false, false},
    {"NO", "Negative", false, false},
    {"RR", "Roger received", false, false},
    {"SK", "End of contact - signing off", false, false},
    {"FB", "Fine business - excellent", false, false},

    // Bare-capable CQ/HB variants.
    {"CQ CQ CQ", "Calling all stations", false, true},
    {"CQ CONTEST", "Calling all stations (contest)", false, true},
    {"CQ FIELD", "Calling all stations (field day)", false, true},
    {"CQ DX", "Calling all stations (DX)", false, true},
    {"CQ QRP", "Calling all stations (low power)", false, true},
    {"CQ FD", "Calling all stations (field day)", false, true},
    {"CQ CQ", "Calling all stations", false, true},
    {"CQ", "Calling all stations", false, true},
    {"HB", "Heartbeat - automatic presence beacon", true, true},
    {"HEARTBEAT", "Heartbeat - automatic presence beacon", true, true},
};

struct CommandMatch {
    const PillCommandDef *def = nullptr;
    QString commandText;
    QString argText;
    int start = 0;
    int finalLength = 0;
    bool hasMatch = false;
};

template <std::size_t N>
QList<const PillCommandDef *> sortedCommandDefs(const PillCommandDef (&defs)[N]) {
    QList<const PillCommandDef *> sorted;
    sorted.reserve(static_cast<qsizetype>(N));
    for (const auto &def : defs)
        sorted.append(&def);

    std::sort(sorted.begin(), sorted.end(),
              [](const PillCommandDef *lhs, const PillCommandDef *rhs) {
                  const auto left = QLatin1String(lhs->command);
                  const auto right = QLatin1String(rhs->command);
                  if (left.size() != right.size())
                      return left.size() > right.size();
                  return left < right;
              });

    return sorted;
}

const QRegularExpression &callsignRe() {
    static const QRegularExpression re(R"([@]?[A-Z0-9/]+)");
    return re;
}

const QRegularExpression &relayChainRe() {
    static const QRegularExpression re(
        R"(^([@]?[A-Z0-9/]+)(\s*>\s*[@]?[A-Z0-9/]+)+)");
    return re;
}

const QRegularExpression &partialRelayRe() {
    static const QRegularExpression re(R"(^(\s*>\s*[@]?[A-Z0-9/]+)+)");
    return re;
}

const QRegularExpression &grid4Re() {
    static const QRegularExpression re(R"(^[A-R]{2}[0-9]{2}$)");
    return re;
}

const PillCommandDef *findCommandDef(const QString &cmd) {
    for (const auto &def : s_commandDefs) {
        if (cmd == QLatin1String(def.command))
            return &def;
    }
    return nullptr;
}

bool commandNeedsWordBoundary(const QString &cmd) {
    if (cmd.isEmpty())
        return false;

    const QChar last = cmd.at(cmd.length() - 1);
    return last != '?' && last != ':';
}

bool isHeartbeatCommand(const QString &cmd) {
    return cmd == QLatin1String("HB") || cmd == QLatin1String("HEARTBEAT");
}

CommandMatch buildCommandMatch(const PillCommandDef *def, const QString &text,
                               int start, bool bareOnly) {
    CommandMatch match;
    if (!def)
        return match;

    const QString cmd = QLatin1String(def->command);
    match.def = def;
    match.commandText = cmd;
    match.start = start;
    match.finalLength = cmd.length();
    match.hasMatch = true;

    if (!def->includesArg)
        return match;

    int argStart = start + cmd.length();
    while (argStart < text.length() && text.at(argStart) == ' ')
        argStart++;
    if (argStart >= text.length())
        return match;

    int argEnd = argStart;
    while (argEnd < text.length() && text.at(argEnd) != ' ')
        argEnd++;

    const QString argText = text.mid(argStart, argEnd - argStart);
    if (isHeartbeatCommand(cmd) &&
        !grid4Re().match(argText).hasMatch()) {
        if (bareOnly)
            return CommandMatch{};
        return match;
    }

    match.argText = argText;
    match.finalLength = argEnd - start;
    return match;
}

CommandMatch matchCommandAt(const QString &text, int start, bool bareOnly) {
    if (start < 0 || start >= text.length())
        return CommandMatch{};

    static const QList<const PillCommandDef *> sorted =
        sortedCommandDefs(s_commandDefs);

    for (const auto *def : sorted) {
        const QString cmd = QLatin1String(def->command);
        if (text.indexOf(cmd, start) != start)
            continue;

        const int afterCmd = start + cmd.length();
        if (commandNeedsWordBoundary(cmd) && afterCmd < text.length() &&
            text.at(afterCmd) != ' ') {
            continue;
        }

        if (bareOnly && !def->allowsBare)
            return CommandMatch{};

        return buildCommandMatch(def, text, start, bareOnly);
    }

    if (!bareOnly && text.at(start) == '?') {
        CommandMatch compat;
        compat.commandText = QStringLiteral("?");
        compat.start = start;
        compat.finalLength = 1;
        compat.hasMatch = true;
        return compat;
    }

    return CommandMatch{};
}

QString commandTooltip(const QString &cmd) {
    if (const auto *def = findCommandDef(cmd))
        return QString::fromUtf8(def->tooltip);
    if (cmd.startsWith(QLatin1String("CQ")))
        return QStringLiteral("Calling all stations");
    return QString("Command: %1").arg(cmd);
}

QString formatCommandTooltip(const CommandMatch &match,
                             const QString &implicitTarget,
                             bool usedImplicitTarget) {
    QString tip = commandTooltip(match.commandText);
    if (tip.contains(QLatin1String("%1"))) {
        tip = tip.arg(match.argText.isEmpty() ? QStringLiteral("\xe2\x80\xa6")
                                              : match.argText);
    }

    if (usedImplicitTarget && !implicitTarget.isEmpty()) {
        tip = QString("%1. Directed to selected %2.")
                  .arg(tip, implicitTarget);
    }

    return tip;
}

QList<QPair<int, int>> parseHops(const QString &chainText) {
    QList<QPair<int, int>> hops;
    int scanPos = 0;
    while (scanPos < chainText.length()) {
        while (scanPos < chainText.length() &&
               (chainText[scanPos] == ' ' || chainText[scanPos] == '>')) {
            scanPos++;
        }
        if (scanPos >= chainText.length())
            break;
        int callStart = scanPos;
        while (scanPos < chainText.length() &&
               chainText[scanPos] != '>' && chainText[scanPos] != ' ')
            scanPos++;
        if (scanPos > callStart)
            hops.append({callStart, scanPos - callStart});
    }
    return hops;
}

QList<DirectedMessageParser::Token> parseTokensUpper(
    const QString &text, const QString &implicitTarget) {
    QList<DirectedMessageParser::Token> tokens;

    if (text.isEmpty() || text.startsWith('`'))
        return tokens;

    int contentOffset = 0;
    {
        int colonPos = text.indexOf(':');
        if (colonPos > 0 && colonPos + 1 < text.length() &&
            text.at(colonPos + 1) == ' ') {
            QString maybeSender = text.left(colonPos);
            auto match = callsignRe().match(maybeSender);
            if (match.hasMatch() && match.capturedStart() == 0 &&
                match.capturedLength() == maybeSender.length() &&
                !maybeSender.startsWith('@') &&
                Varicode::isValidCallsign(maybeSender, nullptr)) {
                DirectedMessageParser::Token t;
                t.start = 0;
                t.length = colonPos + 1;
                t.type = DirectedMessageParser::Token::Sender;
                t.tooltip = QString("Sent from %1").arg(maybeSender);
                tokens.append(t);
                contentOffset = colonPos + 2;
            }
        }
    }

    const QString content =
        (contentOffset > 0) ? text.mid(contentOffset) : text;

    int pos = 0;
    bool explicitTargetPresent = false;

    auto relayMatch = relayChainRe().match(content);
    if (relayMatch.hasMatch()) {
        DirectedMessageParser::Token t;
        t.start = 0;
        t.length = relayMatch.capturedLength();
        t.type = DirectedMessageParser::Token::RelayChain;

        QString chainText = relayMatch.captured();
        t.hops = parseHops(chainText);
        t.recipientHopIndex = t.hops.size() - 1;
        t.tooltip = DirectedMessageTooltipUtils::relayTooltip(chainText,
                                                              t.hops);

        tokens.append(t);
        pos = t.length;
        explicitTargetPresent = true;
    } else {
        int firstSpace = content.indexOf(' ');
        QString addressPart =
            (firstSpace >= 0) ? content.left(firstSpace) : content;

        bool foundAddress = false;
        int addressEndPos = -1;

        if (!addressPart.isEmpty()) {
            if (addressPart.startsWith('@')) {
                DirectedMessageParser::Token t;
                t.start = 0;
                t.length = addressPart.length();
                t.type = DirectedMessageParser::Token::Group;
                t.tooltip = QString("Group call to %1").arg(addressPart);
                tokens.append(t);
                foundAddress = true;
                explicitTargetPresent = true;
                addressEndPos = addressPart.length();
            } else {
                auto match = callsignRe().match(addressPart);
                if (match.hasMatch() && match.capturedStart() == 0 &&
                    match.capturedLength() == addressPart.length() &&
                    Varicode::isValidCallsign(addressPart, nullptr)) {
                    DirectedMessageParser::Token t;
                    t.start = 0;
                    t.length = addressPart.length();
                    t.type = DirectedMessageParser::Token::Recipient;
                    t.tooltip = QString("Directed to %1").arg(addressPart);
                    tokens.append(t);
                    foundAddress = true;
                    explicitTargetPresent = true;
                    addressEndPos = addressPart.length();
                }
            }
        }

        if (!foundAddress && content.startsWith('>')) {
            auto partialMatch = partialRelayRe().match(content);
            if (partialMatch.hasMatch()) {
                DirectedMessageParser::Token t;
                t.start = 0;
                t.length = partialMatch.capturedLength();
                t.type = DirectedMessageParser::Token::RelayChain;
                t.partial = true;

                QString chainText = partialMatch.captured();
                t.hops = parseHops(chainText);
                t.recipientHopIndex = t.hops.size() - 1;
                t.tooltip = DirectedMessageTooltipUtils::relayTooltip(
                    chainText, t.hops);

                tokens.append(t);
                foundAddress = true;
                explicitTargetPresent = true;
                addressEndPos = t.length;
            }
        }

        if (foundAddress) {
            pos = (addressEndPos >= 0)
                      ? addressEndPos
                      : ((firstSpace >= 0) ? firstSpace : content.length());
        }
    }

    if (!explicitTargetPresent) {
        const auto bareMatch = matchCommandAt(content, 0, true);
        if (bareMatch.hasMatch) {
            DirectedMessageParser::Token t;
            t.start = bareMatch.start;
            t.length = bareMatch.finalLength;
            t.type = DirectedMessageParser::Token::Command;
            t.tooltip = formatCommandTooltip(bareMatch, QString(), false);
            tokens.append(t);

            for (auto &tok : tokens) {
                if (tok.type != DirectedMessageParser::Token::Sender)
                    tok.start += contentOffset;
            }
            return tokens;
        }
    }

    const bool hasTarget =
        explicitTargetPresent || !implicitTarget.isEmpty();
    if (hasTarget && pos < content.length()) {
        while (pos < content.length() && content[pos] == ' ')
            pos++;

        if (pos < content.length()) {
            const auto cmdMatch = matchCommandAt(content, pos, false);
            if (cmdMatch.hasMatch) {
                DirectedMessageParser::Token t;
                t.start = cmdMatch.start;
                t.length = cmdMatch.finalLength;
                t.type = DirectedMessageParser::Token::Command;
                t.tooltip = formatCommandTooltip(
                    cmdMatch, implicitTarget,
                    !explicitTargetPresent && !implicitTarget.isEmpty());
                tokens.append(t);
            }
        }
    }

    for (auto &tok : tokens) {
        if (tok.type == DirectedMessageParser::Token::Sender)
            continue;
        tok.start += contentOffset;
        for (auto &hop : tok.hops)
            hop.first += contentOffset;
    }

    return tokens;
}

} // namespace

QList<DirectedMessageParser::Token>
DirectedMessageParser::parseTokensForLine(const QString &text,
                                          const QString &implicitTarget) {
    return parseTokensUpper(text.toUpper(), implicitTarget.toUpper());
}
