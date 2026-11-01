#include "Engine.h"

#include "Dsp.h"
#include "Oscillator.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>

namespace cs80
{
namespace
{
/** Samples between filter coefficient updates. Small enough that a swept
    filter stays smooth, cheap enough to run eight two-channel voices without
    breaking a sweat. */
constexpr int kControlBlock = 8;
constexpr int kMixBlock = 512;
constexpr int kPitchBlocks = kMixBlock / kControlBlock + 1;

/** Length of the raised-cosine hand-over applied when a voice is stolen or
    retriggered. 64 host samples is 1.3 ms at 48 kHz: short enough to be
    inaudible as a delay, long enough to remove the edge completely. */
constexpr int kDeclickSamples = 64;

/** Fixed trim in front of the master fader. It leaves enough room for an
    eight-note chord to breathe before the output stage has to limit. */
constexpr float kOutputTrim = 0.68f;

inline float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db / 20.0f);
}

/** One-pole coefficient for a given time constant. Deriving it from the block
    length keeps every glide identical whether the host hands us 16 samples or
    512. */
inline float smoothingCoefficient (float timeSeconds, double sampleRate, int numSamples) noexcept
{
    const float blocks = (float) std::max (numSamples, 1);
    const float tau = std::max (timeSeconds, 1.0e-4f);
    return 1.0f - std::exp (-blocks / (tau * (float) sampleRate));
}

inline float lfoWaveform (float wave, float phase, const Wavetables& tables) noexcept
{
    if (wave < 0.5f)  // triangle
        return 1.0f - 4.0f * std::abs (phase - 0.5f);

    if (wave < 1.5f)  // sine
        return tables.readSine (phase);

    if (wave < 2.5f)  // square
        return phase < 0.5f ? 1.0f : -1.0f;

    return 2.0f * phase - 1.0f; // saw
}

/** A transparent peak limiter. Below 0.85 it is exactly linear; above that it
    bends smoothly toward 1.0. This keeps the last few dB of a dense chord
    musical instead of hard-clipping it. */
inline float softLimit (float x) noexcept
{
    const float magnitude = std::abs (x);

    if (magnitude <= 0.85f)
        return x;

    const float shaped = 0.85f + 0.149f * (1.0f - std::exp (-(magnitude - 0.85f) / 0.149f));
    return std::copysign (shaped, x);
}

/** Cheap deterministic hash used to give each voice its own analogue
    imperfections without touching the audio-thread RNG. */
inline uint32_t mixHash (uint32_t x) noexcept
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

inline float signedFromHash (uint32_t value) noexcept
{
    return (float) (value & 0xffffffu) / 8388608.0f - 1.0f;
}

struct ModRoute
{
    int source = 0;
    int destination = 0;
    float amount = 0.0f;
};

/** Value of one modulation source, already normalised. LFO and random are
    bipolar; envelopes, velocity and pressures are unipolar. */
inline float modulationSourceValue (int source, float lfo, float velocity,
                                   float channelPressure, float polyPressure,
                                   float modWheel, float keyTrack,
                                   float filterEnv, float ampEnv,
                                   float randomValue) noexcept
{
    switch (source)
    {
        case modsrc::lfo:             return lfo;
        case modsrc::velocity:        return velocity;
        case modsrc::channelPressure: return channelPressure;
        case modsrc::polyPressure:    return polyPressure;
        case modsrc::modWheel:        return modWheel;
        case modsrc::keyTrack:        return keyTrack;
        case modsrc::filterEnv:       return filterEnv;
        case modsrc::ampEnv:          return ampEnv;
        case modsrc::random:          return randomValue;
        default:                      return 0.0f;
    }
}

struct Modulation
{
    float lfoPhase = 0.0f;       // phase at the start of the block
    float lfoRate = 0.0f;        // cycles per second
    float lfoFade = 0.0f;
    float bendSemisFrom = 0.0f, bendSemisTo = 0.0f;
    float ribbonSemisFrom = 0.0f, ribbonSemisTo = 0.0f;
    float modWheel = 0.0f;
    float channelPressure = 0.0f;
};

//==============================================================================
/** Three-tap BBD ensemble. The CS-80's chorus is a huge part of its character,
    so it lives on the voice bus rather than as a bypassable afterthought.

    Each tap has its own rate, depth and stereo phase. The wet path is
    band-limited, the feedback is gently saturated, and the low end is kept
    mostly dry so chords never turn into mud. */
class Chorus
{
public:
    void prepare (double newSampleRate, const Wavetables& sharedTables)
    {
        tables = &sharedTables;
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        bufferSize = std::max (256, (int) (sampleRate * 0.09) + 8);
        bufferL.assign ((size_t) bufferSize, 0.0f);
        bufferR.assign ((size_t) bufferSize, 0.0f);

        feedbackHighpassCoeff = 1.0f - std::exp (-2.0f * kPi * 95.0f / (float) sampleRate);
        toneCoeff = 1.0f - std::exp (-2.0f * kPi * 8200.0f / (float) sampleRate);
        lowShelfCoeff = 1.0f - std::exp (-2.0f * kPi * 220.0f / (float) sampleRate);
        amountCoeff = 1.0f - std::exp (-1.0f / (0.020f * (float) sampleRate));

        reset();
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        write = 0;
        phases = { 0.0f, 0.31f, 0.63f, 0.86f, 0.47f };
        slowPhase = 0.0f;
        feedbackL = feedbackR = 0.0f;
        feedbackHighpassL = feedbackHighpassR = 0.0f;
        toneL = toneR = 0.0f;
        lowL = lowR = 0.0f;
        smoothedAmount = 0.0f;
        tailPeak = 0.0f;
        cold = true;
    }

    void process (float* left, float* right, int numSamples, float amount, int mode)
    {
        const float target = clampf (amount, 0.0f, 1.0f);

        // When the ensemble is fully faded and its tail has genuinely decayed,
        // the delay lines have been cleared, so skipping is safe and there is
        // no stale audio to burst out when it is switched back on.
        if (cold && target <= 1.0e-4f)
            return;

        if (tables == nullptr || bufferSize <= 0)
            return;

        cold = false;

        // The CS-80 has two chorus positions plus tremolo. Chorus I is the
        // gentle, slow, three-tap setting; Chorus II is the wide five-tap halo
        // this instrument is known for. The tap layout is chosen once per
        // block, so neither setting costs anything the other does not.
        const bool chorusOne = mode == 0;
        const int tapCount = chorusOne ? 3 : 5;
        const float baseDelay[5] = { chorusOne ? 0.0061f : 0.0073f,
                                     chorusOne ? 0.0104f : 0.0119f,
                                     chorusOne ? 0.0161f : 0.0173f,
                                     0.0247f, 0.0331f };
        const float depth[5] = { chorusOne ? 0.0025f : 0.0019f,
                                 chorusOne ? 0.0036f : 0.0027f,
                                 chorusOne ? 0.0046f : 0.0034f,
                                 0.0042f, 0.0049f };
        const float rate[5] = { chorusOne ? 0.105f : 0.19f,
                                chorusOne ? 0.079f : 0.23f,
                                chorusOne ? 0.058f : 0.13f,
                                0.097f, 0.071f };
        const float tapGain[5] = { chorusOne ? 0.52f : 0.46f,
                                   chorusOne ? 0.34f : 0.30f,
                                   chorusOne ? 0.20f : 0.22f,
                                   0.16f, 0.11f };

        const float slowIncrement = 0.083f / (float) sampleRate;
        float blockPeak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            smoothedAmount += (target - smoothedAmount) * amountCoeff;

            const float inL = left[i];
            const float inR = right[i];

            bufferL[(size_t) write] = inL + feedbackL;
            bufferR[(size_t) write] = inR + feedbackR;

            // A very slow wobble of the whole delay line gives the ensemble
            // the gentle pitch drift a real string machine has. All modulation
            // reads come from the shared sine table: no libm on the audio thread.
            const float drift = 1.0f + 0.055f * tables->readSine (slowPhase);

            float wetL = 0.0f, wetR = 0.0f;

            for (int tap = 0; tap < tapCount; ++tap)
            {
                float phaseR = phases[(size_t) tap] + 0.43f;
                if (phaseR >= 1.0f)
                    phaseR -= 1.0f;

                const float modL = tables->readSine (phases[(size_t) tap]);
                const float modR = tables->readSine (phaseR);

                const float delayL = (baseDelay[tap] * drift + depth[tap] * modL) * (float) sampleRate;
                const float delayR = (baseDelay[tap] * drift + depth[tap] * modR) * (float) sampleRate;

                wetL += tapGain[tap] * readInterpolated (bufferL, delayL);
                wetR += tapGain[tap] * readInterpolated (bufferR, delayR);

                phases[(size_t) tap] += rate[tap] / (float) sampleRate;
                if (phases[(size_t) tap] >= 1.0f)
                    phases[(size_t) tap] -= 1.0f;
            }

            slowPhase += slowIncrement;
            if (slowPhase >= 1.0f)
                slowPhase -= 1.0f;

            // Keep the sub-bass out of the modulated path; the dry signal
            // carries the weight, the wet path supplies the shimmer.
            lowL += (inL - lowL) * lowShelfCoeff;
            lowR += (inR - lowR) * lowShelfCoeff;
            wetL -= lowL * 0.55f;
            wetR -= lowR * 0.55f;

            feedbackHighpassL += (wetL - feedbackHighpassL) * feedbackHighpassCoeff;
            feedbackHighpassR += (wetR - feedbackHighpassR) * feedbackHighpassCoeff;

            feedbackL = flushDenormal (fastTanh (wetL * 0.55f) * 0.22f
                                       - feedbackHighpassL * 0.06f);
            feedbackR = flushDenormal (fastTanh (wetR * 0.55f) * 0.22f
                                       - feedbackHighpassR * 0.06f);

            toneL = flushDenormal (toneL + (wetL - toneL) * toneCoeff);
            toneR = flushDenormal (toneR + (wetR - toneR) * toneCoeff);

            const float dryGain = 1.0f - 0.28f * smoothedAmount;
            const float wetGain = (chorusOne ? 0.60f : 0.67f) * smoothedAmount;

            left[i] = inL * dryGain + toneL * wetGain;
            right[i] = inR * dryGain + toneR * wetGain;

            blockPeak = std::max (blockPeak, std::max (std::abs (toneL), std::abs (toneR)));

            if (++write >= bufferSize)
                write = 0;
        }

        tailPeak = blockPeak;

        if (target <= 1.0e-4f && smoothedAmount < 1.0e-4f && tailPeak < 1.0e-6f)
        {
            std::fill (bufferL.begin(), bufferL.end(), 0.0f);
            std::fill (bufferR.begin(), bufferR.end(), 0.0f);
            feedbackL = feedbackR = 0.0f;
            feedbackHighpassL = feedbackHighpassR = 0.0f;
            toneL = toneR = 0.0f;
            lowL = lowR = 0.0f;
            smoothedAmount = 0.0f;
            cold = true;
        }
    }

private:
    float readInterpolated (const std::vector<float>& buffer, float delaySamples) const
    {
        float readPos = (float) write - delaySamples;
        while (readPos < 0.0f)
            readPos += (float) bufferSize;
        while (readPos >= (float) bufferSize)
            readPos -= (float) bufferSize;

        const int i1 = (int) readPos;
        const float frac = readPos - (float) i1;
        const int i0 = (i1 - 1 + bufferSize) % bufferSize;
        const int i2 = (i1 + 1) % bufferSize;
        const int i3 = (i1 + 2) % bufferSize;

        // Catmull-Rom rather than linear: the delay line is moving, and a
        // cubic read removes the gritty modulation noise a linear tap leaves
        // on sustained chords.
        const float y0 = buffer[(size_t) i0], y1 = buffer[(size_t) i1];
        const float y2 = buffer[(size_t) i2], y3 = buffer[(size_t) i3];
        const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c = -0.5f * y0 + 0.5f * y2;
        return ((a * frac + b) * frac + c) * frac + y1;
    }

    double sampleRate = 48000.0;
    int bufferSize = 0;
    int write = 0;
    std::vector<float> bufferL, bufferR;
    std::array<float, 5> phases { 0.0f, 0.31f, 0.63f, 0.86f, 0.47f };
    float slowPhase = 0.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
    float feedbackHighpassL = 0.0f, feedbackHighpassR = 0.0f;
    float toneL = 0.0f, toneR = 0.0f;
    float lowL = 0.0f, lowR = 0.0f;
    float feedbackHighpassCoeff = 1.0f, toneCoeff = 1.0f, lowShelfCoeff = 1.0f;
    float amountCoeff = 0.001f;
    float smoothedAmount = 0.0f;
    float tailPeak = 0.0f;
    bool cold = true;
    const Wavetables* tables = nullptr;
};

//==============================================================================
/** A larger, smoother plate than a stock Freeverb: six damped combs and three
    diffusing allpasses per side, a genuine stereo pre-delay and a high-passed
    wet path. It stays out of the way of the synth but adds the expensive-
    sounding air the CS-80 loves. */
class PlateReverb
{
public:
    void prepare (double newSampleRate, const Wavetables& sharedTables)
    {
        tables = &sharedTables;
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        const double scale = sampleRate / 44100.0;

        const int combDelays[6] = { 1116, 1188, 1277, 1356, 1422, 1491 };
        const int allpassDelays[4] = { 556, 441, 341, 213 };

        for (int i = 0; i < 6; ++i)
        {
            combL[(size_t) i].resize ((int) (combDelays[i] * scale) + 4);
            combR[(size_t) i].resize ((int) ((combDelays[i] + 23) * scale) + 4);
            combL[(size_t) i].modDepth = (0.7f + 0.23f * (float) i) * (float) scale;
            combR[(size_t) i].modDepth = (0.9f + 0.19f * (float) i) * (float) scale;
            combL[(size_t) i].modPhase = 0.11f * (float) i;
            combR[(size_t) i].modPhase = 0.19f + 0.13f * (float) i;
        }

        for (int i = 0; i < 4; ++i)
        {
            allpassL[(size_t) i].resize ((int) (allpassDelays[i] * scale) + 4);
            allpassR[(size_t) i].resize ((int) ((allpassDelays[i] + 17) * scale) + 4);
        }

        preDelayL.assign ((int) (sampleRate * 0.031) + 4, 0.0f);
        preDelayR.assign ((int) (sampleRate * 0.038) + 4, 0.0f);
        preDelaySamplesL = std::max (1, (int) (sampleRate * 0.021));
        preDelaySamplesR = std::max (1, (int) (sampleRate * 0.029));
        mixCoeff = 1.0f - std::exp (-1.0f / (0.020f * (float) sampleRate));

        reset();
    }

    void reset()
    {
        for (auto& c : combL) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.index = 0; c.store = 0.0f; }
        for (auto& c : combR) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.index = 0; c.store = 0.0f; }
        for (auto& a : allpassL) { std::fill (a.buffer.begin(), a.buffer.end(), 0.0f); a.index = 0; }
        for (auto& a : allpassR) { std::fill (a.buffer.begin(), a.buffer.end(), 0.0f); a.index = 0; }
        std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
        std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
        preWriteL = preWriteR = 0;
        modPhase1 = modPhase2 = 0.0f;
        wetHighpassL = wetHighpassR = 0.0f;
        wetLowpassL = wetLowpassR = 0.0f;
        smoothedMix = 0.0f;
        tailPeak = 0.0f;
        cold = true;
    }

    void process (float* left, float* right, int numSamples, float mix)
    {
        const float target = clampf (mix, 0.0f, 1.0f);

        // A frozen tank with stale audio is worse than no reverb: when the mix
        // has fully faded and the tail is genuinely gone, the buffers have been
        // cleared, so skipping cannot produce a burst on re-enable.
        if (cold && target <= 1.0e-4f)
            return;

        if (tables == nullptr)
            return;

        cold = false;

        constexpr float feedback = 0.862f;
        constexpr float damping = 0.23f;
        const float hpCoeff = 1.0f - std::exp (-2.0f * kPi * 170.0f / (float) sampleRate);
        const float lpCoeff = 1.0f - std::exp (-2.0f * kPi * 7600.0f / (float) sampleRate);
        const float modInc1 = 0.107f / (float) sampleRate;
        const float modInc2 = 0.163f / (float) sampleRate;
        float blockPeak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            smoothedMix += (target - smoothedMix) * mixCoeff;
            int readL = preWriteL - preDelaySamplesL;
            int readR = preWriteR - preDelaySamplesR;

            while (readL < 0)
                readL += (int) preDelayL.size();

            while (readR < 0)
                readR += (int) preDelayR.size();

            const float inL = preDelayL[(size_t) readL];
            const float inR = preDelayR[(size_t) readR];

            const float feed = (left[i] + right[i]) * 0.32f;
            preDelayL[(size_t) preWriteL] = feed;
            preDelayR[(size_t) preWriteR] = feed;

            if (++preWriteL >= (int) preDelayL.size())
                preWriteL = 0;
            if (++preWriteR >= (int) preDelayR.size())
                preWriteR = 0;

            float wetL = 0.0f, wetR = 0.0f;

            // Slowly detune the tank. Static comb delays give a metallic ring;
            // a fraction of a millisecond of movement is what makes a plate
            // sound like air rather than a bank of filters.
            const float modA = tables->readSine (modPhase1);
            const float modB = tables->readSine (modPhase2);

            for (int i = 0; i < 6; ++i)
                wetL += processComb (combL[(size_t) i], inL, feedback, damping,
                                     combL[(size_t) i].modDepth * ((i & 1) ? modB : modA));

            for (int i = 0; i < 6; ++i)
                wetR += processComb (combR[(size_t) i], inR, feedback, damping,
                                     combR[(size_t) i].modDepth * ((i & 1) ? modA : modB));

            modPhase1 += modInc1;
            modPhase2 += modInc2;

            if (modPhase1 >= 1.0f) modPhase1 -= 1.0f;
            if (modPhase2 >= 1.0f) modPhase2 -= 1.0f;

            for (auto& ap : allpassL)
                wetL = processAllpass (ap, wetL);

            for (auto& ap : allpassR)
                wetR = processAllpass (ap, wetR);

            wetHighpassL += (wetL - wetHighpassL) * hpCoeff;
            wetHighpassR += (wetR - wetHighpassR) * hpCoeff;
            wetL -= wetHighpassL;
            wetR -= wetHighpassR;

            wetLowpassL += (wetL - wetLowpassL) * lpCoeff;
            wetLowpassR += (wetR - wetLowpassR) * lpCoeff;

            const float dryGain = 1.0f - 0.26f * smoothedMix;
            const float wetGain = 0.40f * smoothedMix;

            left[i] = left[i] * dryGain + wetLowpassL * wetGain;
            right[i] = right[i] * dryGain + wetLowpassR * wetGain;

            blockPeak = std::max (blockPeak, std::max (std::abs (wetLowpassL),
                                                       std::abs (wetLowpassR)));
        }

        tailPeak = blockPeak;

        if (target <= 1.0e-4f && smoothedMix < 1.0e-4f && tailPeak < 1.0e-6f)
        {
            for (auto& c : combL) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.store = 0.0f; }
            for (auto& c : combR) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.store = 0.0f; }
            for (auto& a : allpassL) std::fill (a.buffer.begin(), a.buffer.end(), 0.0f);
            for (auto& a : allpassR) std::fill (a.buffer.begin(), a.buffer.end(), 0.0f);
            std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
            std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
            wetHighpassL = wetHighpassR = 0.0f;
            wetLowpassL = wetLowpassR = 0.0f;
            smoothedMix = 0.0f;
            cold = true;
        }
    }

private:
    struct Comb
    {
        std::vector<float> buffer;
        int index = 0;
        float store = 0.0f;
        float modDepth = 0.0f;
        float modPhase = 0.0f;

        void resize (int size) { buffer.assign ((size_t) std::max (size, 8), 0.0f); }
    };

    struct Allpass
    {
        std::vector<float> buffer;
        int index = 0;

        void resize (int size) { buffer.assign ((size_t) std::max (size, 8), 0.0f); }
    };

    static float processComb (Comb& comb, float in, float feedback, float damping,
                              float modOffset) noexcept
    {
        const int size = (int) comb.buffer.size();
        float readPos = (float) comb.index - modOffset;

        while (readPos < 0.0f)
            readPos += (float) size;

        while (readPos >= (float) size)
            readPos -= (float) size;

        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        const float out = lerpf (comb.buffer[(size_t) i0], comb.buffer[(size_t) i1], frac);

        comb.store = out * (1.0f - damping) + comb.store * damping;
        comb.buffer[(size_t) comb.index] = in + comb.store * feedback;

        if (++comb.index >= size)
            comb.index = 0;

        return out;
    }

    static float processAllpass (Allpass& ap, float in) noexcept
    {
        const float buffered = ap.buffer[(size_t) ap.index];
        ap.buffer[(size_t) ap.index] = in + buffered * 0.5f;

        if (++ap.index >= (int) ap.buffer.size())
            ap.index = 0;

        return buffered - in;
    }

    double sampleRate = 48000.0;
    std::array<Comb, 6> combL, combR;
    std::array<Allpass, 4> allpassL, allpassR;
    std::vector<float> preDelayL, preDelayR;
    int preDelaySamplesL = 0, preDelaySamplesR = 0;
    int preWriteL = 0, preWriteR = 0;
    float modPhase1 = 0.0f, modPhase2 = 0.0f;
    float wetHighpassL = 0.0f, wetHighpassR = 0.0f;
    float wetLowpassL = 0.0f, wetLowpassR = 0.0f;
    float mixCoeff = 0.001f;
    float smoothedMix = 0.0f;
    float tailPeak = 0.0f;
    bool cold = true;
    const Wavetables* tables = nullptr;
};

//==============================================================================
/** One complete CS-80 channel card: two VCOs, a sub oscillator, a dedicated
    ring carrier, a high-pass into a low-pass and its own pair of envelopes.
    A voice owns two of these in parallel. */
struct Layer
{
    void prepare (double newSampleRate, const Wavetables& tables)
    {
        sampleRate = newSampleRate;
        const double voiceRate = sampleRate * 2.0;
        oscillatorTables = &tables;
        osc1.prepare (tables, voiceRate);
        osc2.prepare (tables, voiceRate);
        subOsc.prepare (tables, voiceRate);
        sineOsc.prepare (tables, voiceRate);
        osc1.reset (0.0);
        osc2.reset (0.33);
        subOsc.reset (0.17);
        sineOsc.reset (0.09);
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
        vcaAdaa.reset();
        decimator.prepare();
        decimator.reset();
        cutoffSmooth = 1400.0f;
        lastOutput = 0.0f;
        declickCorrection = 0.0f;
        declickRemaining = kDeclickSamples;
        declickPending = false;
        ringPhase = 0.0;
        noiseState = 0x12345678u;
        modulationPitch = modulationPwm = modulationCutoff = 0.0f;
        modulationResonance = modulationAmp = modulationDrive = modulationLevel = 0.0f;

        for (int i = 0; i < kDeclickSamples; ++i)
            declickCurve[(size_t) i] = 0.5f + 0.5f * std::cos (kPi * (float) i
                                                              / (float) (kDeclickSamples - 1));
    }

    void kill()
    {
        ampEnv.reset();
        filterEnv.reset();
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
        vcaAdaa.reset();
        decimator.reset();
        lastOutput = 0.0f;
        declickCorrection = 0.0f;
        declickRemaining = kDeclickSamples;
        declickPending = false;
        modulationPitch = modulationPwm = modulationCutoff = 0.0f;
        modulationResonance = modulationAmp = modulationDrive = modulationLevel = 0.0f;
    }

    float nextNoise() noexcept
    {
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return (float) ((int32_t) noiseState) / 2147483648.0f;
    }

    float nextRandom01() noexcept
    {
        return (nextNoise() + 1.0f) * 0.5f;
    }

    double sampleRate = 48000.0;
    const Wavetables* oscillatorTables = nullptr;
    BandlimitedOscillator osc1, osc2, subOsc, sineOsc;
    Envelope ampEnv, filterEnv;
    SVF hpf, lpf;
    SoftClipAdaa driveAdaa;
    WarmSatAdaa vcaAdaa;
    Decimator2x decimator;
    std::array<float, kDeclickSamples> declickCurve {};
    float cutoffSmooth = 1400.0f;
    float lastOutput = 0.0f;
    float declickCorrection = 0.0f;
    int declickRemaining = kDeclickSamples;
    bool declickPending = false;
    double ringPhase = 0.0;
    uint32_t noiseState = 0x12345678u;

    // Smoothed modulation-matrix offsets. Smoothing here (not on the raw
    // route amounts) means an automated amount moves like a slider rather than
    // like a sequence of steps, and it costs one multiply per destination.
    float modulationPitch = 0.0f;      // semitones
    float modulationPwm = 0.0f;        // pulse-width fraction
    float modulationCutoff = 0.0f;     // octaves
    float modulationResonance = 0.0f;  // 0..1 offset
    float modulationAmp = 0.0f;        // gain offset
    float modulationDrive = 0.0f;      // 0..1 offset
    float modulationLevel = 0.0f;      // layer gain offset
};

//==============================================================================
struct Voice
{
    void prepare (double newSampleRate, const Wavetables& tables)
    {
        sampleRate = newSampleRate;

        for (auto& layer : layers)
            layer.prepare (sampleRate, tables);

        active = false;
        released = false;
        held = false;
        note = 60;
        velocity = 0.8f;
        polyPressure = -1.0f;
        age = 0;
        noiseState = 0x12345678u;
        varianceState = 0xfeedfaceu;
        cutoffSmooth = 1400.0f;
        driftPhase1 = driftPhase2 = driftOffset = 0.0f;
        layerDriftPhase = 0.0f;
        layerCutoffDriftPhase = 0.0f;
        twangSemis = twangCurrent = 0.0f;
        glidePitch = 60.0f;
        glideTarget = 60.0f;
        lastNote = 60.0f;
        panACurrent = panBCurrent = 0.0f;
        resetVariance (0.0f);
    }

    void resetVariance (float amount)
    {
        const uint32_t seed = mixHash (varianceState ^ 0x9e3779b9u);
        vintagePitchCents = signedFromHash (mixHash (seed + 1u)) * 5.0f * amount;
        vintageCutoffOct = signedFromHash (mixHash (seed + 2u)) * 0.14f * amount;
        vintageLevel = 1.0f + signedFromHash (mixHash (seed + 3u)) * 0.05f * amount;
        vintageEnvScale = 1.0f + signedFromHash (mixHash (seed + 4u)) * 0.055f * amount;
        driftOffset = signedFromHash (mixHash (seed + 5u)) * 2.4f * amount;
    }

    void start (int midiNote, float newVelocity, uint64_t newAge, const Params& p,
                float glideFromNote)
    {
        // A stolen voice is retriggered from wherever it is: keeping its
        // oscillator phase and filter state is what stops the changeover
        // from clicking.
        const bool steal = active;

        active = true;
        released = false;
        held = false;
        note = midiNote;
        glideTarget = (float) midiNote;
        velocity = clampf (newVelocity, 0.0f, 1.0f);
        age = newAge;
        polyPressure = -1.0f;

        noiseState = 0x9e3779b9u
                   ^ ((uint32_t) midiNote * 2654435761u)
                   ^ ((uint32_t) newAge * 2246822519u);

        varianceState = mixHash (noiseState ^ (uint32_t) (newAge * 31u));
        resetVariance (p.vintage);

        for (int i = 0; i < 2; ++i)
        {
            auto& layer = layers[(size_t) i];

            const float ampStart = steal ? layer.ampEnv.getValue() : 0.0f;
            const float filterStart = steal ? layer.filterEnv.getValue() : p.fIl;

            if (! steal)
            {
                // Free-running phases. Eight voices that all start at phase
                // zero stack into one giant comb filter; scattered phases
                // sound like an ensemble of oscillators instead.
                layer.osc1.reset (nextRandom01());
                layer.osc2.reset (nextRandom01());
                layer.subOsc.reset (nextRandom01());
                layer.sineOsc.reset (nextRandom01());
                layer.hpf.reset();
                layer.lpf.reset();
                layer.driveAdaa.reset();
                layer.vcaAdaa.reset();
                layer.decimator.reset();
                layer.cutoffSmooth = p.lpfCutoff;
                layer.ringPhase = (double) nextRandom01();
                layer.noiseState = mixHash (noiseState + (uint32_t) (i * 977u)) | 1u;
            }

            layer.ampEnv.trigger (ampStart);
            layer.filterEnv.trigger (filterStart);

            layer.declickPending = steal;
            layer.declickRemaining = 0;
        }

        if (p.glide > 0.0005f)
        {
            // A stolen voice keeps its current pitch and glides from there.
            if (! steal)
                glidePitch = glideFromNote >= 0.0f ? glideFromNote : (float) midiNote;
        }
        else
        {
            glidePitch = (float) midiNote;
        }

        if (! steal)
        {
            driftPhase1 = nextRandom01();
            driftPhase2 = nextRandom01();
            layerDriftPhase = nextRandom01();
            layerCutoffDriftPhase = nextRandom01();
        }

        // Initial touch bend: the note starts a little flat and springs up.
        // Velocity makes it stronger, exactly like the CS-80's key-contact
        // pitch bend, and a lighter touch gives the softer, rounder version.
        twangSemis = p.twang * (1.15f - 0.35f * velocity);
        twangCurrent = twangSemis;

        lastNote = (float) midiNote;

        // A per-note random value gives the Random source a stable identity
        // for the life of the note instead of a per-block lottery.
        modRandom = signedFromHash (mixHash (noiseState ^ 0x51ed270bu));
    }

    float nextNoise() noexcept
    {
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return (float) ((int32_t) noiseState) / 2147483648.0f;
    }

    float nextRandom01() noexcept
    {
        return (nextNoise() + 1.0f) * 0.5f;
    }

    void releaseNow()
    {
        if (! active || released)
            return;

        for (auto& layer : layers)
        {
            layer.ampEnv.release();
            layer.filterEnv.release();
        }

        released = true;
        held = false;
    }

    void kill()
    {
        active = false;
        released = false;
        held = false;

        for (auto& layer : layers)
            layer.kill();
    }

    float activityLevel() const
    {
        return std::max (layers[0].ampEnv.getValue(), layers[1].ampEnv.getValue()) * velocity;
    }

    /** Render both channel cards into a stereo pair. The caller decides which
        cards are audible (single, dual or keyboard-split), and how each card
        is panned. */
    void render (float* left, float* right, int numSamples, const Params& p,
                 const Modulation& mod, bool useLayerA, bool useLayerB,
                 float panA, float panB, const ModRoute* routes, int routeCount,
                 const Tuning& tuning) noexcept
    {
        if (! active)
            return;

        // Scala keyboard mappings explicitly allow unassigned ('x') keys.
        // They must be silent rather than being clamped into an audible
        // sub-Hz oscillator by the frequency setter.
        if (tuning.noteFrequency (note) <= 0.0)
        {
            kill();
            return;
        }

        const float chunkSeconds = (float) numSamples / (float) sampleRate;

        // Slow, deterministic per-voice drift. Nothing in an analogue polysynth
        // ever sits perfectly still, and that tiny motion is most of what makes
        // stacked voices sound alive rather than sampled.
        driftPhase1 += 0.071f * chunkSeconds;
        driftPhase2 += 0.113f * chunkSeconds;
        layerDriftPhase += 0.049f * chunkSeconds;
        layerCutoffDriftPhase += 0.023f * chunkSeconds;

        if (driftPhase1 >= 1.0f)
            driftPhase1 -= 1.0f;

        if (driftPhase2 >= 1.0f)
            driftPhase2 -= 1.0f;

        if (layerDriftPhase >= 1.0f)
            layerDriftPhase -= 1.0f;

        if (layerCutoffDriftPhase >= 1.0f)
            layerCutoffDriftPhase -= 1.0f;

        const float vintage = clampf (p.vintage, 0.0f, 1.0f);
        const float driftCents = vintage * (driftOffset
                                          + 2.3f * std::sin (2.0f * kPi * driftPhase1)
                                          + 1.4f * std::sin (2.0f * kPi * driftPhase2));

        // The two cards are trimmed independently. Giving channel II its own
        // very slow drift keeps a layered patch moving instead of beating at
        // one fixed rate.
        const float layerDriftCents = vintage * (1.9f * std::sin (2.0f * kPi * layerDriftPhase)
                                                 + 0.7f * std::sin (2.0f * kPi * (layerDriftPhase + 0.31f)));
        const float layerCutoffDrift = vintage * 0.085f
                                     * std::sin (2.0f * kPi * layerCutoffDriftPhase);

        for (int i = 0; i < 2; ++i)
        {
            auto& layer = layers[(size_t) i];
            const float scale = i == 0 ? vintageEnvScale
                                       : clampf (vintageEnvScale * p.layerEnv, 0.5f, 2.0f);

            layer.ampEnv.setTimeScale (scale);
            layer.filterEnv.setTimeScale (scale);
        }

        const float pressure = polyPressure >= 0.0f ? polyPressure : mod.channelPressure;
        const float aftertouch = pressure * p.aftertouch;
        const float velocityOctaves = (velocity - 0.5f) * p.velocity * 2.0f;
        const float velocityGain = 1.0f - p.velocity + p.velocity * velocity;

        // Aftertouch is not a subtle afterthought on a CS-80. It opens the
        // filter, lifts the level and adds pressure vibrato all at once.
        const float ampGain = velocityGain * (1.0f + 0.22f * aftertouch);

        // Pre-compute one glide position per control block so both channel
        // cards follow exactly the same pitch curve.
        float glidePitchBlocks[kPitchBlocks] {};
        float twangBlocks[kPitchBlocks] {};
        {
            const int blocks = (numSamples + kControlBlock - 1) / kControlBlock;
            const float glideSeconds = p.glide > 0.0005f ? p.glide : 0.0f;
            const bool glideActive = glideSeconds > 0.0005f;
            const float glideTau = std::max (glideSeconds / 4.5f, 1.0e-4f);
            const float twangTau = 0.042f;
            float local = glidePitch;
            float localTwang = twangCurrent;

            for (int b = 0; b < blocks; ++b)
            {
                const int n = std::min (kControlBlock, numSamples - b * kControlBlock);
                glidePitchBlocks[b] = local;
                twangBlocks[b] = localTwang;

                if (glideActive)
                {
                    const float glideCoeff = 1.0f - std::exp (-(float) n
                                                              / (glideTau * (float) sampleRate));
                    local += (glideTarget - local) * glideCoeff;
                }
                else
                {
                    local = glideTarget;
                }

                const float twangCoeff = 1.0f - std::exp (-(float) n
                                                          / (twangTau * (float) sampleRate));
                localTwang += (0.0f - localTwang) * twangCoeff;
            }

            glidePitch = local;
            twangCurrent = localTwang;
        }

        float bufferA[kMixBlock];
        float bufferB[kMixBlock];

        if (useLayerA)
            renderLayer (0, bufferA, numSamples, p, mod, driftCents - layerDriftCents * 0.5f,
                         aftertouch, velocityOctaves, ampGain, glidePitchBlocks,
                         twangBlocks, layerCutoffDrift, routes, routeCount, tuning);

        if (useLayerB)
            renderLayer (1, bufferB, numSamples, p, mod, driftCents + layerDriftCents * 0.5f,
                         aftertouch, velocityOctaves, ampGain, glidePitchBlocks,
                         twangBlocks, layerCutoffDrift, routes, routeCount, tuning);

        // Pan is a voice-level destination, so it is evaluated once per
        // chunk. The existing one-pole pan glide turns the chunk-rate updates
        // into a smooth movement without per-sample routing cost.
        float panModulation = 0.0f;

        if (routeCount > 0)
        {
            const float lfo = lfoWaveform (p.lfoWave, mod.lfoPhase,
                                           *layers[0].oscillatorTables);
            const float keyTrackSource = clampf ((glidePitch - 60.0f) / 36.0f, -1.0f, 1.0f);
            const float poly = polyPressure >= 0.0f ? polyPressure : 0.0f;

            for (int r = 0; r < routeCount; ++r)
            {
                const auto& route = routes[r];

                if (route.destination != moddest::pan)
                    continue;

                const float value = modulationSourceValue (route.source, lfo, velocity,
                                                           mod.channelPressure, poly,
                                                           mod.modWheel, keyTrackSource,
                                                           layers[0].filterEnv.getValue(),
                                                           layers[0].ampEnv.getValue(),
                                                           modRandom);
                panModulation += route.amount * value;
            }

            panModulation = clampf (panModulation, -1.0f, 1.0f);
        }

        // Changing the voice count changes each voice's place in the stereo
        // field. Sliding to the new position instead of jumping keeps chords
        // smooth as notes are added and released.
        panACurrent += (clampf (panA + panModulation, -1.0f, 1.0f) - panACurrent) * 0.25f;
        panBCurrent += (clampf (panB + panModulation, -1.0f, 1.0f) - panBCurrent) * 0.25f;

        const float angleA = (panACurrent + 1.0f) * kPi * 0.25f;
        const float angleB = (panBCurrent + 1.0f) * kPi * 0.25f;
        const float panAL = std::cos (angleA), panAR = std::sin (angleA);
        const float panBL = std::cos (angleB), panBR = std::sin (angleB);

        for (int i = 0; i < numSamples; ++i)
        {
            if (useLayerA)
            {
                left[i] += bufferA[i] * panAL;
                right[i] += bufferA[i] * panAR;
            }

            if (useLayerB)
            {
                left[i] += bufferB[i] * panBL;
                right[i] += bufferB[i] * panBR;
            }
        }

        const bool aAlive = useLayerA && layers[0].ampEnv.isActive();
        const bool bAlive = useLayerB && layers[1].ampEnv.isActive();

        if (! aAlive && ! bAlive)
            active = false;
    }

    void renderLayer (int index, float* destination, int numSamples, const Params& p,
                      const Modulation& mod, float driftCents, float aftertouch,
                      float velocityOctaves, float ampGain,
                      const float* glidePitchBlocks, const float* twangBlocks,
                      float layerCutoffDrift, const ModRoute* routes, int routeCount,
                      const Tuning& tuning) noexcept
    {
        auto& layer = layers[(size_t) index];
        const bool second = index == 1;

        const float baseLpfRes = second ? p.layerRes : p.lpfRes;
        const float hpfQ = 0.7f + p.hpfRes * 7.0f;
        const float layerGain = second ? clampf (p.layerLevel, 0.0f, 1.0f) : 1.0f;
        const float layerAmp = ampGain * layerGain;
        const float pitchOffset = second ? p.layerOctave * 12.0f + p.layerDetune * 0.01f : 0.0f;
        const float pulseWidthBase = second ? p.layerPw : p.vco2Pw;
        const float cutoffOffset = second ? p.layerCutoff : 0.0f;
        // Two samples per host sample. Everything downstream of the oscillator
        // mix runs at this rate, so the filter's saturation and the VCA curve
        // generate harmonics above the host Nyquist instead of folding them
        // back into the audible band.
        const double voiceRate = sampleRate * 2.0;
        const float voiceRateF = (float) voiceRate;
        const double ringIncrement = (double) p.ringSpeed / voiceRate;

        const float cutoffCoeff = smoothingCoefficient (0.0018f, sampleRate, kControlBlock);

        float lfoPhase = mod.lfoPhase;

        for (int start = 0; start < numSamples; start += kControlBlock)
        {
            const int n = std::min (kControlBlock, numSamples - start);
            const float glidePitchLocal = glidePitchBlocks[start / kControlBlock];
            const float twangSemis = twangBlocks[start / kControlBlock];

            const float lfo = lfoWaveform (p.lfoWave, lfoPhase, *layer.oscillatorTables);
            lfoPhase += mod.lfoRate * (float) n / (float) sampleRate;

            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            const float modAmount = lfo * mod.lfoFade;
            const float keytrack = (glidePitchLocal - 60.0f) / 12.0f * p.filterTrack;

            // ---- Modulation matrix -------------------------------------------------
            float routePitch = 0.0f, routePwm = 0.0f, routeCutoff = 0.0f;
            float routeResonance = 0.0f, routeAmp = 0.0f, routeDrive = 0.0f;
            float routeLevel = 0.0f;

            if (routeCount > 0)
            {
                const float keyTrackSource = clampf ((glidePitchLocal - 60.0f) / 36.0f,
                                                     -1.0f, 1.0f);
                const float poly = polyPressure >= 0.0f ? polyPressure : 0.0f;

                for (int r = 0; r < routeCount; ++r)
                {
                    const auto& route = routes[r];
                    const float value = modulationSourceValue (route.source, lfo, velocity,
                                                               mod.channelPressure, poly,
                                                               mod.modWheel, keyTrackSource,
                                                               layer.filterEnv.getValue(),
                                                               layer.ampEnv.getValue(),
                                                               modRandom);
                    const float scaled = route.amount * value;

                    switch (route.destination)
                    {
                        case moddest::pitch:        routePitch += scaled * 24.0f; break;
                        case moddest::pulseWidth:   routePwm += scaled * 0.45f; break;
                        case moddest::cutoff:       routeCutoff += scaled * 4.0f; break;
                        case moddest::resonance:    routeResonance += scaled * 0.9f; break;
                        case moddest::amplitude:    routeAmp += scaled * 0.8f; break;
                        case moddest::drive:        routeDrive += scaled * 0.8f; break;
                        case moddest::layerLevel:   routeLevel += scaled * 0.8f; break;
                        default: break;
                    }
                }
            }

            const float modCoeff = 1.0f - std::exp (-(float) n / (0.004f * (float) sampleRate));
            layer.modulationPitch += (routePitch - layer.modulationPitch) * modCoeff;
            layer.modulationPwm += (routePwm - layer.modulationPwm) * modCoeff;
            layer.modulationCutoff += (routeCutoff - layer.modulationCutoff) * modCoeff;
            layer.modulationResonance += (routeResonance - layer.modulationResonance) * modCoeff;
            layer.modulationAmp += (routeAmp - layer.modulationAmp) * modCoeff;
            layer.modulationDrive += (routeDrive - layer.modulationDrive) * modCoeff;
            layer.modulationLevel += (routeLevel - layer.modulationLevel) * modCoeff;

            const float lpfRes = clampf (baseLpfRes + layer.modulationResonance, 0.0f, 1.0f);
            const float lpfQ = 0.7f + lpfRes * 12.6f;
            const float driveAmount = clampf (p.drive + layer.modulationDrive, 0.0f, 1.0f);
            const float drive = 1.0f + driveAmount * 2.3f + lpfRes * 0.85f;
            const float driveComp = 1.0f / (0.62f * drive + 0.38f);
            const float filterSat = 0.7f + driveAmount * 2.4f + lpfRes * 2.6f;
            const float blockLayerAmp = layerAmp
                                      * clampf (1.0f + layer.modulationAmp, 0.0f, 2.0f)
                                      * clampf (1.0f + layer.modulationLevel, 0.2f, 2.0f);

            const float hpCutoff = clampf (p.hpfCutoff * std::exp2 ((glidePitchLocal - 60.0f) / 12.0f * 0.15f),
                                           20.0f, 6000.0f);
            layer.hpf.setCoefficients (voiceRateF, hpCutoff, hpfQ);

            // Bend and ribbon glides are interpolated across the chunk, so the
            // pitch curve does not depend on the host's buffer size.
            const float progress = (float) start / (float) numSamples;
            const float baseSemis = lerpf (mod.bendSemisFrom, mod.bendSemisTo, progress)
                                  + lerpf (mod.ribbonSemisFrom, mod.ribbonSemisTo, progress)
                                  + (driftCents + vintagePitchCents) * 0.01f
                                  + p.tune * 0.01f;

            const float vibratoCents = modAmount
                                     * (p.lfoPitch * (18.0f + 42.0f * mod.modWheel) + aftertouch * 30.0f);

            const float pitchSemis = baseSemis + vibratoCents * 0.01f + layer.modulationPitch;

            // The tuning table, not a hard-coded 440 Hz formula, turns a note
            // number into a frequency. In 12-TET the two are identical.
            const double f1 = tuning.frequencyForNote (glidePitchLocal
                                                       + pitchSemis - twangSemis
                                                       + p.vco1Octave * 12.0
                                                       + p.vco1Detune * 0.01
                                                       + pitchOffset);

            const double f2 = tuning.frequencyForNote (glidePitchLocal
                                                       + pitchSemis - twangSemis
                                                       + p.vco2Octave * 12.0
                                                       + p.vco2Detune * 0.01
                                                       + pitchOffset);

            layer.osc1.setFrequency (voiceRate, f1);
            layer.osc2.setFrequency (voiceRate, f2);
            layer.subOsc.setFrequency (voiceRate, f1 * 0.5);
            layer.sineOsc.setFrequency (voiceRate, f1);

            const float pulseWidth = clampf (pulseWidthBase
                                             + modAmount * p.lfoPwm * 0.38f
                                             + aftertouch * 0.11f
                                             + layer.modulationPwm,
                                             0.03f, 0.97f);

            const float cutoffOctaves = keytrack
                                      + (layer.filterEnv.getValue() - 0.35f) * 4.8f * p.filterEnvAmt
                                      + modAmount * p.lfoFilter * 2.2f
                                      + aftertouch * 2.2f
                                      + velocityOctaves
                                      + vintageCutoffOct
                                      + layerCutoffDrift
                                      + layer.modulationCutoff
                                      + cutoffOffset;

            const float targetCutoff = clampf (p.lpfCutoff * std::exp2 (cutoffOctaves),
                                               25.0f, voiceRateF * 0.45f);

            layer.cutoffSmooth += (targetCutoff - layer.cutoffSmooth) * cutoffCoeff;
            layer.cutoffSmooth = clampf (flushDenormal (layer.cutoffSmooth), 20.0f,
                                         voiceRateF * 0.45f);
            layer.lpf.setCoefficients (voiceRateF, layer.cutoffSmooth, lpfQ);

            const float tremolo = 1.0f - p.lfoAmp * mod.lfoFade * (0.5f - 0.5f * lfo);

        // Only spend table reads on signals that actually reach the mix. The
        // phases keep running so no oscillator is frozen, but a patch with the
        // sub and sine bypassed costs nothing for them.
        const bool needSaw = p.vco1Level > 1.0e-4f || p.ring > 1.0e-4f;
        const bool needPulse = p.vco2Level > 1.0e-4f || p.ring > 1.0e-4f;
        const bool needSub = p.vco1Sub > 1.0e-4f;
        const bool needSine = p.sineLevel > 1.0e-4f;
        const bool needNoise = p.noise > 1.0e-4f;
        const bool needRing = p.ring > 1.0e-4f;

            for (int j = 0; j < n; ++j)
            {
                const float amp = layer.ampEnv.process();
                layer.filterEnv.process();

                float decimated = 0.0f;

                for (int os = 0; os < 2; ++os)
                {
                    float saw = 0.0f, pulse = 0.0f, sub = 0.0f, sine = 0.0f, noise = 0.0f;

                    if (needSaw)
                        saw = layer.osc1.saw();
                    else
                        layer.osc1.advanceOnly();

                    if (needPulse)
                        pulse = layer.osc2.pulse (pulseWidth);
                    else
                        layer.osc2.advanceOnly();

                    if (needSub)
                        sub = layer.subOsc.sub();
                    else
                        layer.subOsc.advanceOnly();

                    if (needSine)
                        sine = layer.sineOsc.sine();
                    else
                        layer.sineOsc.advanceOnly();

                    if (needNoise)
                        noise = layer.nextNoise();

                    float carrier = 0.0f;

                    if (needRing)
                    {
                        carrier = layer.oscillatorTables->readSine ((float) layer.ringPhase);
                        layer.ringPhase += ringIncrement;

                        while (layer.ringPhase >= 1.0)
                            layer.ringPhase -= 1.0;
                    }

                    const float ringSignal = needRing ? saw * (0.72f * carrier + 0.28f * pulse) : 0.0f;

                    float raw = 0.50f * (saw * p.vco1Level
                                         + pulse * p.vco2Level
                                         + sub * p.vco1Sub * 0.85f)
                              + 0.30f * ringSignal * p.ring
                              + 0.09f * noise * p.noise;

                    raw = clampf (raw, -8.0f, 8.0f);

                    const float highpassed = layer.hpf.highpass (raw, 0.8f + p.drive * 1.4f);

                    // Anti-aliased saturation is the heart of the voice's
                    // warmth. At 2x the harmonics it creates stay above the
                    // host Nyquist until the decimator removes them.
                    const float driven = layer.driveAdaa.process (highpassed * drive) * driveComp;
                    const float filtered = layer.lpf.lowpass (driven, filterSat)
                                         * (1.0f - 0.10f * lpfRes);

                    // The VCA's sine bypass is a defining CS-80 detail: the
                    // filter can thin and vowel the tone while the sine keeps
                    // the note solid underneath it.
                    const float mixed = filtered + sine * p.sineLevel * 0.85f;
                    const float warmed = layer.vcaAdaa.process (mixed * 1.16f) * (1.0f / 1.16f);

                    const float value = warmed * amp * blockLayerAmp * tremolo * vintageLevel;

                    if (os == 0)
                        layer.decimator.pushOdd (value);
                    else
                        decimated = layer.decimator.pushEven (value);
                }

                float value = decimated;

                // On a steal, match the previous voice's last sample and
                // settle to the new waveform over a fraction of a millisecond.
                if (layer.declickPending)
                {
                    layer.declickCorrection = layer.lastOutput - value;
                    layer.declickRemaining = 0;
                    layer.declickPending = false;
                }

                if (layer.declickRemaining < kDeclickSamples)
                {
                    // Raised cosine: gain 1 on the first sample (so the new
                    // waveform continues exactly from the old one) and exactly
                    // 0 on the last, with zero slope at both ends.
                    value += layer.declickCorrection
                           * layer.declickCurve[(size_t) layer.declickRemaining];
                    ++layer.declickRemaining;
                }

                layer.lastOutput = value;
                destination[start + j] = value;
            }
        }
    }

    double sampleRate = 48000.0;
    std::array<Layer, 2> layers;
    bool active = false;
    bool released = false;
    bool held = false;
    int note = 60;
    float velocity = 0.8f;
    float polyPressure = -1.0f;
    uint64_t age = 0;
    uint32_t noiseState = 0x12345678u;
    uint32_t varianceState = 0xfeedfaceu;
    float cutoffSmooth = 1400.0f;
    float driftPhase1 = 0.0f, driftPhase2 = 0.0f, driftOffset = 0.0f;
    float layerDriftPhase = 0.0f, layerCutoffDriftPhase = 0.0f;
    float twangSemis = 0.0f, twangCurrent = 0.0f;
    float glidePitch = 60.0f, glideTarget = 60.0f, lastNote = 60.0f;
    float vintagePitchCents = 0.0f;
    float vintageCutoffOct = 0.0f;
    float vintageLevel = 1.0f;
    float vintageEnvScale = 1.0f;
    float panACurrent = 0.0f, panBCurrent = 0.0f;
    float modRandom = 0.0f;
};
} // namespace

//==============================================================================
struct Engine::Impl
{
    double sampleRate = 48000.0;
    bool prepared = false;
    Params params;
    std::array<Voice, kNumVoices> voices;
    Wavetables wavetables;
    Chorus chorus;
    PlateReverb reverb;
    WarmSatAdaa outputSatL, outputSatR;

    std::array<ModRoute, pid::kModSlots> routes {};
    int routeCount = 0;

    // Three immutable-on-read tuning slots. The writer avoids both the
    // published slot and the one currently owned by the audio callback. A
    // reader announces its slot and rechecks the published index before it
    // dereferences anything, closing the race with rapid successive updates.
    std::array<Tuning, 3> tunings;
    std::atomic<int> activeTuning { 0 };
    std::atomic<int> readingTuning { -1 };
    mutable std::mutex tuningWriterMutex; // never acquired by the audio thread
    Tuning publishedTuning;

    std::array<std::atomic<float>, kNumVoices> activity {};
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };
    std::array<std::atomic<float>, kScopeSize> scopeL {};
    std::array<std::atomic<float>, kScopeSize> scopeR {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<int> activeVoices { 0 };

    uint64_t ageCounter = 1;
    float bendTarget = 0.0f, bendSmoothed = 0.0f;
    float ribbonTarget = 0.0f, ribbonSmoothed = 0.0f;
    float modWheel = 0.0f;
    float channelPressure = 0.0f;
    bool sustainDown = false;

    float lfoPhase = 0.0f;
    float lfoElapsed = 0.0f;
    float lastNotePlayed = -1.0f;
    float gainSmoothed = kOutputTrim * 0.6f;
    float limiterGain = 1.0f;
    bool hasRendered = false;
    float brillianceLpL = 0.0f, brillianceLpR = 0.0f;
    float analogLowL = 0.0f, analogLowR = 0.0f;
    float tremoloPhase = 0.0f;
    float dcX1L = 0.0f, dcY1L = 0.0f, dcX1R = 0.0f, dcY1R = 0.0f;

    std::array<float, kMixBlock> mixL {}, mixR {};

    Impl()
    {
        params = defaultParams();

        for (auto& a : activity)
            a.store (0.0f);

        for (auto& s : scopeL)
            s.store (0.0f);

        for (auto& s : scopeR)
            s.store (0.0f);
    }

    void reset()
    {
        for (auto& v : voices)
            v.prepare (sampleRate, wavetables);

        chorus.prepare (sampleRate, wavetables);
        reverb.prepare (sampleRate, wavetables);
        outputSatL.reset();
        outputSatR.reset();

        bendSmoothed = bendTarget;
        ribbonSmoothed = ribbonTarget;
        lfoPhase = 0.0f;
        lfoElapsed = 0.0f;
        tremoloPhase = 0.0f;
        gainSmoothed = dbToGain (params.masterDb) * kOutputTrim;
        limiterGain = 1.0f;
        hasRendered = false;
        brillianceLpL = brillianceLpR = 0.0f;
        analogLowL = analogLowR = 0.0f;
        dcX1L = dcY1L = dcX1R = dcY1R = 0.0f;
        sustainDown = false;

        for (auto& a : activity)
            a.store (0.0f);

        meterL.store (0.0f);
        meterR.store (0.0f);
        activeVoices.store (0);
    }

    Voice* voiceForNote (int midiNote)
    {
        for (auto& v : voices)
            if (v.active && ! v.released && v.note == midiNote)
                return &v;

        return nullptr;
    }

    Voice* allocateVoice()
    {
        for (auto& v : voices)
            if (! v.active)
                return &v;

        Voice* oldestReleased = nullptr;
        Voice* oldest = nullptr;

        for (auto& v : voices)
        {
            if (v.released && (oldestReleased == nullptr || v.age < oldestReleased->age))
                oldestReleased = &v;

            if (oldest == nullptr || v.age < oldest->age)
                oldest = &v;
        }

        return oldestReleased != nullptr ? oldestReleased : oldest;
    }

    void updateVoiceParameters()
    {
        for (auto& v : voices)
        {
            for (int i = 0; i < 2; ++i)
            {
                auto& layer = v.layers[(size_t) i];

                layer.ampEnv.setTimes (params.aA, params.aD, params.aR);
                layer.ampEnv.setLevels (1.0f, params.aS);

                layer.filterEnv.setTimes (params.fA, params.fD, params.fR);
                layer.filterEnv.setLevels (params.fAl, params.fS);
            }
        }
    }

    void layerActivity (const Voice& voice, bool useLayerA, bool useLayerB,
                        bool& aAlive, bool& bAlive) const
    {
        aAlive = useLayerA && voice.layers[0].ampEnv.isActive();
        bAlive = useLayerB && voice.layers[1].ampEnv.isActive();
    }

    void publishTuning (const Tuning& newTuning)
    {
        const std::lock_guard<std::mutex> lock (tuningWriterMutex);
        const int current = activeTuning.load();
        const int reading = readingTuning.load();

        for (int slot = 0; slot < (int) tunings.size(); ++slot)
        {
            if (slot == current || slot == reading)
                continue;

            tunings[(size_t) slot] = newTuning;
            publishedTuning = newTuning;
            activeTuning.store (slot);
            return;
        }
    }

    Tuning tuningSnapshot() const
    {
        const std::lock_guard<std::mutex> lock (tuningWriterMutex);
        return publishedTuning;
    }

    void processChunk (float* left, float* right, int numSamples)
    {
        // Compile the eight routing slots once per chunk. A slot that is off,
        // unrouted or at zero depth is left out of the per-control-block loop
        // entirely, so an unused matrix costs one comparison per slot.
        routeCount = 0;

        for (int slot = 0; slot < pid::kModSlots; ++slot)
        {
            const int source = (int) std::round (params.modSource[slot]);
            const int destination = (int) std::round (params.modDest[slot]);
            const float amount = params.modAmount[slot];

            if (source > modsrc::off && source < modsrc::count
                && destination > moddest::off && destination < moddest::count
                && std::abs (amount) > 1.0e-4f)
            {
                routes[(size_t) routeCount++] = { source, destination, amount };
            }
        }

        int tuningSlot = 0;
        do
        {
            tuningSlot = activeTuning.load();
            readingTuning.store (tuningSlot);
        }
        while (activeTuning.load() != tuningSlot);

        const Tuning& currentTuningTable = tunings[(size_t) tuningSlot];

        std::fill_n (mixL.begin(), numSamples, 0.0f);
        std::fill_n (mixR.begin(), numSamples, 0.0f);

        updateVoiceParameters();

        const int count = (int) std::count_if (voices.begin(), voices.end(),
                                               [] (const Voice& v) { return v.active; });

        const float bendPrevious = bendSmoothed;
        const float ribbonPrevious = ribbonSmoothed;

        bendSmoothed += (bendTarget - bendSmoothed)
                      * smoothingCoefficient (0.014f, sampleRate, numSamples);
        ribbonSmoothed += (ribbonTarget - ribbonSmoothed)
                        * smoothingCoefficient (0.007f, sampleRate, numSamples);

        if (count == 0)
            lfoElapsed = 0.0f;
        else
            lfoElapsed += (float) numSamples / (float) sampleRate;

        const float fade = params.lfoDelay <= 0.0f
                             ? (count > 0 ? 1.0f : 0.0f)
                             : clampf ((lfoElapsed - params.lfoDelay) / 0.35f, 0.0f, 1.0f);

        Modulation mod;
        mod.lfoPhase = lfoPhase;
        mod.lfoRate = params.lfoRate;
        mod.lfoFade = fade;
        mod.bendSemisFrom = bendPrevious * params.bendRange;
        mod.bendSemisTo = bendSmoothed * params.bendRange;
        mod.ribbonSemisFrom = ribbonPrevious * 12.0f;
        mod.ribbonSemisTo = ribbonSmoothed * 12.0f;
        mod.modWheel = modWheel;
        mod.channelPressure = channelPressure;

        const float mode = params.layerMode;
        const bool dual = mode > 0.5f && mode < 1.5f;
        const bool split = mode >= 1.5f;
        int voiceIndex = 0;
        int activeIndex = 0;

        for (auto& v : voices)
        {
            if (! v.active)
            {
                activity[(size_t) voiceIndex].store (0.0f);
                ++voiceIndex;
                continue;
            }

            const bool useA = ! split || v.note < (int) std::round (params.splitPoint);
            const bool useB = (dual || (split && ! useA)) && params.layerLevel > 0.0005f;

            // Spread the sounding voices evenly across the field in allocation
            // order. A single note stays centred, a chord opens up, and the two
            // channel cards cross slightly for a wide, three-dimensional image.
            const float spreadPosition = count > 1
                                           ? ((float) activeIndex / (float) (count - 1)) * 2.0f - 1.0f
                                           : 0.0f;
            const float voicePan = spreadPosition * params.spread * 0.82f;

            const float panA = voicePan;
            const float panB = clampf (params.layerPan * 0.86f - voicePan * 0.64f, -1.0f, 1.0f);

            v.render (mixL.data(), mixR.data(), numSamples, params, mod,
                      useA, useB, panA, panB, routes.data(), routeCount,
                      currentTuningTable);

            bool aAlive = false, bAlive = false;
            layerActivity (v, useA, useB, aAlive, bAlive);

            const float level = v.active && (aAlive || bAlive) ? v.activityLevel() : 0.0f;
            auto& act = activity[(size_t) voiceIndex];
            act.store (std::max (level, act.load() * 0.55f));

            ++voiceIndex;
            ++activeIndex;
        }

        lfoPhase += params.lfoRate * (float) numSamples / (float) sampleRate;
        while (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;

        const int ensembleMode = (int) std::lround (params.ensembleMode);
        const bool tremoloMode = ensembleMode == 2;

        // In tremolo mode the chorus fades to fully dry (the existing per-
        // sample smoothing removes any switch click) and a slow stereo
        // amplitude sweep takes over at the output stage.
        chorus.process (mixL.data(), mixR.data(), numSamples,
                        tremoloMode ? 0.0f : params.ensemble, ensembleMode);
        reverb.process (mixL.data(), mixR.data(), numSamples, params.space);

        // A little fixed headroom trim keeps eight voices + ensemble from
        // sitting on the limiter, while the master fader keeps its dB scale.
        const float targetGain = dbToGain (params.masterDb) * kOutputTrim;
        const float gainFrom = gainSmoothed;
        gainSmoothed += (targetGain - gainSmoothed)
                      * smoothingCoefficient (0.035f, sampleRate, numSamples);
        const float gainStep = (gainSmoothed - gainFrom) / (float) numSamples;
        float gain = gainFrom;

        const float tiltCoeff = 1.0f - std::exp (-2.0f * kPi * 2200.0f / (float) sampleRate);
        const float analogLowCoeff = 1.0f - std::exp (-2.0f * kPi * 155.0f / (float) sampleRate);
        const float limiterAttack = 1.0f - std::exp (-1.0f / (0.0006f * (float) sampleRate));
        const float limiterRelease = 1.0f - std::exp (-1.0f / (0.140f * (float) sampleRate));

        float peakL = 0.0f, peakR = 0.0f;
        int writePos = scopeWrite.load (std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float l = mixL[(size_t) i];
            float r = mixR[(size_t) i];

            if (tremoloMode)
            {
                // Stereo tremolo, 90 degrees apart, at the CS-80's fixed
                // slow rate. The depth follows the Ensemble control.
                const float depth = clampf (params.ensemble, 0.0f, 1.0f) * 0.62f;
                float phaseR = tremoloPhase + 0.25f;
                if (phaseR >= 1.0f)
                    phaseR -= 1.0f;

                l *= 1.0f - depth * (0.5f + 0.5f * wavetables.readSine (tremoloPhase));
                r *= 1.0f - depth * (0.5f + 0.5f * wavetables.readSine (phaseR));

                tremoloPhase += 4.6f / (float) sampleRate;
                if (tremoloPhase >= 1.0f)
                    tremoloPhase -= 1.0f;
            }

            brillianceLpL += (l - brillianceLpL) * tiltCoeff;
            brillianceLpR += (r - brillianceLpR) * tiltCoeff;
            l += params.brilliance * (l - brillianceLpL) * 0.8f;
            r += params.brilliance * (r - brillianceLpR) * 0.8f;

            l *= gain;
            r *= gain;
            gain += gainStep;

            // A fixed output transformer: a little low-end weight and a touch
            // of asymmetric second-harmonic colour. It is deliberately below
            // the threshold where it would sound like distortion; it simply
            // makes the instrument feel like it is running through iron.
            analogLowL += (l - analogLowL) * analogLowCoeff;
            analogLowR += (r - analogLowR) * analogLowCoeff;
            l += analogLowL * 0.09f;
            r += analogLowR * 0.09f;
            l = outputSatL.process (l * 1.045f + 0.014f * l * l) * (1.0f / 1.045f);
            r = outputSatR.process (r * 1.045f + 0.014f * r * r) * (1.0f / 1.045f);

            const float dcL = l - dcX1L + 0.9985f * dcY1L;
            dcX1L = l;
            dcY1L = flushDenormal (dcL);
            l = dcL;

            const float dcR = r - dcX1R + 0.9985f * dcY1R;
            dcX1R = r;
            dcY1R = flushDenormal (dcR);
            r = dcR;

            // Stereo-linked peak limiter: catch the rare overshoot without
            // squashing the whole instrument through a saturator.
            const float magnitude = std::max (std::abs (l), std::abs (r));
            const float targetLimiter = magnitude > 0.92f ? 0.92f / magnitude : 1.0f;
            limiterGain += (targetLimiter - limiterGain)
                         * (targetLimiter < limiterGain ? limiterAttack : limiterRelease);
            l *= limiterGain;
            r *= limiterGain;

            l = softLimit (l);
            r = softLimit (r);
            l = clampf (l, -1.0f, 1.0f);
            r = clampf (r, -1.0f, 1.0f);

            left[i] = l;
            right[i] = r;

            peakL = std::max (peakL, std::abs (l));
            peakR = std::max (peakR, std::abs (r));

            scopeL[(size_t) writePos].store (l, std::memory_order_relaxed);
            scopeR[(size_t) writePos].store (r, std::memory_order_relaxed);
            if (++writePos >= kScopeSize)
                writePos = 0;
        }

        scopeWrite.store (writePos, std::memory_order_relaxed);
        meterL.store (std::max (peakL, meterL.load (std::memory_order_relaxed) * 0.72f),
                      std::memory_order_relaxed);
        meterR.store (std::max (peakR, meterR.load (std::memory_order_relaxed) * 0.72f),
                      std::memory_order_relaxed);

        activeVoices.store ((int) std::count_if (voices.begin(), voices.end(),
                                                 [] (const Voice& v) { return v.active; }),
                            std::memory_order_relaxed);

        hasRendered = true;
        readingTuning.store (-1);
    }
};

//==============================================================================
Engine::Engine() : impl (std::make_unique<Impl>()) {}
Engine::~Engine() = default;
Engine::Engine (Engine&&) noexcept = default;
Engine& Engine::operator= (Engine&&) noexcept = default;

void Engine::prepare (double sampleRate)
{
    if (sampleRate <= 0.0)
        sampleRate = 48000.0;

    impl->sampleRate = sampleRate;
    impl->prepared = true;

    // Table construction is deliberately outside the audio callback: the
    // wavetables and the decimator coefficients are built here, once.
    impl->wavetables.prepare (sampleRate);
    impl->reset();
}

void Engine::reset()
{
    impl->reset();
}

double Engine::sampleRate() const { return impl->sampleRate; }
bool Engine::isPrepared() const { return impl->prepared; }

int Engine::latencySamples() noexcept { return Decimator2x::latencyHostSamples(); }
double Engine::oversamplingFactor() noexcept { return 2.0; }

int Engine::voiceNote (int index) const
{
    if (index < 0 || index >= kNumVoices)
        return -1;

    const auto& voice = impl->voices[(size_t) index];
    return voice.active ? voice.note : -1;
}

bool Engine::voiceIsActive (int index) const
{
    if (index < 0 || index >= kNumVoices)
        return false;

    return impl->voices[(size_t) index].active;
}

void Engine::setTuning (const Tuning& newTuning)
{
    impl->publishTuning (newTuning);
}

Tuning Engine::tuning() const
{
    return impl->tuningSnapshot();
}

void Engine::setParams (const Params& newParams)
{
    impl->params = newParams;

    // Before the first block (typical when a preset is loaded right after
    // prepareToPlay) snap the gain instead of gliding from the previous patch.
    if (! impl->hasRendered)
        impl->gainSmoothed = dbToGain (newParams.masterDb) * kOutputTrim;
}

Params Engine::params() const
{
    return impl->params;
}

void Engine::noteOn (int midiNote, float velocity)
{
    if (! impl->prepared)
        return;

    if (auto* existing = impl->voiceForNote (midiNote))
        existing->releaseNow();

    if (auto* voice = impl->allocateVoice())
    {
        const float glideFrom = impl->params.glide > 0.0005f
                                  ? impl->lastNotePlayed
                                  : -1.0f;

        voice->start (midiNote, velocity, impl->ageCounter++, impl->params, glideFrom);
        impl->lastNotePlayed = (float) midiNote;
    }
}

void Engine::noteOff (int midiNote)
{
    for (auto& v : impl->voices)
    {
        if (v.active && ! v.released && v.note == midiNote)
        {
            if (impl->sustainDown)
                v.held = true;
            else
                v.releaseNow();
        }
    }
}

void Engine::allNotesOff (bool immediately)
{
    for (auto& v : impl->voices)
    {
        if (! v.active)
            continue;

        if (immediately)
            v.kill();
        else
            v.releaseNow();
    }

    if (immediately)
        impl->activeVoices.store (0);
}

void Engine::setSustainPedal (bool isDown)
{
    impl->sustainDown = isDown;

    if (! isDown)
    {
        for (auto& v : impl->voices)
            if (v.held)
                v.releaseNow();
    }
}

void Engine::pitchBend (float normalised)
{
    impl->bendTarget = clampf (normalised, -1.0f, 1.0f);
}

void Engine::setRibbon (float normalised)
{
    impl->ribbonTarget = clampf (normalised, -1.0f, 1.0f);
}

void Engine::setModWheel (float normalised)
{
    impl->modWheel = clampf (normalised, 0.0f, 1.0f);
}

void Engine::setChannelPressure (float normalised)
{
    impl->channelPressure = clampf (normalised, 0.0f, 1.0f);
}

void Engine::setPolyPressure (int midiNote, float normalised)
{
    for (auto& v : impl->voices)
        if (v.active && v.note == midiNote)
            v.polyPressure = clampf (normalised, 0.0f, 1.0f);
}

void Engine::process (float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    if (! impl->prepared || left == nullptr || right == nullptr)
    {
        if (left != nullptr)
            std::fill_n (left, numSamples, 0.0f);
        if (right != nullptr)
            std::fill_n (right, numSamples, 0.0f);

        return;
    }

    int remaining = numSamples;

    while (remaining > 0)
    {
        const int block = std::min (remaining, kMixBlock);
        impl->processChunk (left, right, block);
        left += block;
        right += block;
        remaining -= block;
    }
}

int Engine::activeVoiceCount() const
{
    return impl->activeVoices.load (std::memory_order_relaxed);
}

float Engine::voiceActivity (int index) const
{
    if (index < 0 || index >= kNumVoices)
        return 0.0f;

    return impl->activity[(size_t) index].load (std::memory_order_relaxed);
}

float Engine::outputMeter (int channel) const
{
    return channel == 1
             ? impl->meterR.load (std::memory_order_relaxed)
             : impl->meterL.load (std::memory_order_relaxed);
}

int Engine::readScope (float* destination, int maxSamples) const
{
    const int count = std::clamp (maxSamples, 0, kScopeSize);
    const int end = impl->scopeWrite.load (std::memory_order_relaxed);

    for (int i = 0; i < count; ++i)
    {
        int index = end - count + i;
        while (index < 0)
            index += kScopeSize;

        destination[i] = impl->scopeL[(size_t) index].load (std::memory_order_relaxed);
    }

    return count;
}

int Engine::readScopeStereo (float* destinationL, float* destinationR, int maxSamples) const
{
    const int count = std::clamp (maxSamples, 0, kScopeSize);
    const int end = impl->scopeWrite.load (std::memory_order_relaxed);

    for (int i = 0; i < count; ++i)
    {
        int index = end - count + i;
        while (index < 0)
            index += kScopeSize;

        if (destinationL != nullptr)
            destinationL[i] = impl->scopeL[(size_t) index].load (std::memory_order_relaxed);

        if (destinationR != nullptr)
            destinationR[i] = impl->scopeR[(size_t) index].load (std::memory_order_relaxed);
    }

    return count;
}
} // namespace cs80
