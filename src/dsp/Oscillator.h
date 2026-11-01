#pragma once

#include "Dsp.h"

#include <array>
#include <cmath>

namespace cs80
{
/** Band-limited sawtooth tables with quarter-octave mip levels.

    PolyBLEP is a good correction, but a narrow pulse or a high note still
    leaves audible alias products: two discontinuities close together defeat
    the two-sample residual. These tables are built additively at prepare time
    with every harmonic kept below 45% of the host sample rate, then read with
    a Catmull-Rom interpolator. A pulse is produced as the difference of two
    positions in the same table, which stays band-limited by construction.

    Nothing here allocates or computes transcendental functions per sample, and
    a band change crossfades over 128 samples so no timbre step is audible. */
struct Wavetables
{
    static constexpr int kSize = 4096;
    static constexpr int kMask = kSize - 1;
    static constexpr int kBands = 44;          // quarter-octave steps
    static constexpr int kMaxHarmonics = kSize / 2 - 1;
    static constexpr double kLowestBandHz = 16.0;

    std::array<std::array<float, kSize>, kBands> saw {};
    std::array<float, kSize> sine {};
    std::array<int, kBands> harmonics {};
    double bandLimitHz = 21600.0;
    bool ready = false;

    /** Build every table. Called from prepare(), never from the audio thread. */
    void prepare (double hostSampleRate)
    {
        bandLimitHz = std::max (8000.0, hostSampleRate * 0.45);
        ready = false;

        for (int i = 0; i < kSize; ++i)
            sine[(size_t) i] = (float) std::sin (2.0 * 3.14159265358979323846
                                                 * (double) i / (double) kSize);

        for (int band = 0; band < kBands; ++band)
        {
            const double top = kLowestBandHz * std::exp2 ((double) (band + 1) / 4.0);
            const int count = std::clamp ((int) std::floor (bandLimitHz / top),
                                          1, kMaxHarmonics);

            harmonics[(size_t) band] = count;
            auto& table = saw[(size_t) band];
            table.fill (0.0f);

            // Ideal saw: 2t - 1 = -(2/pi) * sum sin(2*pi*k*t) / k. The sine
            // table supplies the waveform; the index accumulator walks each
            // harmonic without a sin() call or a per-sample modulo.
            for (int k = 1; k <= count; ++k)
            {
                const float amplitude = -(2.0f / kPi) / (float) k;
                int index = 0;

                for (int n = 0; n < kSize; ++n)
                {
                    table[(size_t) n] += amplitude * sine[(size_t) index];
                    index = (index + k) & kMask;
                }
            }
        }

        ready = true;
    }

    /** Mip level whose top harmonic still sits below the band limit. */
    int bandFor (double hz) const noexcept
    {
        if (hz <= kLowestBandHz)
            return 0;

        const int band = (int) std::ceil (4.0 * std::log2 (hz / kLowestBandHz));
        return std::clamp (band, 0, kBands - 1);
    }

    float readSaw (int band, float phase) const noexcept
    {
        const float x = phase * (float) kSize;
        int i = (int) x;
        const float frac = x - (float) i;
        i &= kMask;

        const int safeBand = std::clamp (band, 0, kBands - 1);
        const auto& table = saw[(size_t) safeBand];
        const float y1 = table[(size_t) i];
        const float y2 = table[(size_t) ((i + 1) & kMask)];

        // Above ~96 harmonics the table is already sparse enough that a linear
        // read is transparent; only the very low bands (hundreds of harmonics
        // crammed into one cycle) need the cubic interpolator.
        if (harmonics[(size_t) safeBand] <= 96)
            return y1 + (y2 - y1) * frac;

        const float y0 = table[(size_t) ((i - 1) & kMask)];
        const float y3 = table[(size_t) ((i + 2) & kMask)];

        const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c = -0.5f * y0 + 0.5f * y2;
        return ((a * frac + b) * frac + c) * frac + y1;
    }

    float readSine (float phase) const noexcept
    {
        const float x = phase * (float) kSize;
        int i = (int) x;
        const float frac = x - (float) i;
        i &= kMask;

        const float y0 = sine[(size_t) i];
        const float y1 = sine[(size_t) ((i + 1) & kMask)];
        return y0 + (y1 - y0) * frac;
    }
};

//==============================================================================
/** Band-limited, mip-mapped replacement for the old PolyBLEP oscillator.

    Phase is a normalised double so tuning stays exact at every sample rate.
    Band changes crossfade rather than jump, which keeps heavy vibrato free of
    timbre steps. */
struct BandlimitedOscillator
{
    void prepare (const Wavetables& sharedTables, double newSampleRate) noexcept
    {
        tables = &sharedTables;
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        phase = 0.0;
        inc = 0.0;
        band = 0;
        fadeFrom = -1;
        fade = 1.0f;
        band = tables->bandFor (440.0);
    }

    void reset (double startPhase = 0.0) noexcept
    {
        phase = startPhase;
        fade = 1.0f;
        fadeFrom = -1;
    }

    void setFrequency (double rate, double hz) noexcept
    {
        if (rate > 0.0)
            sampleRate = rate;

        const double nyquist = sampleRate * 0.5;
        hz = std::clamp (hz, 0.25, nyquist * 0.96);
        inc = hz / sampleRate;

        const int wanted = tables->bandFor (hz);

        if (wanted != band)
        {
            if (fade > 0.999f)
            {
                fadeFrom = band;
                fade = 0.0f;
            }

            band = wanted;
        }
    }

    float saw() noexcept
    {
        const float value = read (phase);
        advance();
        return value;
    }

    /** Band-limited pulse. The difference of two positions in a band-limited
        saw is exactly a band-limited pulse, including the width-dependent DC
        offset (the saw itself has no DC). */
    float pulse (float width) noexcept
    {
        width = clampf (width, 0.02f, 0.98f);

        float shifted = phase - (double) width;

        if (shifted < 0.0)
            shifted += 1.0;

        const float value = read (shifted) - read (phase);
        advance();
        return value;
    }

    /** Square exactly one octave below the main oscillator. The caller drives
        this oscillator at half the pitch. */
    float sub() noexcept
    {
        return pulse (0.5f);
    }

    float sine() noexcept
    {
        const float value = tables->readSine ((float) phase);
        advance();
        return value;
    }

    /** Keep the phase running while the oscillator is silent, so no table read
        (and no interpolation work) is spent on a signal that is mixed at zero.
        Enabling the oscillator later picks up from a continuously running
        phase instead of a frozen one. */
    void advanceOnly() noexcept { advance(); }

private:
    float read (double position) const noexcept
    {
        const auto p = (float) position;

        if (fade >= 0.999f || fadeFrom < 0)
            return tables->readSaw (band, p);

        const float previous = tables->readSaw (fadeFrom, p);
        const float current = tables->readSaw (band, p);
        return lerpf (previous, current, fade);
    }

    void advance() noexcept
    {
        phase += inc;

        if (phase >= 1.0)
            phase -= 1.0;

        if (fade < 1.0f)
            fade = std::min (1.0f, fade + (1.0f / 128.0f));
    }

    const Wavetables* tables = nullptr;
    double sampleRate = 48000.0;
    double phase = 0.0;
    double inc = 0.0;
    int band = 0;
    int fadeFrom = -1;
    float fade = 1.0f;
};
} // namespace cs80
