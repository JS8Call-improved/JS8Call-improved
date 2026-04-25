/**
 * @file NotificationSoundOutput.h
 * @brief Self-contained audio output for notification sounds.
 */

#pragma once

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QObject>
#include <qmath.h>

#include <memory>

class NotificationSoundOutput : public QObject
{
    Q_OBJECT

public:
    explicit NotificationSoundOutput(QObject *parent = nullptr);
    ~NotificationSoundOutput() override;

    void setDevice(QAudioDevice const &device, unsigned msBuffer);
    void setAttenuation(qreal a);
    void play(QByteArray const &data, QAudioFormat const &format);
    void stop();

signals:
    void status(QString message);
    void error(QString message);

private slots:
    void handleStateChanged(QAudio::State newState);

private:
    void release();

    QAudioDevice                  m_device;
    std::unique_ptr<QAudioSink>   m_sink;
    std::unique_ptr<QBuffer>      m_buffer;
    QAudioFormat                  m_currentFormat;
    qreal                         m_volume  = 1.0;
    unsigned                      m_msBuffer = 0;
};
