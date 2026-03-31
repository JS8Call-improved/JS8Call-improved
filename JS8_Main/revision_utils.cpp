/**
 * @file revision_utils.cpp
 * @brief application versioning utility
 */

#include "revision_utils.h"

#include <QCoreApplication>

QString version() {
#if defined(CMAKE_BUILD)
    QString v = (JS8CALL_VERSION);
    if (v == ("0.0.0")) {
        v = QStringLiteral("Development Build %1").arg(QLatin1String(GITSHORT));
    }
    return v;
#else
    QString v{QStringLiteral("Not for Release")};
#endif
}

QString program_title() {
    return QString{"%1 %2"}
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion());
}

QString program_version() {
    return QString{"%1 %2"}
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion());
}
