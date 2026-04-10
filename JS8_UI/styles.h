/**
 * @file styles.h
 * @brief header file that defines platform specific label, button, widget and dialog
 * styles used in functions in the UI_Constructor class that assembles the UI and provides
 * the proper "look and feel" for each desktop platform
 *
 * styles.h was added on 11 Apr, 2026 and is intended to eventually become the default
 * StyleSheet configuration for the JS8Call user interface.
 */

#pragma once
#include <QtGlobal>
#include <QtGui/QGuiApplication>
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QString>

inline QString statusLabelStyle(const QString& bg = "#6699ff",
                                const QString& fg = "#000000")
{
#if defined(USE_QSS_THEMES)
    // If a global application stylesheet is active, defer to it and don't override.
    if (qApp && !qApp->styleSheet().isEmpty()) {
        return QString();
    }
#endif

// Status bar label StyleSheet
#if defined(Q_OS_MACOS)
    return QStringLiteral(
        "QLabel{background-color: %1; color: %2; "
        "border-radius: 8px; padding: 2px 8px; "
        "border: 1px solid rgba(0,0,0,0.15)}"
    ).arg(bg, fg);
#elif defined(Q_OS_WIN)
    return QStringLiteral(
        "QLabel{background-color:%1; color:%2; "
        "border-radius:4px; padding:0px 8px; "
        "border:1px solid rgba(0,0,0,0.25)}"
    ).arg(bg, fg);
#else // Linux/other
    return QStringLiteral(
        "QLabel{background-color:%1; color:%2; "
        "border-radius:2px; padding:1px 6px; "
        "border:1px inset rgba(0,0,0,0.18)}"
    ).arg(bg, fg);
#endif
}

// We need special handling for tx_status_label since it technically has four states,
// however, Receiving and Decoding are the presently the same colors. But this allows
// Decoding to be set differently if we choose to do so
enum class TxStatusAppearance {
    Receiving,    // green on black text
    Transmitting, // red
    Decoding,     // same as Receiving
    IdleTimeout   // black bg, white fg
};

QString txStatusLabelStyle(TxStatusAppearance appearance);

static inline QString makeStyle(const QString& bg, const QString& fg) {
#if defined(Q_OS_LINUX)
    return QString(
        "QLabel{background-color:%1; color:%2; "
        "border-radius:2px; padding:1px 6px; "
        "border:1px inset rgba(0,0,0,0.18);}"
    ).arg(bg, fg);
#elif defined(Q_OS_WIN)
    return QString(
        "QLabel{background-color:%1; color:%2; "
        "border-radius:4px; padding:0px 8px; "
        "border:1px solid rgba(0,0,0,0.25);}"
    ).arg(bg, fg);
#elif defined(Q_OS_MACOS)
    return QString(
        "QLabel{background-color:%1; color:%2; "
        "border-radius:8px; padding:2px 8px; "
        "border:1px solid rgba(0,0,0,0.15);}"
    ).arg(bg, fg);
#else
    // Fallback in the event we're compiling on an operating system not
    // defined in the above i.e. BSD Unix, etc. So style isn't defined here
    return QString(
        "QLabel{background-color:%1; color:%2;}"
    ).arg(bg, fg);
#endif
}

inline QString txStatusLabelStyle(TxStatusAppearance appearance) {
    switch (appearance) {
    case TxStatusAppearance::Receiving:
        return makeStyle("#22ff22", "#000000");
    case TxStatusAppearance::Transmitting:
        return makeStyle("#ff2222", "#000000");
    case TxStatusAppearance::Decoding:
        return makeStyle("#22ff22", "#000000"); // same as Receiving but we can define a different color if we want
    case TxStatusAppearance::IdleTimeout:
        return makeStyle("#000000", "#ffffff");
    default:
        return QString(); // no style for compiler fallback
    }
}

// QProgressBar StyleSheet
inline QString progress_bar_stylesheet(const QColor& textColor, bool small = false);

inline QString progress_bar_stylesheet(const QColor& textColor, bool small)
{
    const QString base = QString(
        "QProgressBar {"
        "  border: 0px;"
        "  background-color: #ffffff;"
        "  color: %1;"
        "  text-align: center;"
        "  padding: 0px;"
        "  %2"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #a5cdff;"
        "  border-radius: %3px;"
        "  %4"
        "}"
    );

    // Unified progress bar across all platforms
    const int height = small ? 10 : 14;        // overall control height
    const int radius = small ? 3 : 5;          // roundness of the bar ends
    const QString barDim = QString("min-height:%1px; max-height:%1px; border-radius:%2px;").arg(height).arg(radius);
    const QString chunkDim = QString("min-height:%1px;").arg(height);
    return base.arg(textColor.name(), barDim, QString::number(radius), chunkDim);
}
