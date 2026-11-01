#pragma once

// Small, dependency-free spectral analysis helpers shared by the headless test
// suite and the analysis tool. Everything here runs offline; nothing in this
// header is used by the real-time engine.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace cs80test
{
inline constexpr double kPi = 3.14159265358979323846;

/** In-place iterative radix-2 FFT. `re` and `im` must have the same power of
    two length. */
inline void fft (std::vector<double>& re, std::vector<double>& im)
{
    const size_t n = re.size();
    if (n < 2 || (n & (n - 1)) != 0 || im.size() != n)
        return;

    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;

        for (; j & bit; bit >>= 1)
            j ^= bit;

        j ^= bit;

        if (i < j)
        {
            std::swap (re[i], re[j]);
            std::swap (im[i], im[j]);
        }
    }

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double angle = -2.0 * kPi / (double) len;
        const double wRe = std::cos (angle);
        const double wIm = std::sin (angle);

        for (size_t i = 0; i < n; i += len)
        {
            double curRe = 1.0, curIm = 0.0;

            for (size_t k = 0; k < len / 2; ++k)
            {
                const size_t a = i + k;
                const size_t b = a + len / 2;

                const double uRe = re[a], uIm = im[a];
                const double vRe = re[b] * curRe - im[b] * curIm;
                const double vIm = re[b] * curIm + im[b] * curRe;

                re[a] = uRe + vRe;
                im[a] = uIm + vIm;
                re[b] = uRe - vRe;
                im[b] = uIm - vIm;

                const double nextRe = curRe * wRe - curIm * wIm;
                curIm = curRe * wIm + curIm * wRe;
                curRe = nextRe;
            }
        }
    }
}

/** Single-sided peak-amplitude spectrum (linear), Blackman-Harris windowed. */
inline std::vector<double> amplitudeSpectrum (const std::vector<float>& samples,
                                              size_t fftSize)
{
    std::vector<double> re (fftSize, 0.0), im (fftSize, 0.0);

    const size_t count = std::min (fftSize, samples.size());
    const double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;

    for (size_t i = 0; i < count; ++i)
    {
        const double phase = 2.0 * kPi * (double) i / (double) (count - 1);
        const double window = a0 - a1 * std::cos (phase) + a2 * std::cos (2.0 * phase)
                            - a3 * std::cos (3.0 * phase);
        re[i] = (double) samples[i] * window;
    }

    fft (re, im);

    const double coherentGain = a0;
    std::vector<double> magnitude (fftSize / 2, 0.0);

    for (size_t i = 0; i < magnitude.size(); ++i)
        magnitude[i] = 2.0 * std::sqrt (re[i] * re[i] + im[i] * im[i])
                     / ((double) count * coherentGain);

    return magnitude;
}

struct AliasReport
{
    double fundamentalHz = 0.0;
    double fundamentalAmp = 0.0;
    double fundamentalDb = -200.0;
    double worstAliasDb = -200.0;    // single largest non-harmonic component
    double aliasEnergyDb = -200.0;   // total non-harmonic energy
    double noiseFloorDb = -200.0;    // median non-harmonic bin
    double worstAliasHz = 0.0;
    int harmonics = 0;
};

inline double db20 (double value)
{
    return value > 1.0e-12 ? 20.0 * std::log10 (value) : -240.0;
}

/** Measure how far non-harmonic (aliased / IM) components sit below the
    fundamental. Harmonic bins are excluded using a lobe that is wide enough for
    the Blackman-Harris main lobe and typical analogue drift. */
inline AliasReport analyseAliasing (const std::vector<float>& samples,
                                    double sampleRate, double fundamentalHz,
                                    int exclusionBins = 10)
{
    AliasReport report;
    report.fundamentalHz = fundamentalHz;

    size_t fftSize = 1;

    while (fftSize * 2 <= samples.size() && fftSize < 262144)
        fftSize *= 2;

    const auto magnitude = amplitudeSpectrum (samples, fftSize);
    const double binHz = sampleRate / (double) fftSize;
    const int binCount = (int) magnitude.size();

    if (binCount == 0 || fundamentalHz <= 0.0)
        return report;

    auto binOf = [&] (double hz)
    {
        return (int) std::lround (hz / binHz);
    };

    const int fundamentalBin = std::clamp (binOf (fundamentalHz), 1, binCount - 1);
    double fundamental = 0.0;

    for (int b = std::max (1, fundamentalBin - 3); b <= std::min (binCount - 1, fundamentalBin + 3); ++b)
        fundamental = std::max (fundamental, magnitude[(size_t) b]);

    report.fundamentalAmp = fundamental;
    report.fundamentalDb = db20 (fundamental);

    std::vector<bool> harmonicMask ((size_t) binCount, false);
    int harmonics = 0;

    for (int k = 1; k * fundamentalHz < sampleRate * 0.499; ++k)
    {
        const int centre = binOf ((double) k * fundamentalHz);

        if (centre >= binCount)
            break;

        ++harmonics;

        for (int b = std::max (0, centre - exclusionBins);
             b <= std::min (binCount - 1, centre + exclusionBins); ++b)
            harmonicMask[(size_t) b] = true;
    }

    report.harmonics = harmonics;

    // Ignore the DC / high-pass settling region and anything below 40 Hz.
    const int lowBin = std::max (1, binOf (40.0));

    double aliasPower = 0.0, harmonicReference = fundamental * fundamental;
    double worst = 0.0, worstHz = 0.0;
    std::vector<double> floorBins;

    for (int b = lowBin; b < binCount; ++b)
    {
        if (harmonicMask[(size_t) b])
            continue;

        const double value = magnitude[(size_t) b];
        aliasPower += value * value;
        floorBins.push_back (value);

        if (value > worst)
        {
            worst = value;
            worstHz = (double) b * binHz;
        }
    }

    if (fundamental > 0.0)
    {
        report.worstAliasDb = db20 (worst / fundamental);
        report.aliasEnergyDb = 10.0 * std::log10 (std::max (aliasPower, 1.0e-30) / harmonicReference);
    }

    if (! floorBins.empty())
    {
        std::sort (floorBins.begin(), floorBins.end());
        const double median = floorBins[floorBins.size() / 2];

        if (fundamental > 0.0)
            report.noiseFloorDb = db20 (median / fundamental);
    }

    report.worstAliasHz = worstHz;
    return report;
}
} // namespace cs80test
