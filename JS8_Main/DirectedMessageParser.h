#ifndef DIRECTEDMESSAGEPARSER_H
#define DIRECTEDMESSAGEPARSER_H

#include <QList>
#include <QPair>
#include <QString>

class DirectedMessageParser {
  public:
    struct Token {
        int start = 0;
        int length = 0;
        enum Type {
            Sender,
            Recipient,
            RelayChain,
            Command,
            Group
        } type = Recipient;
        QString tooltip;
        QList<QPair<int, int>> hops;
        int recipientHopIndex = -1;
        bool partial = false;
    };

    static QList<Token> parseTokensForLine(
        const QString &text, const QString &implicitTarget = QString());
};

#endif // DIRECTEDMESSAGEPARSER_H
