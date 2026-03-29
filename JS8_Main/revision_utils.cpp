/**
 * @file revision_utils.cpp
 * @brief application versioning utility
 */

#include "revision_utils.h"

#include <QCoreApplication>

QString version() {
#if defined(CMAKE_BUILD)
    QString v{JS8CALL_VERSION};
#else
    QString v{"Not for Release"};
#endif

    return v;
}

QString program_title() {
    return QString{"%1(v%2) de KN4CRD"}
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion());
}

QString program_version() {
    return QString{"%1 v%2"}
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion());
}
