#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace cs80
{
inline constexpr float kPi = 3.14159265358979323846f;

inline float clampf (float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerpf (float a, float b, float t) noexcept
{
    return a + (b - a) * t;
}

inline float flushDenormal (float v) noexcept
{
    return std::abs (v) < 1.0e-20f ? 0.0f : v;
}

/** Rational tanh approximation. Transparent in the audio band, and it keeps
    the filter/limiter stage free of libm calls on the audio thread. */
inline float fastTanh (float x) noexcept
{
    x = clampf (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/** Cubic soft clipper. Unlike tanh it has a finite, exactly integrable shape,
    which lets the drive stage use antiderivative anti-aliasing below. */
inline float softClip (float x) noexcept
{
    if (x > 1.0f)
        return 2.0f / 3.0f;

    if (x < -1.0f)
        return -2.0f / 3.0f;

    return x - x * x * x * (1.0f / 3.0f);
}

/** Integral of softClip(). Even function, so |x| is enough. */
inline float softClipIntegral (float x) noexcept
{
    const float ax = std::abs (x);

    if (ax <= 1.0f)
    {
        const float x2 = x * x;
        return 0.5f * x2 - (x2 * x2) * (1.0f / 12.0f);
    }

    return (2.0f / 3.0f) * (ax - 1.0f) + 5.0f / 12.0f;
}

//==============================================================================
/** First-order antiderivative anti-aliasing around softClip(). The drive
    stages run at the host rate, so this keeps the harmonics they create from
    folding back as the glassy "digital" fizz you hear in naive saturators. */
struct SoftClipAdaa
{
    void reset() noexcept
    {
        previousInput = 0.0f;
        previousIntegral = 0.0f;
    }

    float process (float x) noexcept
    {
        const float integral = softClipIntegral (x);
        const float delta = x - previousInput;

        const float out = std::abs (delta) > 1.0e-5f
                            ? (integral - previousIntegral) / delta
                            : softClip (x);

        previousInput = x;
        previousIntegral = integral;
        return out;
    }

private:
    float previousInput = 0.0f;
    float previousIntegral = 0.0f;
};

//==============================================================================
/** Normalised cubic saturator with a first-order antiderivative correction.

    f(x) = x - (4/27)x^3 for |x| <= 1.5, flat at +/-1 beyond. The curve has a
    continuous first derivative, reaches exactly 1.0 at x = 1.5 with zero slope,
    and f'(0) = 1, so it is level-matched to the previous tanh stage at normal
    amplitudes while generating far less alias energy than a naive shaper. */
inline float warmShape (float x) noexcept
{
    x = clampf (x, -1.5f, 1.5f);
    return x - (4.0f / 27.0f) * x * x * x;
}

/** Antiderivative of warmShape(). Even, so |x| is enough. */
inline float warmShapeIntegral (float x) noexcept
{
    const float ax = std::abs (x);

    if (ax <= 1.5f)
    {
        const float x2 = x * x;
        return 0.5f * x2 - (1.0f / 27.0f) * x2 * x2;
    }

    return 0.9375f + (ax - 1.5f);
}

struct WarmSatAdaa
{
    void reset() noexcept
    {
        previousInput = 0.0f;
        previousIntegral = 0.0f;
    }

    float process (float x) noexcept
    {
        const float integral = warmShapeIntegral (x);
        const float delta = x - previousInput;
        const float out = std::abs (delta) > 1.0e-5f
                            ? (integral - previousIntegral) / delta
                            : warmShape (x);

        previousInput = x;
        previousIntegral = integral;
        return out;
    }

private:
    float previousInput = 0.0f;
    float previousIntegral = 0.0f;
};

//==============================================================================
/** 2x decimator for the voice's nonlinear path.

    The oscillators, filters and saturation now run at twice the host rate so
    the harmonics they generate stay below Nyquist instead of folding back as
    brittle digital fizz. This windowed half-band FIR is the only thing that
    brings the signal back to the host rate. It is deliberately short: 33 taps
    are flat to 16 kHz at 48 kHz, cost one multiply per tap, and add only eight
    host samples of latency. */
struct Decimator2x
{
    static constexpr int kTaps = 33;
    static constexpr int kCenter = kTaps / 2;

    /** Linear-phase group delay of the half-band FIR, converted from the 2x
        rate back to host samples. */
    static constexpr int latencyHostSamples() noexcept { return (kTaps - 1) / 4; }

    void reset() noexcept
    {
        history.fill (0.0f);
        index = 0;

        if (! prepared)
            prepare();
    }

    /** Build the FIR table outside the audio callback. */
    void prepare() noexcept
    {
        float sum = 0.0f;

        for (int i = 0; i < kTaps; ++i)
        {
            const float n = (float) (i - kCenter);
            const float sinc = std::abs (n) < 1.0e-6f
                                 ? 0.5f
                                 : 0.5f * std::sin (kPi * 0.5f * n) / (kPi * 0.5f * n);
            const float w = 0.42f
                          - 0.5f * std::cos (2.0f * kPi * (float) i / (float) (kTaps - 1))
                          + 0.08f * std::cos (4.0f * kPi * (float) i / (float) (kTaps - 1));

            h[(size_t) i] = sinc * w;
            sum += h[(size_t) i];
        }

        if (std::abs (sum) > 1.0e-6f)
            for (auto& c : h)
                c /= sum;

        prepared = true;
    }

    /** Feed the odd (interpolated-rate) sample. It is needed by the next filter
        output, but this sample itself is discarded when decimating. */
    void pushOdd (float sample) noexcept
    {
        index = (index + 1 == kTaps) ? 0 : index + 1;
        history[(size_t) index] = sample;
    }

    /** Feed the even sample and produce one host-rate output sample. */
    float pushEven (float sample) noexcept
    {
        index = (index + 1 == kTaps) ? 0 : index + 1;
        history[(size_t) index] = sample;

        float sum = 0.0f;
        int pos = index;

        for (int i = 0; i < kTaps; ++i)
        {
            sum += h[(size_t) i] * history[(size_t) pos];

            if (--pos < 0)
                pos = kTaps - 1;
        }

        return sum;
    }

private:
    std::array<float, kTaps> history {};
    std::array<float, kTaps> h {};
    int index = 0;
    bool prepared = false;
};

//==============================================================================
/** PolyBLEP saw/pulse oscillator. The correction band-limits the discontinuities,
    which matters a lot when eight voices of saw and pulse stack up.

    The pulse output is DC compensated, so changing the pulse width modulates
    timbre rather than pushing a moving offset into the filter. */
struct Oscillator
{
    double phase = 0.0;
    double inc = 0.0;

    void reset (double startPhase = 0.0) noexcept { phase = startPhase; }

    void setFrequency (double sampleRate, double hz) noexcept
    {
        const double nyquist = sampleRate * 0.5;
        hz = std::clamp (hz, 0.5, nyquist * 0.95);
        inc = hz / sampleRate;
    }

    static float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f)
            return 0.0f;

        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }

        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }

        return 0.0f;
    }

    float saw() noexcept
    {
        const auto t = (float) phase;
        const auto dt = (float) inc;
        float v = 2.0f * t - 1.0f;
        v -= polyBlep (t, dt);

        advance();
        return v;
    }

    float pulse (float width) noexcept
    {
        width = clampf (width, 0.02f, 0.98f);

        const auto t = (float) phase;
        const auto dt = (float) inc;
        float v = t < width ? 1.0f : -1.0f;
        v += polyBlep (t, dt);

        float t2 = t + 1.0f - width;
        if (t2 >= 1.0f)
            t2 -= 1.0f;

        v -= polyBlep (t2, dt);

        // Remove the width-dependent DC component.
        v -= 2.0f * width - 1.0f;

        advance();
        return v;
    }

    /** A square exactly one octave below the main oscillator. */
    float sub() noexcept
    {
        return pulse (0.5f);
    }

    /** Pure sine at the oscillator frequency. On the CS-80 this path is mixed
        into the VCA after the filter, so it restores the fundamental without
        undoing a resonant high-pass. */
    float sine() noexcept
    {
        const float v = std::sin (2.0f * kPi * (float) phase);
        advance();
        return v;
    }

private:
    void advance() noexcept
    {
        phase += inc;

        if (phase >= 1.0)
            phase -= 1.0;
    }
};

//==============================================================================
/** Topology-preserving state variable filter (12 dB/oct each output).

    The TPT form stays stable at extreme resonance. In addition, the resonance
    state can be softly saturated, which gives the filter the compression and
    harmonic bloom of the CS-80's discrete core at high settings. */
struct SVF
{
    float g = 0.0f, k = 1.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;

    void reset() noexcept
    {
        ic1 = ic2 = 0.0f;
    }

    void setCoefficients (double sampleRate, float hz, float q) noexcept
    {
        hz = clampf (hz, 20.0f, (float) sampleRate * 0.45f);
        q = std::max (q, 0.35f);

        g = std::tan (kPi * hz / (float) sampleRate);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float lowpass (float in, float saturation = 0.0f) noexcept
    {
        const float v3 = in - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;

        ic1 = flushDenormal (2.0f * v1 - ic1);
        ic2 = flushDenormal (2.0f * v2 - ic2);

        if (saturation > 0.0f)
            ic1 = softClip (ic1 * saturation) / saturation;

        if (! std::isfinite (ic1) || ! std::isfinite (ic2))
            reset();

        return v2;
    }

    float highpass (float in, float saturation = 0.0f) noexcept
    {
        const float v3 = in - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;

        ic1 = flushDenormal (2.0f * v1 - ic1);
        ic2 = flushDenormal (2.0f * v2 - ic2);

        if (saturation > 0.0f)
            ic1 = softClip (ic1 * saturation) / saturation;

        if (! std::isfinite (ic1) || ! std::isfinite (ic2))
            reset();

        return in - k * v1 - v2;
    }
};

//==============================================================================
/** Segment based envelope with the CS-80 style pair of levels: it can start at
    an arbitrary initial value (filter IL), climb to an attack level and settle
    on a sustain level. The amp envelope simply uses initial=0, attack=1.

    Time scaling exists so each voice can have its own slightly different
    envelope timing, the way eight analogue voice cards never match exactly. */
class Envelope
{
public:
    void prepare (double sampleRateToUse) noexcept
    {
        sampleRate = sampleRateToUse > 0.0 ? sampleRateToUse : 48000.0;
        reset();
        updateCoefficients();
    }

    void setTimes (float attackSeconds, float decaySeconds, float releaseSeconds) noexcept
    {
        attackSeconds = std::max (attackSeconds, 0.0005f);
        decaySeconds = std::max (decaySeconds, 0.0005f);
        releaseSeconds = std::max (releaseSeconds, 0.0005f);

        // The engine re-sends the whole patch on every block; skip the exp()
        // work unless the times actually moved.
        if (attackSeconds == attackTime && decaySeconds == decayTime && releaseSeconds == releaseTime)
            return;

        attackTime = attackSeconds;
        decayTime = decaySeconds;
        releaseTime = releaseSeconds;
        updateCoefficients();
    }

    void setTimeScale (float scale) noexcept
    {
        scale = clampf (scale, 0.5f, 2.0f);

        if (std::abs (scale - timeScale) < 1.0e-4f)
            return;

        timeScale = scale;
        updateCoefficients();
    }

    void setLevels (float attackLevel, float sustainLevel) noexcept
    {
        attackTarget = clampf (attackLevel, 0.0f, 1.0f);
        sustain = clampf (sustainLevel, 0.0f, 1.0f);

        if (stage == Stage::Attack)
            target = attackTarget;
        else if (stage == Stage::Decay)
            target = sustain;
        else if (stage == Stage::Sustain)
            value = sustain;
    }

    void trigger (float startValue) noexcept
    {
        value = clampf (startValue, 0.0f, 1.0f);
        stage = Stage::Attack;
        target = attackTarget;
        coefficient = attackCoefficient;
        stageLength = samplesFor (attackTime);
        elapsed = 0;
    }

    void release() noexcept
    {
        if (stage == Stage::Idle || stage == Stage::Release)
            return;

        stage = Stage::Release;
        target = 0.0f;
        coefficient = releaseCoefficient;
        stageLength = samplesFor (releaseTime);
        elapsed = 0;
    }

    void reset() noexcept
    {
        stage = Stage::Idle;
        value = 0.0f;
        target = 0.0f;
        elapsed = 0;
        stageLength = 0;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Sustain:
                value = sustain;
                return value;

            case Stage::Attack:
            case Stage::Decay:
            case Stage::Release:
            {
                value = target + (value - target) * coefficient;
                ++elapsed;

                if (elapsed >= stageLength)
                {
                    value = stage == Stage::Release ? 0.0f : target;

                    if (stage == Stage::Attack)
                    {
                        stage = Stage::Decay;
                        target = sustain;
                        coefficient = decayCoefficient;
                        stageLength = samplesFor (decayTime);
                        elapsed = 0;
                    }
                    else if (stage == Stage::Decay)
                    {
                        stage = Stage::Sustain;
                        value = sustain;
                    }
                    else
                    {
                        stage = Stage::Idle;
                        value = 0.0f;
                    }
                }

                return value;
            }
        }

        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }
    float getValue() const noexcept { return value; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    float coefficientFor (float seconds) const noexcept
    {
        const float scaled = seconds * timeScale;
        return clampf (std::exp (-5.0f / (scaled * (float) sampleRate)), 0.0f, 0.99999f);
    }

    uint64_t samplesFor (float seconds) const noexcept
    {
        return (uint64_t) std::max (1.0f, seconds * timeScale * (float) sampleRate);
    }

    void updateCoefficients() noexcept
    {
        attackCoefficient = coefficientFor (attackTime);
        decayCoefficient = coefficientFor (decayTime);
        releaseCoefficient = coefficientFor (releaseTime);
    }

    double sampleRate = 48000.0;
    Stage stage = Stage::Idle;

    float attackTime = 0.01f;
    float decayTime = 1.0f;
    float releaseTime = 0.4f;
    float attackTarget = 1.0f;
    float sustain = 0.8f;
    float timeScale = 1.0f;

    float attackCoefficient = 0.0f;
    float decayCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float coefficient = 0.0f;

    float value = 0.0f;
    float target = 0.0f;
    uint64_t elapsed = 0;
    uint64_t stageLength = 0;
};
} // namespace cs80
