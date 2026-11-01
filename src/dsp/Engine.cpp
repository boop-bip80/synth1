#include "Engine.h"

#include "Dsp.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
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

inline float lfoWaveform (float wave, float phase) noexcept
{
    if (wave < 0.5f)  // triangle
        return 1.0f - 4.0f * std::abs (phase - 0.5f);

    if (wave < 1.5f)  // sine
        return std::sin (2.0f * kPi * phase);

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
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        bufferSize = std::max (256, (int) (sampleRate * 0.06) + 8);
        bufferL.assign ((size_t) bufferSize, 0.0f);
        bufferR.assign ((size_t) bufferSize, 0.0f);

        feedbackHighpassCoeff = 1.0f - std::exp (-2.0f * kPi * 140.0f / (float) sampleRate);
        toneCoeff = 1.0f - std::exp (-2.0f * kPi * 8200.0f / (float) sampleRate);
        lowShelfCoeff = 1.0f - std::exp (-2.0f * kPi * 220.0f / (float) sampleRate);

        reset();
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        write = 0;
        phases = { 0.0f, 0.31f, 0.63f, 0.86f };
        slowPhase = 0.0f;
        feedbackL = feedbackR = 0.0f;
        feedbackHighpassL = feedbackHighpassR = 0.0f;
        toneL = toneR = 0.0f;
        lowL = lowR = 0.0f;
    }

    void process (float* left, float* right, int numSamples, float amount)
    {
        if (amount <= 0.0005f || bufferSize <= 0)
            return;

        // Four BBD taps, each with its own rate, depth and stereo phase. The
        // wider spacing and the extra tap turn the ensemble from a gentle
        // shimmer into the big, softly drifting CS-80 halo.
        const float baseDelay[4] = { 0.0081f, 0.0127f, 0.0181f, 0.0263f };
        const float depth[4] = { 0.0021f, 0.0029f, 0.0036f, 0.0044f };
        const float rate[4] = { 0.17f, 0.23f, 0.13f, 0.097f };
        const float tapGain[4] = { 0.52f, 0.32f, 0.22f, 0.16f };

        const float dryGain = 1.0f - 0.24f * amount;
        const float wetGain = 0.61f * amount;
        const float slowIncrement = 0.083f / (float) sampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = right[i];

            bufferL[(size_t) write] = inL + feedbackL;
            bufferR[(size_t) write] = inR + feedbackR;

            // A very slow wobble of the whole delay line gives the ensemble
            // the gentle pitch drift a real string machine has.
            const float drift = 1.0f + 0.055f * std::sin (2.0f * kPi * slowPhase);

            float wetL = 0.0f, wetR = 0.0f;

            for (int tap = 0; tap < 4; ++tap)
            {
                const float modL = std::sin (2.0f * kPi * phases[(size_t) tap]);
                const float modR = std::sin (2.0f * kPi * (phases[(size_t) tap] + 0.43f));

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

            feedbackL = flushDenormal (fastTanh (wetL * 0.55f) * 0.24f
                                       - feedbackHighpassL * 0.05f);
            feedbackR = flushDenormal (fastTanh (wetR * 0.55f) * 0.24f
                                       - feedbackHighpassR * 0.05f);

            toneL = flushDenormal (toneL + (wetL - toneL) * toneCoeff);
            toneR = flushDenormal (toneR + (wetR - toneR) * toneCoeff);

            left[i] = inL * dryGain + toneL * wetGain;
            right[i] = inR * dryGain + toneR * wetGain;

            if (++write >= bufferSize)
                write = 0;
        }
    }

private:
    float readInterpolated (const std::vector<float>& buffer, float delaySamples) const
    {
        float readPos = (float) write - delaySamples;
        while (readPos < 0.0f)
            readPos += (float) bufferSize;

        const int i0 = (int) readPos % bufferSize;
        const int i1 = (i0 + 1) % bufferSize;
        const float frac = readPos - std::floor (readPos);
        return lerpf (buffer[(size_t) i0], buffer[(size_t) i1], frac);
    }

    double sampleRate = 48000.0;
    int bufferSize = 0;
    int write = 0;
    std::vector<float> bufferL, bufferR;
    std::array<float, 4> phases { 0.0f, 0.31f, 0.63f, 0.86f };
    float slowPhase = 0.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
    float feedbackHighpassL = 0.0f, feedbackHighpassR = 0.0f;
    float toneL = 0.0f, toneR = 0.0f;
    float lowL = 0.0f, lowR = 0.0f;
    float feedbackHighpassCoeff = 1.0f, toneCoeff = 1.0f, lowShelfCoeff = 1.0f;
};

//==============================================================================
/** A larger, smoother plate than a stock Freeverb: six damped combs and three
    diffusing allpasses per side, a genuine stereo pre-delay and a high-passed
    wet path. It stays out of the way of the synth but adds the expensive-
    sounding air the CS-80 loves. */
class PlateReverb
{
public:
    void prepare (double newSampleRate)
    {
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
    }

    void process (float* left, float* right, int numSamples, float mix)
    {
        if (mix <= 0.0005f)
            return;

        constexpr float feedback = 0.862f;
        constexpr float damping = 0.23f;
        const float dryGain = 1.0f - 0.26f * mix;
        const float wetGain = 0.40f * mix;
        const float hpCoeff = 1.0f - std::exp (-2.0f * kPi * 170.0f / (float) sampleRate);
        const float lpCoeff = 1.0f - std::exp (-2.0f * kPi * 7600.0f / (float) sampleRate);
        const float modInc1 = 0.107f / (float) sampleRate;
        const float modInc2 = 0.163f / (float) sampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
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
            const float modA = std::sin (2.0f * kPi * modPhase1);
            const float modB = std::sin (2.0f * kPi * modPhase2);

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

            left[i] = left[i] * dryGain + wetLowpassL * wetGain;
            right[i] = right[i] * dryGain + wetLowpassR * wetGain;
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
};

//==============================================================================
/** One complete CS-80 channel card: two VCOs, a sub oscillator, a dedicated
    ring carrier, a high-pass into a low-pass and its own pair of envelopes.
    A voice owns two of these in parallel. */
struct Layer
{
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        osc1.reset (0.0);
        osc2.reset (0.33);
        subOsc.reset (0.17);
        sineOsc.reset (0.09);
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
        cutoffSmooth = 1400.0f;
        lastOutput = 0.0f;
        declickCorrection = 0.0f;
        declickRemaining = 0;
        declickPending = false;
        ringPhase = 0.0;
        noiseState = 0x12345678u;
    }

    void kill()
    {
        ampEnv.reset();
        filterEnv.reset();
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
        lastOutput = 0.0f;
        declickCorrection = 0.0f;
        declickRemaining = 0;
        declickPending = false;
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
    Oscillator osc1, osc2, subOsc, sineOsc;
    Envelope ampEnv, filterEnv;
    SVF hpf, lpf;
    SoftClipAdaa driveAdaa;
    float cutoffSmooth = 1400.0f;
    float lastOutput = 0.0f;
    float declickCorrection = 0.0f;
    int declickRemaining = 0;
    bool declickPending = false;
    double ringPhase = 0.0;
    uint32_t noiseState = 0x12345678u;
};

//==============================================================================
struct Voice
{
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;

        for (auto& layer : layers)
            layer.prepare (sampleRate);

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
                 float panA, float panB) noexcept
    {
        if (! active)
            return;

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
                         twangBlocks, layerCutoffDrift);

        if (useLayerB)
            renderLayer (1, bufferB, numSamples, p, mod, driftCents + layerDriftCents * 0.5f,
                         aftertouch, velocityOctaves, ampGain, glidePitchBlocks,
                         twangBlocks, layerCutoffDrift);

        const float angleA = (panA + 1.0f) * kPi * 0.25f;
        const float angleB = (panB + 1.0f) * kPi * 0.25f;
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
                      float layerCutoffDrift) noexcept
    {
        auto& layer = layers[(size_t) index];
        const bool second = index == 1;

        const float lpfRes = second ? p.layerRes : p.lpfRes;
        const float lpfQ = 0.7f + lpfRes * 12.6f;
        const float hpfQ = 0.7f + p.hpfRes * 7.0f;
        const float drive = 1.0f + p.drive * 2.3f + lpfRes * 0.85f;
        const float driveComp = 1.0f / (0.62f * drive + 0.38f);
        const float filterSat = 0.7f + p.drive * 2.4f + lpfRes * 2.6f;
        const float layerGain = second ? clampf (p.layerLevel, 0.0f, 1.0f) : 1.0f;
        const float layerAmp = ampGain * layerGain;
        const float pitchOffset = second ? p.layerOctave * 12.0f + p.layerDetune * 0.01f : 0.0f;
        const float pulseWidthBase = second ? p.layerPw : p.vco2Pw;
        const float cutoffOffset = second ? p.layerCutoff : 0.0f;
        const double ringIncrement = (double) p.ringSpeed / sampleRate;

        const float cutoffCoeff = smoothingCoefficient (0.0018f, sampleRate, kControlBlock);

        float lfoPhase = mod.lfoPhase;

        for (int start = 0; start < numSamples; start += kControlBlock)
        {
            const int n = std::min (kControlBlock, numSamples - start);
            const float glidePitchLocal = glidePitchBlocks[start / kControlBlock];
            const float twangSemis = twangBlocks[start / kControlBlock];

            const float lfo = lfoWaveform (p.lfoWave, lfoPhase);
            lfoPhase += mod.lfoRate * (float) n / (float) sampleRate;

            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            const float modAmount = lfo * mod.lfoFade;
            const float keytrack = (glidePitchLocal - 60.0f) / 12.0f * p.filterTrack;

            const float hpCutoff = clampf (p.hpfCutoff * std::exp2 ((glidePitchLocal - 60.0f) / 12.0f * 0.15f),
                                           20.0f, 6000.0f);
            layer.hpf.setCoefficients (sampleRate, hpCutoff, hpfQ);

            // Bend and ribbon glides are interpolated across the chunk, so the
            // pitch curve does not depend on the host's buffer size.
            const float progress = (float) start / (float) numSamples;
            const float baseSemis = lerpf (mod.bendSemisFrom, mod.bendSemisTo, progress)
                                  + lerpf (mod.ribbonSemisFrom, mod.ribbonSemisTo, progress)
                                  + (driftCents + vintagePitchCents) * 0.01f
                                  + p.tune * 0.01f;

            const float vibratoCents = modAmount
                                     * (p.lfoPitch * (18.0f + 42.0f * mod.modWheel) + aftertouch * 30.0f);

            const float pitchSemis = baseSemis + vibratoCents * 0.01f;

            const double f1 = 440.0 * std::exp2 ((glidePitchLocal - 69.0
                                                  + pitchSemis - twangSemis
                                                  + p.vco1Octave * 12.0
                                                  + p.vco1Detune * 0.01
                                                  + pitchOffset) / 12.0);

            const double f2 = 440.0 * std::exp2 ((glidePitchLocal - 69.0
                                                  + pitchSemis - twangSemis
                                                  + p.vco2Octave * 12.0
                                                  + p.vco2Detune * 0.01
                                                  + pitchOffset) / 12.0);

            layer.osc1.setFrequency (sampleRate, f1);
            layer.osc2.setFrequency (sampleRate, f2);
            layer.subOsc.setFrequency (sampleRate, f1 * 0.5);
            layer.sineOsc.setFrequency (sampleRate, f1);

            const float pulseWidth = clampf (pulseWidthBase
                                             + modAmount * p.lfoPwm * 0.38f
                                             + aftertouch * 0.11f,
                                             0.03f, 0.97f);

            const float cutoffOctaves = keytrack
                                      + (layer.filterEnv.getValue() - 0.35f) * 4.8f
                                      + modAmount * p.lfoFilter * 2.2f
                                      + aftertouch * 2.2f
                                      + velocityOctaves
                                      + vintageCutoffOct
                                      + layerCutoffDrift
                                      + cutoffOffset;

            const float targetCutoff = clampf (p.lpfCutoff * std::exp2 (cutoffOctaves),
                                               25.0f, (float) sampleRate * 0.45f);

            layer.cutoffSmooth += (targetCutoff - layer.cutoffSmooth) * cutoffCoeff;
            layer.cutoffSmooth = clampf (flushDenormal (layer.cutoffSmooth), 20.0f,
                                         (float) sampleRate * 0.45f);
            layer.lpf.setCoefficients (sampleRate, layer.cutoffSmooth, lpfQ);

            const float tremolo = 1.0f - p.lfoAmp * mod.lfoFade * (0.5f - 0.5f * lfo);

            for (int j = 0; j < n; ++j)
            {
                const float amp = layer.ampEnv.process();
                layer.filterEnv.process();

                const float saw = layer.osc1.saw();
                const float pulse = layer.osc2.pulse (pulseWidth);
                const float sub = layer.subOsc.sub();
                const float sine = layer.sineOsc.sine();
                const float noise = layer.nextNoise();

                const float carrier = std::sin (2.0f * kPi * (float) layer.ringPhase);
                layer.ringPhase += ringIncrement;

                while (layer.ringPhase >= 1.0)
                    layer.ringPhase -= 1.0;

                const float ringSignal = saw * (0.72f * carrier + 0.28f * pulse);

                float raw = 0.50f * (saw * p.vco1Level
                                     + pulse * p.vco2Level
                                     + sub * p.vco1Sub * 0.85f)
                          + 0.30f * ringSignal * p.ring
                          + 0.09f * noise * p.noise;

                raw = clampf (raw, -8.0f, 8.0f);

                const float highpassed = layer.hpf.highpass (raw, 0.8f + p.drive * 1.4f);

                // Anti-aliased saturation is the heart of the voice's warmth.
                const float driven = layer.driveAdaa.process (highpassed * drive) * driveComp;
                const float filtered = layer.lpf.lowpass (driven, filterSat)
                                     * (1.0f - 0.10f * lpfRes);

                // The VCA's sine bypass is a defining CS-80 detail: the filter
                // can thin and vowel the tone while the sine keeps the note
                // solid underneath it.
                const float mixed = filtered + sine * p.sineLevel * 0.85f;

                // A gentle VCA curve thickens the voice and rounds the peaks
                // the way the original's discrete output stage does.
                const float warmed = fastTanh (mixed * 1.16f) * (1.0f / 1.16f);

                float value = warmed * amp * layerAmp * tremolo * vintageLevel;

                // On a steal, match the previous voice's last sample and
                // settle to the new waveform over a fraction of a millisecond.
                if (layer.declickPending)
                {
                    layer.declickCorrection = layer.lastOutput - value;
                    layer.declickRemaining = 32;
                    layer.declickPending = false;
                }

                if (layer.declickRemaining > 0)
                {
                    value += layer.declickCorrection * ((float) layer.declickRemaining / 32.0f);
                    --layer.declickRemaining;
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
};
} // namespace

//==============================================================================
struct Engine::Impl
{
    double sampleRate = 48000.0;
    bool prepared = false;
    Params params;
    std::array<Voice, kNumVoices> voices;
    Chorus chorus;
    PlateReverb reverb;

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
            v.prepare (sampleRate);

        chorus.prepare (sampleRate);
        reverb.prepare (sampleRate);

        bendSmoothed = bendTarget;
        ribbonSmoothed = ribbonTarget;
        lfoPhase = 0.0f;
        lfoElapsed = 0.0f;
        gainSmoothed = dbToGain (params.masterDb) * kOutputTrim;
        limiterGain = 1.0f;
        hasRendered = false;
        brillianceLpL = brillianceLpR = 0.0f;
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

    void processChunk (float* left, float* right, int numSamples)
    {
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
        const bool spreadVoices = count > 1;
        int voiceIndex = 0;

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

            const float voicePan = spreadVoices
                                     ? ((voiceIndex % 2) == 0 ? -1.0f : 1.0f) * params.spread * 0.55f
                                     : 0.0f;

            const float panA = voicePan;
            const float panB = clampf (params.layerPan * 0.90f + voicePan * 0.55f, -1.0f, 1.0f);

            v.render (mixL.data(), mixR.data(), numSamples, params, mod,
                      useA, useB, panA, panB);

            bool aAlive = false, bAlive = false;
            layerActivity (v, useA, useB, aAlive, bAlive);

            const float level = v.active && (aAlive || bAlive) ? v.activityLevel() : 0.0f;
            auto& act = activity[(size_t) voiceIndex];
            act.store (std::max (level, act.load() * 0.55f));

            ++voiceIndex;
        }

        lfoPhase += params.lfoRate * (float) numSamples / (float) sampleRate;
        while (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;

        chorus.process (mixL.data(), mixR.data(), numSamples, params.ensemble);
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
        const float limiterAttack = 1.0f - std::exp (-1.0f / (0.0006f * (float) sampleRate));
        const float limiterRelease = 1.0f - std::exp (-1.0f / (0.140f * (float) sampleRate));

        float peakL = 0.0f, peakR = 0.0f;
        int writePos = scopeWrite.load (std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float l = mixL[(size_t) i];
            float r = mixR[(size_t) i];

            brillianceLpL += (l - brillianceLpL) * tiltCoeff;
            brillianceLpR += (r - brillianceLpR) * tiltCoeff;
            l += params.brilliance * (l - brillianceLpL) * 0.8f;
            r += params.brilliance * (r - brillianceLpR) * 0.8f;

            l *= gain;
            r *= gain;
            gain += gainStep;

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

    for (auto& v : impl->voices)
        v.prepare (sampleRate);

    impl->chorus.prepare (sampleRate);
    impl->reverb.prepare (sampleRate);
    impl->reset();
}

void Engine::reset()
{
    impl->reset();
}

double Engine::sampleRate() const { return impl->sampleRate; }
bool Engine::isPrepared() const { return impl->prepared; }

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
