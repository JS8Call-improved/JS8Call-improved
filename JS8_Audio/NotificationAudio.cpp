/**
 * @file NotificationAudio.cpp
 * @brief Implementation of NotificationAudio class
 */
#include "NotificationAudio.h"
#include "BWFFile.h"
#include "soundout.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(notificationaudio_js8)

/******************************************************************************/
// Public Implementation
/******************************************************************************/
/**
 * @brief Constructs a NotificationAudio object.
 * @param parent The parent QObject.
 * @return None.
 */
NotificationAudio::NotificationAudio(QObject *parent)
    : QObject{parent}, m_stream{new SoundOutput} {
    connect(m_stream.data(), &SoundOutput::status, this,
            &NotificationAudio::status);
    connect(m_stream.data(), &SoundOutput::error, this,
            &NotificationAudio::error);
}

/**
 * @brief Destructs the NotificationAudio object.
 */
NotificationAudio::~NotificationAudio() { stop(); }

/**
 * @brief Handles status messages from the SoundOutput.
 * @param message The status message.
 */
void NotificationAudio::status(QString const message) {
    if (message == "Idle")
        stop();
}

/**
 * @brief Handles error messages from the SoundOutput.
 * @param message The error message.
 */
void NotificationAudio::error(QString const message) {
    qCDebug(notificationaudio_js8) << "notification error:" << message;
}

/**
 * @brief Sets the audio device and buffer size.
 * @param device The QAudioDevice to use.
 * @param msBuffer The buffer size in milliseconds.
 */
void NotificationAudio::setDevice(QAudioDevice const &device,
                                  unsigned const msBuffer) {
    m_device = device;
    m_msBuffer = msBuffer;
}

/**
 * @brief Plays an audio file from the specified file path.
 * @param filePath The path to the audio file.
 */
void NotificationAudio::play(QString const &filePath) {
    if (auto const it = m_cache.constFind(filePath); it != m_cache.constEnd()) {
        playEntry(it);
    } else if (auto file = BWFFile(QAudioFormat{}, filePath);
               file.open(QIODevice::ReadOnly)) {
        if (auto data = file.readAll(); !data.isEmpty()) {
            playEntry(m_cache.emplace(filePath, file.format(), data));
        }
    }
}

/**
 * @brief Stops audio playback.
 */
void NotificationAudio::stop() { m_stream->stop(); }

/******************************************************************************/
// Private Implementation
/******************************************************************************/

void NotificationAudio::playEntry(Cache::const_iterator const it) {
    if (m_buffer.isOpen())
        m_buffer.close();

    auto const &[format, data] = *it;

    m_buffer.setData(data);

    if (m_buffer.open(QIODevice::ReadOnly)) {
        m_stream->setDeviceFormat(m_device, format, m_msBuffer);
        m_stream->restart(&m_buffer);
    }
}

/******************************************************************************/

Q_LOGGING_CATEGORY(notificationaudio_js8, "notificationaudio.js8", QtWarningMsg)
