#ifndef DIRECTEDMESSAGETOOLTIPUTILS_H
#define DIRECTEDMESSAGETOOLTIPUTILS_H

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace DirectedMessageTooltipUtils {

inline QString joinHumanList(const QStringList &items) {
    if (items.isEmpty())
        return QString();
    if (items.size() == 1)
        return items.first();
    return items.mid(0, items.size() - 1).join(", ") +
           QStringLiteral(" and ") + items.last();
}

inline QString routingTooltipFromTexts(const QStringList &hops) {
    if (hops.isEmpty())
        return QString();
    if (hops.size() == 1)
        return QString("Relay to %1").arg(hops.first());

    const QString destination = hops.last();
    const QStringList relays = hops.mid(0, hops.size() - 1);
    return QString("Routing through %1 to %2")
        .arg(joinHumanList(relays), destination);
}

inline QString relayTooltip(const QString &chainText,
                            const QList<QPair<int, int>> &hops) {
    QStringList hopTexts;
    hopTexts.reserve(hops.size());
    for (const auto &hop : hops)
        hopTexts.append(chainText.mid(hop.first, hop.second));
    return routingTooltipFromTexts(hopTexts);
}

} // namespace DirectedMessageTooltipUtils

#endif // DIRECTEDMESSAGETOOLTIPUTILS_H
