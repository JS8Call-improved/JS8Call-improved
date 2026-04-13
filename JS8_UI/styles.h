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

/**
* @brief Provides platform-adaptive stylesheet strings for status bar QLabel widgets.
 *
 * This module defines styling for QLabel-based status indicators,
 * with platform-specific geometry (border-radius, padding, border style) targeting
 * macOS, Windows, and Linux. A minimal fallback is provided for other platforms, e.g.
 * possibly compiling on BSD Unix, iOS, Android, etc..
 *
 * @section tx_status Transmit Status Appearance
 * The @c TxStatusAppearance enum represents four logical states for the TX status label:
 * - @c Receiving    – Active receive; rendered with a green background, black text.
 * - @c Transmitting – Active transmit; rendered with a red background, black text.
 * - @c Decoding     – Decoding in progress; currently shares colors with @c Receiving,
 *    but is defined separately to allow future change.
 * - @c IdleTimeout  – Idle or timed-out; rendered with a black background, white text.
 *
 * @fn static inline QString makeStyle(const QString& bg, const QString& fg)
 * @brief Constructs a platform-appropriate QSS stylesheet string for a QLabel.
 * @param bg Background color as a CSS color string.
 * @param fg Foreground (text) color as a CSS color string..
 * @return A QSS QString suitable for use with @c QWidget::setStyleSheet().
 *
 * @fn inline QString txStatusLabelStyle(TxStatusAppearance appearance)
 * @brief Returns the QSS stylesheet string for a given TX status state.
 * @param appearance The desired @c TxStatusAppearance state.
 * @return A QSS QString for the requested appearance, or an empty QString()
 *         for any unhandled @c default case (compiler safety fallback).
 */
static inline QString statusLabelStyle(const QString& bg = "#6699ff",
                                const QString& fg = "#000000")
{
#if defined(Q_OS_MACOS)
    return QStringLiteral(
        "QLabel{background-color: %1; color: %2; "
        "border-radius: 6px; padding: 2px 8px; "
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
        "border-radius:1px; padding:1px 6px; "
        "border:1px inset rgba(0,0,0,0.18)}"
    ).arg(bg, fg);
#endif
}

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
        "border-radius:1px; padding:1px 6px; "
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
        "border-radius:6px; padding:2px 8px; "
        "border:1px solid rgba(0,0,0,0.15);}"
    ).arg(bg, fg);
#else
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
        return makeStyle("#22ff22", "#000000");
    case TxStatusAppearance::IdleTimeout:
        return makeStyle("#000000", "#ffffff");
    default:
        return QString(); // no style for compiler fallback
    }
}

// Frequency display in main window
static inline QString logFrameStyle()
{
#if defined(Q_OS_MACOS)
    return QStringLiteral(
        "QFrame#frame { background-color: #F2F2F0; }"
        "QLabel#currentFreq {"
        " color: #39FF14;"
        " background-color: black;"
        " border-radius:6px; padding:0px 8px; "
        " font-family: Monaco, 'Courier New', monospace;"
        " font-size: 28pt;"
        " font-weight: bold;"
        " min-width: 200px;"
        " min-height: 40px;"
        "}"
    );
#elif defined(Q_OS_WIN)
    return QStringLiteral(
        "QFrame#frame { background-color: #F2F2F0; }"
        "QLabel#currentFreq {"
        " color: #39FF14;"
        " background-color: black;"
        " border-radius:4px; padding:0px 8px; "
        " font-family: Consolas, 'Courier New', monospace;"
        " font-size: 28pt;"
        " font-weight: bold;"
        " min-width: 200px;"
        " min-height: 40px;"
        "}"
    );
#else
    return QStringLiteral(
        "QFrame#frame { background-color: #F2F2F0; }"
        "QLabel#currentFreq {"
        " color: #39FF14;"
        " background-color: black;"
        " border-radius:2px; padding:0px 8px; "
        " font-size: 28pt;"
        " font-weight: bold;"
        " min-width: 200px;"
        " min-height: 40px;"
        "}"
    );
#endif
}

/**
 * @brief Constructs a unified cross-platform QSS stylesheet for the QProgressBar.
 *
 * Generates a stylesheet that produces a consistent progress bar appearance across
 * all platforms, with a white background (@c #ffffff) and a light-blue chunk fill
 * (@c #a5cdff). Border is suppressed entirely; text is centered. The bar dimensions
 * are fully constrained via @c min-height / @c max-height to prevent platform layout
 * interference.
 *
 * @param textColor The color applied to the progress bar's text label. This comes from
 * platform system pallette so it is dynamic for light/dark themes.
 * @param small     If @c true, renders a compact variant (height: 10px, radius: 3px);
 *                  if @c false (default), renders the standard size (height: 14px, radius: 5px).
 * @return A QSS QString suitable for use with @c QWidget::setStyleSheet().
 */
static inline QString progress_bar_stylesheet(const QColor&, const QColor&, bool small = false)
{
    const QString base = QString(
        "QProgressBar {"
        "  border: 0px;"
        "  background-color: #ffffff;"
        "  color: #000000;"
        "  text-align: center;"
        "  padding: 0px;"
        "  %1"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #a5cdff;"
        "  border-radius: %2px;"
        "  %3"
        "}"
    );

    const int height = small ? 10 : 14;        // overall control height
    const int radius = small ? 3 : 5;          // roundness of the bar ends
    const QString barDim = QString("min-height:%1px; max-height:%1px; border-radius:%2px;").arg(height).arg(radius);
    const QString chunkDim = QString("min-height:%1px;").arg(height);
    return base.arg(barDim, QString::number(radius), chunkDim);
}

/**
 * @brief Returns a platform-native QPushButton stylesheet.
 *
 * Generates a stylesheet that approximates the native button conventions of the
 * specific platform, using system-appropriate fonts, geometry, and accent colors.
 * Hover, pressed, and disabled pseudo-states are defined for all active variants.
 *
 * @return A QSS QString suitable for use with @c QWidget::setStyleSheet(),
 *         or an empty default Qt style on unsupported platforms.
 *
 * @note A Linux variant targeting Ubuntu/Noto Sans with a neutral grey palette
 *       (@c #E0E0E0 background, @c #BDBDBD border) has been drafted but is currently
 *       commented out pending a styling decision.
 *
 * @note On platforms other than Windows and macOS, Qt's built-in default button
 *       style is returned unchanged. This includes Linux until the pending variant
 *       is defined
 *
 * @todo Evaluate and enable the Linux stylesheet when platform styling is decided upon.
 */
inline QString buttonStyle() {
#if defined(Q_OS_WIN)
    return R"(
        QPushButton {
            background-color: #6699ff;
            color: black;
            border: none;
            border-radius: 4px;
            padding: 3px 9px;
            font-family: "Segoe UI";
        }
        QPushButton:hover {
            background-color: #4d7fff;
            color: white;
        }
        QPushButton:pressed {
            background-color: #003EAA;
        }
        QPushButton:disabled {
            background-color: #ececec;
            color: #888888;
        }
    )";

#elif defined(Q_OS_MAC)
    return R"(
        QPushButton {
            background-color: #6699ff;
            color: black;
            border: none;
            border-radius: 6px;
            padding: 3px 9px;
            min-height: 15px;
            max-height: 15px;
            font-family: "-apple-system";
        }
        QPushButton:hover {
            background-color: #4d7fff;
            color: white;
        }
        QPushButton:pressed {
            background-color: #003EAA;
        }
        QPushButton:disabled {
            background-color: #ececec;
            color: #888888;
        }
    )";

// Suggested possibility for linux - not defined at this time
//#elif defined(Q_OS_LINUX)
//    return R"(
//        QPushButton {
//            background-color: #E0E0E0;
//            color: #212121;
//            border: 1px solid #BDBDBD;
//            border-radius: 1px;
//            padding: 5px 15px;
//            font-family: "Ubuntu", "Noto Sans";
//        }
//        QPushButton:hover {
//            background-color: #F5F5F5;
//        }
//        QPushButton:pressed {
//            background-color: #BDBDBD;
//        }
//        QPushButton:disabled {
//            background-color: #ececec;
//            color: #888888;
//        }
//    )";

#else
    return R"( ... )";
#endif
}

