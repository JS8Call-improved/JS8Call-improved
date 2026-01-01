// Diagnostic harness for JS8 frequency/timing tracking.
// Synthesizes a Mode A frame with known small frequency/timing offsets and AWGN,
// runs the decoder twice (tracking disabled vs enabled), and prints outcomes.
//
// Build example (adjust Qt/FFTW include/library paths as needed):
//   g++ -std=c++17 -O2 -I.. tools/tracking_diag.cpp kalman.cpp -lQt5Core -lfftw3f -lpthread
//
// Notes:
// - This is a standalone diagnostic; it defines the globals (dec_data, specData,
//   fftw_mutex) that JS8.cpp expects.
// - Tracking is controlled via env vars: JS8_DISABLE_FREQ_TRACKING,
//   JS8_DISABLE_TIMING_TRACKING.

#include <cmath>
#include <cstring>
#include <optional>
#include <random>
#include <sstream>
#include <vector>
#include <iostream>
#include <mutex>
#include <thread>
#include <cstdlib>

#include <QCoreApplication>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>

#include "JS8_Include/commons.h"
#include "JS8_Mode/JS8.h"

// Provide the globals expected by JS8.cpp
struct dec_data dec_data;
struct specData specData;
std::mutex fftw_mutex;

Q_LOGGING_CATEGORY(decoder_js8, "decoder.js8", QtWarningMsg)

// Include implementation (in the diagnostic binary only).
#include "../JS8_Mode/JS8.cpp"

namespace
{
    using Mode = ModeA; // Fixed to Mode A for this diagnostic.

    struct DecodeMetrics
    {
        bool  decoded      = false;
        int   decodedCount = 0;
        float snr          = -99.0f;
        std::optional<double> refinedHz;
        std::optional<double> refinedDt;
    };

    struct SynthConfig
    {
        double snrDb          = -6.0;
        double freqOffsetHz   = 0.7;
        double timingOffsetSm = 0.4;  // samples (downsampled domain)
        double timingDriftSm  = 0.02; // samples per symbol
    };

    std::size_t
    synth_frame(SynthConfig const & cfg)
    {
        constexpr char message[] = "TESTTEST1234"; // 12 chars

        int tones[NN] = {};
        JS8::encode(0, Costas, message, tones);

        constexpr double fs   = 12000.0;
        constexpr double baud = fs / Mode::NSPS;
        constexpr double f0   = 1000.0; // Hz

        std::vector<float> samples(Mode::NMAX, 0.0f);

        double snrLin   = std::pow(10.0, cfg.snrDb / 10.0);
        double noiseVar = (snrLin > 0.0) ? (1.0 / snrLin) : 1.0;
        std::mt19937 rng(0xBEEF);
        std::normal_distribution<double> noise(0.0, std::sqrt(noiseVar));

        for (int sym = 0; sym < NN; ++sym)
        {
            double freq = f0 + tones[sym] * baud + cfg.freqOffsetHz;
            double dphi = 2.0 * M_PI * freq / fs;
            double phi  = 0.0;

            double timingShift = cfg.timingOffsetSm + cfg.timingDriftSm * sym;

            for (int n = 0; n < Mode::NSPS; ++n)
            {
                // Apply timing shift by advancing sample time.
                double t   = (sym * Mode::NSPS + n + timingShift) / fs;
                double s   = std::cos(2.0 * M_PI * freq * t + phi);
                phi        = std::fmod(phi + dphi, 2.0 * M_PI);

                std::size_t idx = sym * Mode::NSPS + n;
                if (idx < samples.size()) samples[idx] = static_cast<float>(s + noise(rng));
            }
        }

        auto const count = std::min(samples.size(), std::size_t(JS8_RX_SAMPLE_SIZE));
        for (std::size_t i = 0; i < count; ++i)
        {
            dec_data.d2[i] = static_cast<std::int16_t>(std::round(samples[i] * 2000.0));
        }

        dec_data.params.kin   = static_cast<int>(count);
        dec_data.params.kposA = 0;
        dec_data.params.kszA  = static_cast<int>(count);
        dec_data.params.nsubmodes = 1 << 0; // ModeA only.
        dec_data.params.nfa = 0;
        dec_data.params.nfb = 4000;
        dec_data.params.nfqso = static_cast<int>(f0);
        dec_data.params.syncStats = false;
        dec_data.params.newdat = true;
        dec_data.params.kposB = dec_data.params.kszB = 0;
        dec_data.params.kposC = dec_data.params.kszC = 0;
        dec_data.params.kposE = dec_data.params.kszE = 0;
        dec_data.params.kposI = dec_data.params.kszI = 0;
        dec_data.params.nutc = code_time(0,0,0);
        return count;
    }

    void
    set_tracking_env(bool enable)
    {
        if (enable)
        {
            ::unsetenv("JS8_DISABLE_FREQ_TRACKING");
            ::unsetenv("JS8_DISABLE_TIMING_TRACKING");
        }
        else
        {
            ::setenv("JS8_DISABLE_FREQ_TRACKING", "1", 1);
            ::setenv("JS8_DISABLE_TIMING_TRACKING", "1", 1);
        }
    }

    DecodeMetrics
    run_decode(bool enableTracking)
    {
        set_tracking_env(enableTracking);

        // Capture refined offsets from debug logs.
        std::optional<double> refinedHz;
        std::optional<double> refinedDt;

        auto handler = [&](QtMsgType type, QMessageLogContext const & ctx, QString const & msg)
        {
            Q_UNUSED(type);
            if (std::strcmp(ctx.category, "decoder.js8") != 0) return;

            std::string s = msg.toStdString();
            std::istringstream iss(s);
            std::string tok;
            while (iss >> tok)
            {
                if (tok == "refinedHz")
                {
                    double v;
                    if (iss >> v) refinedHz = v;
                }
                else if (tok == "refinedDt")
                {
                    double v;
                    if (iss >> v) refinedDt = v;
                }
            }
            std::cerr << s << "\n";
        };

        auto prevHandler = qInstallMessageHandler(
            +[](QtMsgType type, QMessageLogContext const & ctx, QString const & msg)
            {
                // Placeholder; replaced below in the event loop to capture metrics.
                Q_UNUSED(type); Q_UNUSED(ctx); Q_UNUSED(msg);
            });

        QLoggingCategory::setFilterRules(QStringLiteral("decoder.js8.debug=true\n"));

        DecodeMetrics r;
        JS8::Decoder decoder;
        QEventLoop loop;

        QObject::connect(&decoder, &JS8::Decoder::decodeEvent,
                         [&r, &loop](JS8::Event::Variant const & ev)
        {
            if (auto dec = std::get_if<JS8::Event::Decoded>(&ev))
            {
                r.decoded = true;
                r.snr     = dec->snr;
            }
            else if (auto fin = std::get_if<JS8::Event::DecodeFinished>(&ev))
            {
                r.decodedCount = static_cast<int>(fin->decoded);
                loop.quit();
            }
        });

        decoder.start(QThread::LowestPriority);
        decoder.decode();

        // Replace handler now that the decoder thread is running.
        qInstallMessageHandler(
            +[handler](QtMsgType type, QMessageLogContext const & ctx, QString const & msg)
            {
                handler(type, ctx, msg);
            });

        loop.exec();
        decoder.quit();

        qInstallMessageHandler(prevHandler);

        r.refinedHz = refinedHz;
        r.refinedDt = refinedDt;
        return r;
    }

    void
    run_once(SynthConfig const & cfg)
    {
        synth_frame(cfg);

        auto legacy = run_decode(false);
        auto track  = run_decode(true);

        std::cout << "SNR(dB)=" << cfg.snrDb
                  << " freqOff=" << cfg.freqOffsetHz
                  << " timOffSmpl=" << cfg.timingOffsetSm
                  << " timDriftSmpl=" << cfg.timingDriftSm
                  << "\n";

        std::cout << "Legacy: decoded=" << legacy.decoded
                  << " decodedCount=" << legacy.decodedCount
                  << " SNR=" << legacy.snr
                  << " refinedHz=" << (legacy.refinedHz ? std::to_string(*legacy.refinedHz) : "n/a")
                  << " refinedDt=" << (legacy.refinedDt ? std::to_string(*legacy.refinedDt) : "n/a")
                  << "\n";

        std::cout << "Track : decoded=" << track.decoded
                  << " decodedCount=" << track.decodedCount
                  << " SNR=" << track.snr
                  << " refinedHz=" << (track.refinedHz ? std::to_string(*track.refinedHz) : "n/a")
                  << " refinedDt=" << (track.refinedDt ? std::to_string(*track.refinedDt) : "n/a")
                  << "\n";
    }
}

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    SynthConfig cfg;
    bool sweep = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--sweep") sweep = true;
        else if (arg.rfind("--snr=", 0) == 0) cfg.snrDb = std::stod(arg.substr(6));
        else if (arg.rfind("--foff=", 0) == 0) cfg.freqOffsetHz = std::stod(arg.substr(7));
    }

    if (!sweep)
    {
        run_once(cfg);
        return 0;
    }

    for (double snr : {-12.0, -10.0, -8.0, -6.0, -4.0, -2.0, 0.0})
    {
        cfg.snrDb = snr;
        run_once(cfg);
    }

    return 0;
}
