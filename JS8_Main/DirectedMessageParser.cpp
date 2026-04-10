/**
 * @file DirectedMessageParser.cpp
 * @brief Shared parser for directed-message pills.
 */

#include "DirectedMessageParser.h"

#include "DirectedMessageTooltipUtils.h"
#include "Varicode.h"

#include <algorithm>
#include <optional>
#include <QRegularExpression>

namespace {

using CommandArgParser = std::optional<QString> (*)(QStringView token);

struct PillCommandDef {
    const char *command;
    const char *tooltip;
    bool includesArg;
    bool allowsBare;
    CommandArgParser argParser = nullptr;
    const char *missingArgText = nullptr;
};

const QRegularExpression &messageIdRe() {
    static const QRegularExpression re(R"(^[0-9]+$)");
    return re;
}

const QRegularExpression &gridTokenRe() {
    static const QRegularExpression re(
        R"(^[A-R]{2}[0-9]{2}(?:[A-X]{2}(?:[0-9]{2}(?:[A-X]{2}(?:[0-9]{2})?)?)?)?$)");
    return re;
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

const QRegularExpression &runtimeRelayFollowHopStandaloneRe() {
    static const QRegularExpression re(
        R"(^\b(?<prefix>[A-Z0-9]{1,4}\/)?(?<base>([0-9A-Z])?([0-9A-Z])([0-9])([A-Z])?([A-Z])?([A-Z])?)(?<suffix>\/[A-Z0-9]{1,4})?\b$)");
    return re;
}

int nextTokenEnd(const QString &text, int start) {
    int end = start;
    while (end < text.length() && text.at(end) != ' ')
        end++;
    return end;
}

std::optional<QString> parseCallsignArg(QStringView token) {
    if (token.isEmpty())
        return std::nullopt;

    const QString tokenText = token.toString();
    if (!Varicode::isValidCallsign(tokenText, nullptr))
        return std::nullopt;

    return tokenText;
}

std::optional<QString> parseQueryCallArg(QStringView token) {
    if (token.isEmpty())
        return std::nullopt;

    if (token.endsWith(QLatin1Char('?')))
        token.chop(1);

    return parseCallsignArg(token);
}

std::optional<QString> parseMessageIdArg(QStringView token) {
    if (token.isEmpty())
        return std::nullopt;

    const QString tokenText = token.toString();
    if (!messageIdRe().match(tokenText).hasMatch())
        return std::nullopt;

    bool ok = false;
    tokenText.toInt(&ok, 10);
    if (!ok)
        return std::nullopt;

    return tokenText;
}

std::optional<QString> parseGridArg(QStringView token) {
    if (token.isEmpty())
        return std::nullopt;

    const QString tokenText = token.toString();
    if (!gridTokenRe().match(tokenText).hasMatch())
        return std::nullopt;

    return tokenText;
}

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
    {"MSG TO:", "Store message for later retrieval by %1", true, false,
     parseCallsignArg, "[target callsign is missing]"},
    {"HEARTBEAT SNR", "Heartbeat signal report", false, false},
    {"QUERY MSGS", "Query: Do you have stored messages for me?", false,
     false},
    {"QUERY CALL", "Query: Can you reach %1?", true, false,
     parseQueryCallArg, "[target callsign is missing]"},
    {"QUERY MSG", "Query: Deliver stored message %1", true, false,
     parseMessageIdArg, "[message id is missing]"},
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
    {"GRID", "My grid locator is %1", true, false, parseGridArg,
     "[grid locator is missing]"},
    {"73", "Best regards - end of contact", false, false},
    {"YES", "Affirmative", false, false},
    {"NO", "Negative", false, false},
    {"RR", "Roger received", false, false},
    {"SK", "End of contact - signing off", false, false},
    {"FB", "Fine business - excellent", false, false},

    // Bare-capable CQ/HB variants.
    {"CQ CQ CQ", "Calling all stations. Your location is %1.", true, true,
     parseGridArg, "not specified"},
    {"CQ CONTEST", "Calling all stations (contest). Your location is %1.", true,
     true, parseGridArg, "not specified"},
    {"CQ FIELD", "Calling all stations (field day). Your location is %1.", true,
     true, parseGridArg, "not specified"},
    {"CQ DX", "Calling all stations (DX). Your location is %1.", true, true,
     parseGridArg, "not specified"},
    {"CQ QRP", "Calling all stations (low power). Your location is %1.", true,
     true, parseGridArg, "not specified"},
    {"CQ FD", "Calling all stations (field day). Your location is %1.", true,
     true, parseGridArg, "not specified"},
    {"CQ CQ", "Calling all stations. Your location is %1.", true, true,
     parseGridArg, "not specified"},
    {"CQ", "Calling all stations. Your location is %1.", true, true,
     parseGridArg, "not specified"},
    {"HB", "Heartbeat - automatic presence beacon. Your location is %1.", true,
     true, parseGridArg, "not specified"},
    {"HEARTBEAT", "Heartbeat - automatic presence beacon. Your location is %1.",
     true, true, parseGridArg, "not specified"},
};

struct CommandMatch {
    const PillCommandDef *def = nullptr;
    QString commandText;
    bool hasValidArg = false;
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

CommandMatch buildCommandMatch(const PillCommandDef *def, const QString &text,
                               int start) {
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

    if (!def->argParser) {
        const int argEnd = nextTokenEnd(text, argStart);
        match.hasValidArg = true;
        match.argText = text.mid(argStart, argEnd - argStart);
        match.finalLength = argEnd - start;
        return match;
    }

    const int argEnd = nextTokenEnd(text, argStart);
    QStringView tokenView(text);
    tokenView = tokenView.sliced(argStart, argEnd - argStart);

    if (const auto parsedArg = def->argParser(tokenView)) {
        match.hasValidArg = true;
        match.argText = *parsedArg;
        match.finalLength = argStart + tokenView.length() - start;
    }
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

        return buildCommandMatch(def, text, start);
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
    return QString("Command: %1").arg(cmd);
}

QString formatCommandTooltip(const CommandMatch &match,
                             const QString &implicitTarget,
                             bool usedImplicitTarget) {
    QString tip = commandTooltip(match.commandText);
    if (tip.contains(QLatin1String("%1"))) {
        const QString replacement =
            match.hasValidArg
                ? match.argText
                : ((match.def && match.def->missingArgText)
                       ? QString::fromUtf8(match.def->missingArgText)
                       : QStringLiteral("[...]"));
        tip = tip.arg(replacement);
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

struct RelayChainValidation {
    QList<QPair<int, int>> hops;
    int validHopCount = 0;
    int tokenLength = 0;
};

bool isRelayHopZeroValid(const QString &hopText) {
    return !hopText.startsWith('@') &&
           Varicode::isValidCallsign(hopText, nullptr);
}

bool isRuntimeRelayFollowHopValid(const QString &hopText) {
    if (hopText.startsWith('@'))
        return false;

    return runtimeRelayFollowHopStandaloneRe().match(hopText).hasMatch();
}

RelayChainValidation validateRelayChain(const QString &chainText, bool partial,
                                        const QString &implicitTarget) {
    RelayChainValidation validation;
    validation.hops = parseHops(chainText);
    if (validation.hops.isEmpty())
        return validation;

    if (partial && !isRelayHopZeroValid(implicitTarget))
        return validation;

    for (int i = 0; i < validation.hops.size(); ++i) {
        const auto hop = validation.hops.at(i);
        const QString hopText = chainText.mid(hop.first, hop.second);
        const bool valid =
            (partial || i > 0) ? isRuntimeRelayFollowHopValid(hopText)
                               : isRelayHopZeroValid(hopText);
        if (!valid)
            break;
        validation.validHopCount++;
    }

    if (validation.validHopCount > 0) {
        const auto lastHop = validation.hops.at(validation.validHopCount - 1);
        validation.tokenLength = lastHop.first + lastHop.second;
        validation.hops = validation.hops.mid(0, validation.validHopCount);
    }

    return validation;
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
    bool foundAddress = false;
    int addressEndPos = -1;

    auto relayMatch = relayChainRe().match(content);
    if (relayMatch.hasMatch()) {
        const QString chainText = relayMatch.captured();
        const auto validation = validateRelayChain(chainText, false, QString());

        if (validation.validHopCount >= 2) {
            DirectedMessageParser::Token t;
            t.start = 0;
            t.length = validation.tokenLength;
            t.type = DirectedMessageParser::Token::RelayChain;
            t.hops = validation.hops;
            t.recipientHopIndex = t.hops.size() - 1;
            t.tooltip = DirectedMessageTooltipUtils::relayTooltip(
                chainText.left(validation.tokenLength), t.hops);

            tokens.append(t);
            foundAddress = true;
            explicitTargetPresent = true;
            addressEndPos = validation.tokenLength;
        } else if (validation.validHopCount == 1) {
            const auto hop = validation.hops.first();
            const QString recipient = chainText.mid(hop.first, hop.second);

            DirectedMessageParser::Token t;
            t.start = hop.first;
            t.length = hop.second;
            t.type = DirectedMessageParser::Token::Recipient;
            t.tooltip = QString("Directed to %1").arg(recipient);

            tokens.append(t);
            foundAddress = true;
            explicitTargetPresent = true;
            addressEndPos = hop.first + hop.second;
        }
    }

    if (!foundAddress) {
        int firstSpace = content.indexOf(' ');
        QString addressPart =
            (firstSpace >= 0) ? content.left(firstSpace) : content;

        if (!addressPart.isEmpty()) {
            if (addressPart.startsWith('@')) {
                if (Varicode::isCompoundCallsign(addressPart)) {
                    DirectedMessageParser::Token t;
                    t.start = 0;
                    t.length = addressPart.length();
                    t.type = DirectedMessageParser::Token::Group;
                    t.tooltip = QString("Group call to %1").arg(addressPart);
                    tokens.append(t);
                    foundAddress = true;
                    explicitTargetPresent = true;
                    addressEndPos = addressPart.length();
                }
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
                QString chainText = partialMatch.captured();
                const auto validation =
                    validateRelayChain(chainText, true, implicitTarget);
                if (validation.validHopCount >= 1) {
                    DirectedMessageParser::Token t;
                    t.start = 0;
                    t.length = validation.tokenLength;
                    t.type = DirectedMessageParser::Token::RelayChain;
                    t.partial = true;
                    t.hops = validation.hops;
                    t.recipientHopIndex = t.hops.size() - 1;
                    t.tooltip = DirectedMessageTooltipUtils::relayTooltip(
                        chainText.left(validation.tokenLength), t.hops);

                    tokens.append(t);
                    foundAddress = true;
                    explicitTargetPresent = true;
                    addressEndPos = validation.tokenLength;
                }
            }
        }
    }

    if (foundAddress) {
        pos = (addressEndPos >= 0)
                  ? addressEndPos
                  : content.length();
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
