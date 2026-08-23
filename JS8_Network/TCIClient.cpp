#include "TCIClient.h"
#include "TCIStreamFrame.h"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QVersionNumber>

#include <QtEndian>
#include <QtMath>

#include <cmath>
#include <cstring>

#include <qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(tcinetworking_js8)

namespace
{
    qint16 clampFloatToInt16(float sample)
    {
        if (!std::isfinite(sample))
            return 0;

        sample = qBound(-1.0f, sample, 1.0f);

        return static_cast<qint16>(
            qRound(sample * 32767.0f)
        );
    }

    qint16 readInt16Le(char const *p)
    {
        return qFromLittleEndian<qint16>(
            reinterpret_cast<uchar const *>(p)
        );
    }

    float readFloat32Le(char const *p)
    {
        quint32 const bits = qFromLittleEndian<quint32>(
            reinterpret_cast<uchar const *>(p)
        );

        float value = 0.0f;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    QByteArray convertRxAudioToMonoInt16(TCIStream::StreamFrame const &frame)
    {
        if (frame.channels != 1 && frame.channels != 2) {
            qWarning() << "Unsupported TCI RX channel count:"
                       << frame.channels;
            return {};
        }

        if (frame.sampleCount == 0)
            return {};

        QByteArray out;
        out.resize(static_cast<qsizetype>(frame.sampleCount) *
                   qsizetype(sizeof(qint16)));

        auto *dest = reinterpret_cast<qint16 *>(out.data());

        switch (frame.sampleType) {
        case TCIStream::INT16: {
            qsizetype const expectedBytes =
                qsizetype(frame.sampleCount) *
                qsizetype(frame.channels) *
                qsizetype(sizeof(qint16));

            if (frame.payload.size() < expectedBytes) {
                qWarning() << "Short TCI RX int16 payload:"
                           << frame.payload.size()
                           << "expected"
                           << expectedBytes;
                return {};
            }

            char const *src = frame.payload.constData();

            for (quint32 i = 0; i < frame.sampleCount; ++i) {
                if (frame.channels == 1) {
                    dest[i] = readInt16Le(src);
                    src += sizeof(qint16);
                } else {
                    qint32 const left = readInt16Le(src);
                    src += sizeof(qint16);

                    qint32 const right = readInt16Le(src);
                    src += sizeof(qint16);

                    dest[i] = static_cast<qint16>((left + right) / 2);
                }
            }

            return out;
        }

        case TCIStream::FLOAT32: {
            qsizetype const expectedBytes =
                qsizetype(frame.sampleCount) *
                qsizetype(frame.channels) *
                qsizetype(sizeof(float));

            if (frame.payload.size() < expectedBytes) {
                qWarning() << "Short TCI RX float32 payload:"
                           << frame.payload.size()
                           << "expected"
                           << expectedBytes;
                return {};
            }

            char const *src = frame.payload.constData();

            for (quint32 i = 0; i < frame.sampleCount; ++i) {
                if (frame.channels == 1) {
                    float const sample = readFloat32Le(src);
                    src += sizeof(float);

                    dest[i] = clampFloatToInt16(sample);
                } else {
                    float const left = readFloat32Le(src);
                    src += sizeof(float);

                    float const right = readFloat32Le(src);
                    src += sizeof(float);

                    dest[i] = clampFloatToInt16((left + right) * 0.5f);
                }
            }

            return out;
        }

        default:
            qWarning() << "Unsupported TCI RX sample type:"
                       << frame.sampleType;
            return {};
        }
    }

void appendFloat32Le(QByteArray &out, float value)
    {
        quint32 bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));

        bits = qToLittleEndian(bits);

        out.append(reinterpret_cast<char const *>(&bits), sizeof(bits));
    }

    float int16ToFloat(qint16 sample)
    {
        return qBound(-1.0f, float(sample) / 32768.0f, 1.0f);
    }

    QByteArray convertMonoInt16ToFloat32(QByteArray const &monoInt16,
                                     int outputChannels)
    {
        if (outputChannels < 1)
            return {};

        int const inputSamples =
            monoInt16.size() / int(sizeof(qint16));

        auto const *src =
            reinterpret_cast<qint16 const *>(monoInt16.constData());

        QByteArray out;
        out.reserve(inputSamples * outputChannels * int(sizeof(float)));

        for (int i = 0; i < inputSamples; ++i) {
            float const sample = int16ToFloat(qFromLittleEndian<qint16>(
                reinterpret_cast<uchar const *>(&src[i])));

            for (int ch = 0; ch < outputChannels; ++ch)
                appendFloat32Le(out, sample);
        }

        return out;
    }

    quint32 txHeaderSampleCount(TciTxAudioPolicy const &policy,
                            int monoInputSamples)
    {
        if (policy.sampleCountMeaning ==
            TciTxAudioPolicy::SampleCountMeaning::TotalSampleValues) {
            return quint32(monoInputSamples * policy.channels);
            }

        return quint32(monoInputSamples);
    }
}

TCIClient::TCIClient(QObject *parent)
    : QObject(parent)
{
}

void TCIClient::createSocket()
{
    if (socket_)
        return;

    socket_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(socket_, &QWebSocket::connected,
            this, &TCIClient::onConnected);

    connect(socket_, &QWebSocket::disconnected,
            this, &TCIClient::onDisconnected);

    connect(socket_, &QWebSocket::textMessageReceived,
            this, &TCIClient::onTextMessageReceived);

    connect(socket_, &QWebSocket::binaryMessageReceived,
            this, &TCIClient::onBinaryMessageReceived);

    connect(socket_,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this,
            &TCIClient::onErrorOccurred);
}

void TCIClient::destroySocket()
{
    if (!socket_)
        return;

    QWebSocket *socket = socket_;
    socket_ = nullptr;

    bool disconnected = socket->disconnect(this);
    Q_UNUSED(disconnected);

    socket->deleteLater();
}

void TCIClient::connectToServer(QUrl const &url)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, url]() {
                connectToServer(url);
            },
            Qt::QueuedConnection);
        return;
    }

    if (url_ == url && (connected_ || connecting_)) {
        qCInfo(tcinetworking_js8) << "TCI connection already active or pending:"
                << "client=" << this
                << "url=" << url;

        emitReadyIfReady();
        return;
    }

    connected_ = false;
    connecting_ = true;
    ready_ = false;
    url_ = url;

    resetProtocolState();

    destroySocket();
    createSocket();

    qCInfo(tcinetworking_js8) << "Opening TCI connection:"
            << "client=" << this
            << "url=" << url;

    socket_->open(url);
}

void TCIClient::disconnectFromServer()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                disconnectFromServer();
            },
            Qt::QueuedConnection
        );
        return;
    }

    if (socket_)
        socket_->close();
}

bool TCIClient::isConnected() const
{
    return connected_;
}

bool TCIClient::isReady() const
{
    return ready_;
}

void TCIClient::setFrequency(qint64 hz)
{
    sendText(QStringLiteral("vfo:0,0,%1;").arg(hz));
}

void TCIClient::queryFrequency()
{
    sendText("vfo;");
}

void TCIClient::setTxFrequency(qint64 hz)
{
    sendText(QStringLiteral("vfo:0,1,%1;").arg(hz));
}

void TCIClient::queryTxFrequency()
{
    sendText("vfo:0,1;");
}

void TCIClient::setSplit(bool enabled)
{
    sendText(QStringLiteral("split_enable:0,%1;")
                 .arg(enabled ? "true" : "false"));
}

void TCIClient::querySplit()
{
    sendText("split_enable:0;");
}

void TCIClient::setMode(const QString &mode)
{
    sendText(QStringLiteral("modulation:0,0,%1;").arg(mode.trimmed().toLower()));
}

void TCIClient::queryMode()
{
    sendText("modulation;");
}

void TCIClient::setPtt(bool enabled)
{
    sendText(QStringLiteral("trx:0,%1;").arg(enabled ? "true" : "false"));
}

void TCIClient::queryPtt()
{
    sendText("trx;");
}

void TCIClient::configureAudio(int sampleRate, int channels, int samplesPerFrame)
{
    audio_sample_rate_ = sampleRate;
    audio_channels_ = channels;
    audio_samples_per_frame_ = samplesPerFrame;

    sendText("audio_stream_sample_type:int16;");
    sendText(QStringLiteral("audio_stream_channels:%1;").arg(audio_channels_));
    sendText(QStringLiteral("audio_stream_samples:%1;").arg(audio_samples_per_frame_));
    sendText(QStringLiteral("audio_samplerate:%1;").arg(audio_sample_rate_));
}

void TCIClient::startRxAudio()
{
    sendText("audio_start:0;");
}

void TCIClient::stopRxAudio()
{
    sendText("audio_stop:0;");
}

void TCIClient::startTxAudio()
{
    sendText("tx_audio_start:0;");
}

void TCIClient::stopTxAudio()
{
    sendText("tx_audio_stop:0;");
}

void TCIClient::sendTxAudio(QByteArray const &monoInt16Pcm,
                            TciTxAudioPolicy const &policy)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, monoInt16Pcm, policy]() {
                sendTxAudio(monoInt16Pcm, policy);
            },
            Qt::QueuedConnection);
        return;
    }

    if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState)
        return;

    if (monoInt16Pcm.isEmpty())
        return;

    if (monoInt16Pcm.size() % qsizetype(sizeof(qint16))) {
        qWarning() << "Ignoring unaligned TCI TX PCM block:"
                   << monoInt16Pcm.size();
        return;
    }

    int const monoInputSamples =
        monoInt16Pcm.size() / int(sizeof(qint16));

    QByteArray payload;
    quint32 headerSampleCount = 0;

    switch (policy.sampleType) {
    case TCIStream::INT16:
        if (policy.channels != 1) {
            qWarning() << "Unsupported TCI TX int16 channel count:"
                       << policy.channels;
            return;
        }

        payload = monoInt16Pcm;
        headerSampleCount =
            txHeaderSampleCount(policy, monoInputSamples);
        break;

    case TCIStream::FLOAT32:
        payload =
            convertMonoInt16ToFloat32(monoInt16Pcm, policy.channels);

        if (payload.isEmpty())
            return;

        headerSampleCount =
            txHeaderSampleCount(policy, monoInputSamples);
        break;

    default:
        qWarning() << "Unsupported TCI TX sample type:"
                   << policy.sampleType;
        return;
    }

    QByteArray const frame =
        TCIStream::makeTxAudioFrameWithSampleCount(
            payload,
            headerSampleCount,
            0,
            quint32(policy.sampleRate),
            quint32(policy.sampleType),
            quint32(policy.channels));

    if (frame.size() <= 64) {
        qWarning() << "Refusing to send empty TCI TX audio frame:"
                   << "payloadBytes=" << payload.size()
                   << "frameBytes=" << frame.size();
        return;
    }

    if (!canSend())
        return;

    qCInfo(tcinetworking_js8).noquote()
    << "TCI TX binary:"
    << "stream=" << tciStreamTypeName(TCIStream::TX_AUDIO_STREAM)
    << "receiver=" << 0
    << "rate=" << policy.sampleRate
    << "type=" << tciSampleTypeName(policy.sampleType)
    << "sampleCount=" << headerSampleCount
    << "channels=" << policy.channels
    << "payloadBytes=" << payload.size()
    << "messageBytes=" << frame.size()
    << "sampleCountMeaning="
    << tciSampleCountMeaningName(policy.sampleCountMeaning);

    socket_->sendBinaryMessage(frame);
}

void TCIClient::sendTxAudioMonoInt16(QByteArray const &pcm)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, pcm]() {
                sendTxAudioMonoInt16(pcm);
            },
            Qt::QueuedConnection);
        return;
    }

    if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState)
        return;

    if (pcm.isEmpty())
        return;

    if (pcm.size() % qsizetype(sizeof(qint16))) {
        qWarning() << "Ignoring unaligned TCI TX audio PCM block:"
                   << pcm.size();
        return;
    }

    QByteArray const frame =
        TCIStream::makeTxAudioFrame(pcm,
                                    0,     // receiver
                                    48000, // sample rate
                                    TCIStream::INT16,
                                    1);    // mono

    if (!canSend())
        return;

    socket_->sendBinaryMessage(frame);
}

void TCIClient::onConnected()
{
    connected_ = true;
    connecting_ = false;

    qCInfo(tcinetworking_js8) << "Connected to TCI server"
            << "client=" << this;

    Q_EMIT connected();
}

void TCIClient::onDisconnected()
{
    connected_ = false;
    connecting_ = false;
    ready_ = false;

    qCInfo(tcinetworking_js8) << "Disconnected from TCI server"
            << "client=" << this;

    destroySocket();

    Q_EMIT disconnected();
}

void TCIClient::onTextMessageReceived(const QString &message)
{
    qCInfo(tcinetworking_js8).noquote() << "TCI RX text:" << message;
    parseTextMessage(message);

    ensureProtocolHandler();

    const QString name = commandName(message);
    const QStringList args = commandArgs(message);

    if (protocol_handler_)
        protocol_handler_->handleTextCommand(name,
                                             args,
                                             server_info_,
                                             capabilities_);

    if (name == "protocol")
        selectProtocolHandler();

    if (!server_info_logged_ &&
        !server_info_.programName.isEmpty() &&
        !server_info_.protocolVersion.isEmpty()) {
        logServerInfo();
        server_info_logged_ = true;
        }

    logCapabilities();
    logTxPolicy();
}

void TCIClient::onBinaryMessageReceived(QByteArray const &message)
{
    TCIStream::StreamFrame const frame = TCIStream::parseFrame(message);

    if (!frame.valid) {
        qWarning() << "Invalid TCI binary frame:" << message.size();
        return;
    }

    if (frame.streamType != TCIStream::RX_AUDIO_STREAM)
        logBinaryFrame(QStringLiteral("RX"), frame, message.size());

    ensureProtocolHandler();

    if (protocol_handler_)
        protocol_handler_->handleBinaryFrame(frame, capabilities_);

    if (frame.streamType == TCIStream::TX_CHRONO) {
        logCapabilities();
        logTxPolicy();

        if (protocol_handler_) {
            TciTxAudioPolicy const policy =
                protocol_handler_->deriveTxAudioPolicy(server_info_, capabilities_);

            Q_EMIT txChronoRequested(policy);
        }

        return;
    }

    if (frame.streamType != TCIStream::RX_AUDIO_STREAM) {
        qCInfo(tcinetworking_js8) << "Ignoring non-RX TCI stream type:"
                << tciStreamTypeName(frame.streamType);
        return;
    }

    if (frame.sampleRate != 48000) {
        qWarning() << "Unsupported TCI RX sample rate:" << frame.sampleRate;
        return;
    }

    QByteArray const monoInt16 =
    convertRxAudioToMonoInt16(frame);

    if (monoInt16.isEmpty())
        return;

    qsizetype const expected_bytes =
        static_cast<qsizetype>(frame.sampleCount) * 2;

    if (frame.payload.size() < expected_bytes) {
        qWarning() << "Short TCI RX audio payload:"
                   << frame.payload.size()
                   << "expected"
                   << expected_bytes;
        return;
    }

    Q_EMIT rxAudioFrame(monoInt16,
                        static_cast<int>(frame.sampleRate));
}

void TCIClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    QString const message =
        socket_ ? socket_->errorString()
                : tr("Unknown TCI socket error.");

    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        qInfo() << "TCI socket closed by remote host:"
                << message;

        Q_EMIT error(tr("TCI disconnected"));

        return;
    }

    qWarning() << "TCI socket error:" << message;
    Q_EMIT error(message);
}

void TCIClient::resetProtocolState()
{
    server_info_ = {};
    capabilities_ = {};
    protocol_handler_.reset();
    protocol_generation_ = TciProtocolGeneration::Unknown;

    server_info_logged_ = false;
    policy_logged_ = false;
}

void TCIClient::ensureProtocolHandler()
{
    if (protocol_handler_)
        return;

    protocol_handler_ = std::make_unique<TciProtocolUnknownHandler>();
    protocol_generation_ = TciProtocolGeneration::Unknown;
}

void TCIClient::selectProtocolHandler()
{
    QVersionNumber const version =
        QVersionNumber::fromString(server_info_.protocolVersion);

    TciProtocolGeneration next_generation =
        TciProtocolGeneration::Unknown;

    if (!version.isNull()) {
        QVersionNumber const modern_boundary(1, 10);

        if (version < modern_boundary)
            next_generation = TciProtocolGeneration::LegacyV1;
        else if (version.majorVersion() <= 2)
            next_generation = TciProtocolGeneration::V2;
        else
            next_generation = TciProtocolGeneration::Unknown;
    }

    if (protocol_handler_ && protocol_generation_ == next_generation)
        return;

    switch (next_generation) {
    case TciProtocolGeneration::LegacyV1:
        protocol_handler_ = std::make_unique<TciProtocolLegacyV1Handler>();
        break;

    case TciProtocolGeneration::V2:
        protocol_handler_ = std::make_unique<TciProtocolV2Handler>();
        break;

    case TciProtocolGeneration::Unknown:
        protocol_handler_ = std::make_unique<TciProtocolUnknownHandler>();
        break;
    }

    protocol_generation_ = next_generation;
    policy_logged_ = false;

    qCInfo(tcinetworking_js8).noquote()
        << "TCI protocol handler selected:"
        << tciGenerationName(protocol_generation_)
        << "program=" << server_info_.programName
        << "version=" << server_info_.protocolVersion;
}

void TCIClient::emitReadyIfReady()
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                emitReadyIfReady();
            },
            Qt::QueuedConnection);
        return;
    }

    if (ready_)
        Q_EMIT ready();
}

void TCIClient::logServerInfo() const
{
    qCInfo(tcinetworking_js8).noquote()
        << "TCI server info:"
        << "program=" << server_info_.programName
        << "protocol=" << server_info_.protocolVersion
        << "device=" << server_info_.device;
}

void TCIClient::logCapabilities() const
{
    qCInfo(tcinetworking_js8).noquote()
        << "TCI capabilities:"
        << "ready=" << capabilities_.readySeen
        << "start=" << capabilities_.startSeen
        << "receiveOnlySeen=" << capabilities_.receiveOnlySeen
        << "receiveOnly=" << capabilities_.receiveOnly
        << "txEnableSeen=" << capabilities_.txEnableSeen
        << "txEnabled=" << capabilities_.txEnabled
        << "txChronoSeen=" << capabilities_.txChronoSeen
        << "textAudioType="
        << (capabilities_.textAudio.sampleTypeSeen
                ? tciSampleTypeName(capabilities_.textAudio.sampleType)
                : QStringLiteral("unknown"))
        << "textAudioChannels="
        << (capabilities_.textAudio.channelsSeen
                ? QString::number(capabilities_.textAudio.channels)
                : QStringLiteral("unknown"))
        << "textAudioSamples="
        << (capabilities_.textAudio.samplesSeen
                ? QString::number(capabilities_.textAudio.samplesPerFrame)
                : QStringLiteral("unknown"))
        << "textAudioRate="
        << (capabilities_.textAudio.sampleRateSeen
                ? QString::number(capabilities_.textAudio.sampleRate)
                : QStringLiteral("unknown"))
        << "rxAudioType="
        << (capabilities_.rxAudio.sampleTypeSeen
                ? tciSampleTypeName(capabilities_.rxAudio.sampleType)
                : QStringLiteral("unknown"))
        << "rxAudioChannels="
        << (capabilities_.rxAudio.channelsSeen
                ? QString::number(capabilities_.rxAudio.channels)
                : QStringLiteral("unknown"))
        << "rxAudioSamples="
        << (capabilities_.rxAudio.samplesSeen
                ? QString::number(capabilities_.rxAudio.samplesPerFrame)
                : QStringLiteral("unknown"))
        << "rxAudioRate="
        << (capabilities_.rxAudio.sampleRateSeen
                ? QString::number(capabilities_.rxAudio.sampleRate)
                : QStringLiteral("unknown"))
        << "txChronoType="
        << (capabilities_.txChronoAudio.sampleTypeSeen
                ? tciSampleTypeName(capabilities_.txChronoAudio.sampleType)
                : QStringLiteral("unknown"))
        << "txChronoChannels="
        << (capabilities_.txChronoAudio.channelsSeen
                ? QString::number(capabilities_.txChronoAudio.channels)
                : QStringLiteral("unknown"))
        << "txChronoSamples="
        << (capabilities_.txChronoAudio.samplesSeen
                ? QString::number(capabilities_.txChronoAudio.samplesPerFrame)
                : QStringLiteral("unknown"))
        << "txChronoRate="
        << (capabilities_.txChronoAudio.sampleRateSeen
                ? QString::number(capabilities_.txChronoAudio.sampleRate)
                : QStringLiteral("unknown"));
}

void TCIClient::logTxPolicy()
{
    if (!protocol_handler_)
        return;

    TciTxAudioPolicy const policy =
        protocol_handler_->deriveTxAudioPolicy(server_info_, capabilities_);

    /*
     * Log repeatedly until something meaningful has been observed. After that,
     * only log when the policy is first derived after handler selection.
     */
    if (policy_logged_)
        return;

    qCInfo(tcinetworking_js8).noquote()
        << "TCI derived TX policy:"
        << "handler=" << tciGenerationName(protocol_generation_)
        << "timing=" << tciTxTimingName(policy.timing)
        << "sampleType=" << tciSampleTypeName(policy.sampleType)
        << "channels=" << policy.channels
        << "samplesPerFrame=" << policy.samplesPerFrame
        << "sampleRate=" << policy.sampleRate
        << "sampleCountMeaning="
        << tciSampleCountMeaningName(policy.sampleCountMeaning);

    policy_logged_ = true;
}

void TCIClient::logBinaryFrame(QString const &direction,
                               TCIStream::StreamFrame const &frame,
                               qsizetype messageBytes) const
{
    qCInfo(tcinetworking_js8).noquote()
        << "TCI" << direction << "binary:"
        << "stream=" << tciStreamTypeName(frame.streamType)
        << "receiver=" << frame.receiver
        << "rate=" << frame.sampleRate
        << "type=" << tciSampleTypeName(frame.sampleType)
        << "sampleCount=" << frame.sampleCount
        << "channels=" << frame.channels
        << "payloadBytes=" << frame.payload.size()
        << "messageBytes=" << messageBytes;
}

void TCIClient::sendText(QString const &message)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, message]() {
                sendText(message);
            },
            Qt::QueuedConnection);
        return;
    }

    if (!canSend())
        return;

    socket_->sendTextMessage(message);
}

bool TCIClient::canSend() const
{
    return socket_
        && socket_->state() == QAbstractSocket::ConnectedState;
}

void TCIClient::parseTextMessage(const QString &message)
{
    const QString name = commandName(message);
    const QStringList args = commandArgs(message);

    if (name == "ready") {
        if (!ready_) {
            ready_ = true;
            emit ready();
        }

        return;
    }

    if (name == "vfo" || name == "dds") {
        if (args.size() >= 3) {
            bool channelOk = false;
            const int channel = args.at(1).toInt(&channelOk);

            bool freqOk = false;
            const qint64 hz = args.at(2).toLongLong(&freqOk);

            if (freqOk) {
                if (channelOk && channel == 1)
                    emit txFrequencyChanged(hz);
                else
                    emit frequencyChanged(hz);
            }
        }

        return;
    }

    if (name == "split_enable") {
        if (args.size() >= 2) {
            const QString value = args.at(1).trimmed().toLower();

            emit splitChanged(
                value == "true" ||
                value == "1" ||
                value == "on" ||
                value == "tx"
            );
        }

        return;
    }

    if (name == "modulation" || name == "mode") {
        if (args.size() >= 3)
            emit modeChanged(args.at(2).trimmed().toUpper());

        return;
    }

    if (name == "trx" || name == "ptt") {
        if (args.size() >= 2) {
            const QString value = args.last().trimmed().toLower();

            emit pttChanged(
                value == "true" ||
                value == "1" ||
                value == "on" ||
                value == "tx"
            );
        }

        return;
    }
}

QString TCIClient::commandName(const QString &message)
{
    QString s = message.trimmed();

    if (s.endsWith(';'))
        s.chop(1);

    const int colon = s.indexOf(':');

    if (colon < 0)
        return s.trimmed().toLower();

    return s.left(colon).trimmed().toLower();
}

QStringList TCIClient::commandArgs(const QString &message)
{
    QString s = message.trimmed();

    if (s.endsWith(';'))
        s.chop(1);

    const int colon = s.indexOf(':');

    if (colon < 0)
        return {};

    const QString argString = s.mid(colon + 1).trimmed();

    if (argString.isEmpty())
        return {};

    QStringList out;

    const auto parts = argString.split(',', Qt::KeepEmptyParts);

    for (const QString &part : parts)
        out << part.trimmed();

    return out;
}

QString TCIClient::semicolon(QString message)
{
    message = message.trimmed();

    if (!message.endsWith(';'))
        message.append(';');

    return message;
}

Q_LOGGING_CATEGORY(tcinetworking_js8, "tcinetworking.js8", QtWarningMsg)