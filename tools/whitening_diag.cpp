// Diagnostic harness for JS8 whitening / noise estimation.
// This is a standalone command-line tool that synthesizes a Mode A frame,
// runs the decoder twice (whitening OFF vs ON), and prints a concise summary.
//
// Build example (adjust Qt/FFTW paths as needed):
//   g++ -std=c++17 -O2 -I.. tools/whitening_diag.cpp kalman.cpp -lQt5Core -lfftw3f -lpthread
//
// Note: this links only the pieces needed for decoding; it defines the
// globals (dec_data, specData, fftw_mutex) that JS8 expects.

#include <cmath>
#include <cstring>
#include <random>
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

    std::size_t
    synth_frame(double snrDb)
    {
        constexpr char message[] = "TESTTEST1234"; // 12 chars

        int tones[NN] = {};
        JS8::encode(0, Costas, message, tones);

        constexpr double fs   = 12000.0;
        constexpr double baud = fs / Mode::NSPS;
        constexpr double f0   = 1000.0; // Hz

        std::vector<float> samples(Mode::NMAX, 0.0f);

        double snrLin   = std::pow(10.0, snrDb / 10.0);
        double noiseVar = (snrLin > 0.0) ? (1.0 / snrLin) : 1.0;
        std::mt19937 rng(0xBEEF);
        std::normal_distribution<double> noise(0.0, std::sqrt(noiseVar));

        for (int sym = 0; sym < NN; ++sym)
        {
            double freq = f0 + tones[sym] * baud;
            double dphi = 2.0 * M_PI * freq / fs;
            double phi  = 0.0;

            for (int n = 0; n < Mode::NSPS; ++n)
            {
                double s = std::cos(phi);
                phi = std::fmod(phi + dphi, 2.0 * M_PI);
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

    struct Result
    {
        bool decoded = false;
        int  nhard   = -1;
        float snr    = -99.0f;
    };

    Result
    run_decode(bool disableWhitening)
    {
        if (disableWhitening) {
            ::setenv("JS8_DISABLE_WHITENING", "1", 1);
        } else {
            ::unsetenv("JS8_DISABLE_WHITENING");
        }

        Result r;
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
                // We don't have iterations from the decoder; use decoded count.
                r.nhard = static_cast<int>(fin->decoded);
                loop.quit();
            }
        });

        decoder.start(QThread::LowestPriority);
        decoder.decode();
        loop.exec();
        decoder.quit();

        return r;
    }
}

int
main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    constexpr double snrDb = 0.0;
    synth_frame(snrDb);

    auto off = run_decode(true);
    auto on  = run_decode(false);

    std::cout << "Whitening OFF: decoded=" << off.decoded
              << " iterations=" << off.nhard
              << " metric=" << off.snr << "\n";

    std::cout << "Whitening ON:  decoded=" << on.decoded
              << " iterations=" << on.nhard
              << " metric=" << on.snr << "\n";

    return 0;
}
