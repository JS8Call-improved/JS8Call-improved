#include "TCIAudioInput.h"

#include <QLoggingCategory>
#include <QThread>
#include <QMetaObject>

#include "moc_TCIAudioInput.cpp"

Q_DECLARE_LOGGING_CATEGORY(tciaudioinput_js8)

TCIAudioInput::TCIAudioInput(QObject *parent)
    : QObject(parent)
{
}

TCIAudioInput::~TCIAudioInput()
{
    stop();
}

void TCIAudioInput::start(QUrl const &url, int framesPerBuffer,
                          AudioDevice *sink,
                          AudioDevice::Channel channel)
{
    Q_UNUSED(channel);

    stop();

    if (!sink) {
        Q_EMIT error(tr("No audio sink supplied for TCI input."));
        return;
    }

    m_url = url;
    m_sink = sink;
    m_framesPerBuffer = framesPerBuffer;
    m_started = false;
    m_suspended = false;
    m_stopping = false;


    if (!m_sink) {
        Q_EMIT error(tr("No audio sink supplied for TCI input."));
        return;
    }

    // TCI bridge RX audio is 48 kHz mono signed int16.
    // Force Mono here. The configured left/right/both channel selector applies
    // to physical stereo devices, not the network mono stream.
    if (!m_sink->initialize(QIODevice::WriteOnly, AudioDevice::Mono)) {
        Q_EMIT error(tr("Failed to initialize TCI audio sink device."));
        m_sink.clear();
        return;
    }

    Q_EMIT status(tr("Connecting"));

    qCInfo(tciaudioinput_js8) << "Connecting TCI audio input:" << url;

    create_client();

    m_session->connectToServer(m_url);
    m_client->emitReadyIfReady();
}

void TCIAudioInput::handle_ready()
{
    if (m_stopping || m_suspended)
        return;

    if (!m_sink || !m_client)
        return;

    if (m_sink)
        m_sink->reset();

    if (m_suspended) {
        m_started = false;
        Q_EMIT status(tr("Suspended"));
        return;
    }

    qCInfo(tciaudioinput_js8)
        << "TCI audio input ready; starting RX stream";

    m_client->configureAudio(48000, 1, 512);
    m_client->startRxAudio();

    m_started = true;

    Q_EMIT status(tr("Receiving"));
}

/*
void TCIAudioInput::handle_rx_audio_frame(QByteArray pcm, int sampleRate)
{
    if (!m_started || m_suspended || !m_sink)
        return;

    if (sampleRate != 48000) {
        Q_EMIT error(tr("Unsupported TCI audio sample rate: %1 Hz.")
                         .arg(sampleRate));
        return;
    }

    if (pcm.isEmpty())
        return;

    qint64 const written = m_sink->write(pcm.constData(), pcm.size());

    if (written != pcm.size()) {
        qCWarning(tciaudioinput_js8)
            << "Short write to TCI audio sink:"
            << written
            << "of"
            << pcm.size();
    }
}
*/
void TCIAudioInput::handle_rx_audio_frame(QByteArray pcm, int sampleRate)
{
    if (!m_started || m_suspended || !m_sink)
        return;

    if (sampleRate != 48000) {
        Q_EMIT error(tr("Unsupported TCI audio sample rate: %1 Hz.")
                         .arg(sampleRate));
        return;
    }

    if (pcm.isEmpty())
        return;

    qint64 const written = m_sink->write(pcm.constData(), pcm.size());

    if (written != pcm.size()) {
        qCWarning(tciaudioinput_js8)
            << "Short write to TCI audio sink:"
            << written
            << "of"
            << pcm.size();
    }
}

void TCIAudioInput::setSession(TCISession *session)
{
    m_session = session;
}

void TCIAudioInput::suspend()
{
    m_suspended = true;

    if (m_client && m_client->isConnected())
        m_client->stopRxAudio();

    Q_EMIT status(tr("Suspended"));
}

void TCIAudioInput::resume()
{
    if (m_sink)
        m_sink->reset();

    m_suspended = false;

    if (m_client && m_client->isConnected() && m_client->isReady()) {
        m_client->startRxAudio();
        m_started = true;
        Q_EMIT status(tr("Receiving"));
        return;
    }

    m_started = false;

    if (!m_stopping) {
        Q_EMIT status(tr("Waiting for TCI connection"));

        if (m_session && m_url.isValid())
            m_session->connectToServer(m_url);

        return;
    }

    Q_EMIT status(tr("Stopped"));
}

void TCIAudioInput::stop()
{
    m_stopping = true;
    m_started = false;
    m_suspended = false;

    if (m_client && m_client->isConnected())
        m_client->stopRxAudio();

    destroy_client();

    if (m_sink) {
        m_sink->close();
        m_sink.clear();
    }

    Q_EMIT status(tr("Stopped"));
}


void TCIAudioInput::handle_disconnected()
{
    qCInfo(tciaudioinput_js8)
        << "TCI audio input disconnected"
        << "stopping =" << m_stopping;

    m_started = false;
    m_suspended = false;

    if (m_stopping) {
        Q_EMIT status(tr("Stopped"));
        return;
    }

    Q_EMIT status(tr("Disconnected"));
}

void TCIAudioInput::handle_error(QString message)
{
    qCWarning(tciaudioinput_js8)
        << "TCI audio input error:" << message;

    m_started = false;

    if (m_stopping) {
        Q_EMIT status(tr("Error"));
        return;
    }
}

void TCIAudioInput::create_client()
{
    if (m_client)
        return;

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                create_client();
            },
            Qt::QueuedConnection);
        return;
    }

    if (!m_session) {
        Q_EMIT error(tr("TCI session is not available."));
        return;
    }

    m_client = m_session->client();

    qCInfo(tciaudioinput_js8)
        << "Created TCI audio client"
        << "client =" << m_client
        << "clientThread =" << m_client->thread()
        << "ownerThread =" << thread()
        << "currentThread =" << QThread::currentThread();


    connect(m_client, &TCIClient::ready,
            this,
            &TCIAudioInput::handle_ready,
            Qt::QueuedConnection);

    connect(m_client, &TCIClient::disconnected,
            this,
            &TCIAudioInput::handle_disconnected,
            Qt::QueuedConnection);

    connect(m_client, &TCIClient::rxAudioFrame,
            this,
            &TCIAudioInput::handle_rx_audio_frame,
            Qt::QueuedConnection);

    connect(m_client, &TCIClient::error,
            this,
            &TCIAudioInput::handle_error,
            Qt::QueuedConnection);
}


void TCIAudioInput::destroy_client()
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                destroy_client();
            },
            Qt::QueuedConnection);
        return;
    }

    if (!m_client)
        return;

    TCIClient *client = m_client;
    m_client = nullptr;

    disconnect(client, nullptr, this, nullptr);

    client->stopRxAudio();
}

Q_LOGGING_CATEGORY(tciaudioinput_js8, "tciaudioinput.js8", QtWarningMsg)