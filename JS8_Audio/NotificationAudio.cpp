/**
 * @file NotificationAudio.cpp
 * @brief Implementation of NotificationAudio class
 */
#include "NotificationAudio.h"
#include "BWFFile.h"
#include "SoundOutput.h"

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
    : QObject{parent}, m_stream{new NotificationSoundOutput} {
    connect(m_stream.data(), &NotificationSoundOutput::status, this,
            &NotificationAudio::status);
    connect(m_stream.data(), &NotificationSoundOutput::error, this,
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
        return;
    }

    BWFFile file(QAudioFormat{}, filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    if (data.isEmpty())
        return;

    QAudioFormat fmt = file.format();

    if (!m_device.isFormatSupported(fmt)) {
        error("Requested output audio format is not supported on device.");
        return;
    }

    // If source is 24-bit packed PCM, repack to Int32
    if (file.bitsPerSample() == 24) {
        QByteArray converted = pcm24le_to_int32le(data);
        if (converted.isEmpty()) {
            error("Bad 24-bit PCM size");
            return;
        }
        data = std::move(converted);
        fmt.setSampleFormat(QAudioFormat::Int32);
        // keep sampleRate + channelCount from file.format()
    }

    // Mono -> stereo so mono files play through both speakers
    if (!upmixMonoToStereoInPlace(fmt, data)) {
        error("Unsupported mono upmix format");
        return;
    }

    playEntry(m_cache.emplace(filePath, fmt, data));
}

/**
 * @brief Stops audio playback.
 */
void NotificationAudio::stop() { m_stream->stop(); }

/******************************************************************************/
// Private Implementation
/******************************************************************************/

void NotificationAudio::playEntry(Cache::const_iterator const it) {
    auto const &[format, data] = *it;
    m_stream->setDevice(m_device, m_msBuffer);
    m_stream->play(data, format);
}

/**
 * @brief Convert 24-bit PCM to 32-bit PCM
 * @param PCM data byte array
 */
QByteArray NotificationAudio::pcm24le_to_int32le(const QByteArray& in)
{
    if ((in.size() % 3) != 0) return {};

    const int samples = in.size() / 3;
    QByteArray out;
    out.resize(samples * 4);

    const uint8_t* src = reinterpret_cast<const uint8_t*>(in.constData());
    int32_t* dst = reinterpret_cast<int32_t*>(out.data());

    for (int i = 0; i < samples; ++i) {
        int32_t v = (int32_t(src[0])      ) |
                    (int32_t(src[1]) <<  8) |
                    (int32_t(src[2]) << 16);

        // sign extend 24 -> 32
        if (v & 0x00800000) v |= 0xFF000000;

        // scale to full 32-bit range (optional but common)
        dst[i] = v << 8;

        src += 3;
    }
    return out;
}

/**
 * @brief Convert mono PCM data to stereo
 * @param fmt The audio format
 * @param data The audio data byte array
 * @return true if upmix was successful, false otherwise
 */
bool NotificationAudio::upmixMonoToStereoInPlace(QAudioFormat& fmt, QByteArray& data)
{
    if (fmt.channelCount() != 1)
        return true; // nothing to do

    switch (fmt.sampleFormat()) {
    case QAudioFormat::UInt8: {
        // 1 byte/sample -> 2 bytes/frame (L,R)
        QByteArray out;
        out.resize(data.size() * 2);

        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        uint8_t* dst = reinterpret_cast<uint8_t*>(out.data());

        for (int i = 0; i < data.size(); ++i) {
            const uint8_t s = src[i];
            *dst++ = s; // L
            *dst++ = s; // R
        }

        data = std::move(out);
        fmt.setChannelCount(2);
        return true;
    }

    case QAudioFormat::Int16: {
        if ((data.size() % 2) != 0) return false;
        const int samples = data.size() / 2;

        QByteArray out;
        out.resize(samples * 4);

        const int16_t* src = reinterpret_cast<const int16_t*>(data.constData());
        int16_t* dst = reinterpret_cast<int16_t*>(out.data());

        for (int i = 0; i < samples; ++i) {
            const int16_t s = src[i];
            *dst++ = s; // L
            *dst++ = s; // R
        }

        data = std::move(out);
        fmt.setChannelCount(2);
        return true;
    }

    case QAudioFormat::Int32: {
        if ((data.size() % 4) != 0) return false;
        const int samples = data.size() / 4;

        QByteArray out;
        out.resize(samples * 8);

        const int32_t* src = reinterpret_cast<const int32_t*>(data.constData());
        int32_t* dst = reinterpret_cast<int32_t*>(out.data());

        for (int i = 0; i < samples; ++i) {
            const int32_t s = src[i];
            *dst++ = s; // L
            *dst++ = s; // R
        }

        data = std::move(out);
        fmt.setChannelCount(2);
        return true;
    }

    case QAudioFormat::Float: {
        // Qt float samples are 32-bit
        if ((data.size() % 4) != 0) return false;
        const int samples = data.size() / 4;

        QByteArray out;
        out.resize(samples * 8); // mono float32 -> stereo float32

        const float* src = reinterpret_cast<const float*>(data.constData());
        float* dst = reinterpret_cast<float*>(out.data());

        for (int i = 0; i < samples; ++i) {
            const float s = src[i];
            *dst++ = s; // L
            *dst++ = s; // R
        }

        data = std::move(out);
        fmt.setChannelCount(2);
        return true;
    }

    default:
        return false; // unsupported here
    }
}

/******************************************************************************/

Q_LOGGING_CATEGORY(notificationaudio_js8, "notificationaudio.js8", QtWarningMsg)
