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
    filter stays smooth, cheap enough to run eight voices without breaking a
    sweat. */
constexpr int kControlBlock = 8;
constexpr int kMixBlock = 512;

/** Fixed trim in front of the master fader. It leaves enough room for an
    eight-note chord to breathe before the output stage has to limit. */
constexpr float kOutputTrim = 0.70f;

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
        phases = { 0.0f, 0.37f, 0.71f };
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

        const float baseDelay[3] = { 0.0087f, 0.0137f, 0.0199f };
        const float depth[3] = { 0.0023f, 0.0031f, 0.0038f };
        const float rate[3] = { 0.19f, 0.27f, 0.14f };
        const float tapGain[3] = { 0.55f, 0.34f, 0.23f };

        const float dryGain = 1.0f - 0.20f * amount;
        const float wetGain = 0.66f * amount;
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

            for (int tap = 0; tap < 3; ++tap)
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
    std::array<float, 3> phases { 0.0f, 0.37f, 0.71f };
    float slowPhase = 0.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
    float feedbackHighpassL = 0.0f, feedbackHighpassR = 0.0f;
    float toneL = 0.0f, toneR = 0.0f;
    float lowL = 0.0f, lowR = 0.0f;
    float feedbackHighpassCoeff = 1.0f, toneCoeff = 1.0f, lowShelfCoeff = 1.0f;
};

//==============================================================================
/** A larger, smoother plate than a stock Freeverb: six damped combs and three
    diffusing allpasses per side, a short pre-delay and a high-passed wet path.
    It stays out of the way of the synth but adds the expensive-sounding air
    the CS-80 loves. */
class PlateReverb
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        const double scale = sampleRate / 44100.0;

        const int combDelays[6] = { 1116, 1188, 1277, 1356, 1422, 1491 };
        const int allpassDelays[3] = { 556, 441, 341 };

        for (int i = 0; i < 6; ++i)
        {
            combL[(size_t) i].resize ((int) (combDelays[i] * scale) + 4);
            combR[(size_t) i].resize ((int) ((combDelays[i] + 23) * scale) + 4);
        }

        for (int i = 0; i < 3; ++i)
        {
            allpassL[(size_t) i].resize ((int) (allpassDelays[i] * scale) + 4);
            allpassR[(size_t) i].resize ((int) ((allpassDelays[i] + 17) * scale) + 4);
        }

        preDelayL.resize ((int) (sampleRate * 0.019) + 4);
        preDelayR.resize ((int) (sampleRate * 0.026) + 4);

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
        wetHighpassL = wetHighpassR = 0.0f;
        wetLowpassL = wetLowpassR = 0.0f;
    }

    void process (float* left, float* right, int numSamples, float mix)
    {
        if (mix <= 0.0005f)
            return;

        constexpr float feedback = 0.855f;
        constexpr float damping = 0.22f;
        const float dryGain = 1.0f - 0.26f * mix;
        const float wetGain = 0.34f * mix;
        const float hpCoeff = 1.0f - std::exp (-2.0f * kPi * 170.0f / (float) sampleRate);
        const float lpCoeff = 1.0f - std::exp (-2.0f * kPi * 7600.0f / (float) sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // Pre-delay keeps the direct transient articulate.
            preDelayL[(size_t) preWriteL] = (left[i] + right[i]) * 0.32f;
            preDelayR[(size_t) preWriteR] = preDelayL[(size_t) preWriteL];

            // Read before writing, which turns each buffer into a real delay.
            const float inL = preDelayL[(size_t) preWriteL];
            const float inR = preDelayR[(size_t) preWriteR];

            preDelayL[(size_t) preWriteL] = (left[i] + right[i]) * 0.32f;
            preDelayR[(size_t) preWriteR] = (left[i] + right[i]) * 0.32f;

            if (++preWriteL >= (int) preDelayL.size())
                preWriteL = 0;
            if (++preWriteR >= (int) preDelayR.size())
                preWriteR = 0;

            float wetL = 0.0f, wetR = 0.0f;

            for (auto& comb : combL)
                wetL += processComb (comb, inL, feedback, damping);

            for (auto& comb : combR)
                wetR += processComb (comb, inR, feedback, damping);

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

        void resize (int size) { buffer.assign ((size_t) std::max (size, 8), 0.0f); }
    };

    struct Allpass
    {
        std::vector<float> buffer;
        int index = 0;

        void resize (int size) { buffer.assign ((size_t) std::max (size, 8), 0.0f); }
    };

    static float processComb (Comb& comb, float in, float feedback, float damping) noexcept
    {
        const float out = comb.buffer[(size_t) comb.index];
        comb.store = out * (1.0f - damping) + comb.store * damping;
        comb.buffer[(size_t) comb.index] = in + comb.store * feedback;

        if (++comb.index >= (int) comb.buffer.size())
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
    std::array<Allpass, 3> allpassL, allpassR;
    std::vector<float> preDelayL, preDelayR;
    int preWriteL = 0, preWriteR = 0;
    float wetHighpassL = 0.0f, wetHighpassR = 0.0f;
    float wetLowpassL = 0.0f, wetLowpassR = 0.0f;
};

//==============================================================================
struct Voice
{
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        osc1.reset (0.0);
        osc2.reset (0.33);
        subOsc.reset (0.17);
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
        active = false;
        released = false;
        held = false;
        cutoffSmooth = 1400.0f;
        glidePitch = 60.0f;
        glideTarget = 60.0f;
        lastOutput = 0.0f;
        declickCorrection = 0.0f;
        declickRemaining = 0;
        declickPending = false;
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
        const float ampStart = steal ? ampEnv.getValue() : 0.0f;
        const float filterStart = steal ? filterEnv.getValue() : p.fIl;

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

        if (! steal)
        {
            // Free-running phases. Eight voices that all start at phase zero
            // stack into one giant comb filter; scattered phases sound like
            // an ensemble of oscillators instead.
            osc1.reset (nextRandom01());
            osc2.reset (nextRandom01());
            subOsc.reset (nextRandom01());
            hpf.reset();
            lpf.reset();
            driveAdaa.reset();
            cutoffSmooth = p.lpfCutoff;
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

        // A stolen voice starts a very short declick ramp from its previous
        // output sample, so the changeover never produces a hard edge.
        glideOverride = 0.0f;
        declickPending = steal;
        declickRemaining = 0;

        ampEnv.trigger (ampStart);
        filterEnv.trigger (filterStart);

        if (! steal)
        {
            driftPhase1 = nextRandom01();
            driftPhase2 = nextRandom01();
        }

        pan = 0.0f;
        panL = 0.70710678f;
        panR = 0.70710678f;
    }

    void setPan (float newPan)
    {
        pan = clampf (newPan, -1.0f, 1.0f);
        const float angle = (pan + 1.0f) * kPi * 0.25f;
        panL = std::cos (angle);
        panR = std::sin (angle);
    }

    void releaseNow()
    {
        if (! active || released)
            return;

        ampEnv.release();
        filterEnv.release();
        released = true;
        held = false;
    }

    void kill()
    {
        active = false;
        released = false;
        held = false;
        ampEnv.reset();
        filterEnv.reset();
        hpf.reset();
        lpf.reset();
        driveAdaa.reset();
    }

    void render (float* destination, int numSamples, const Params& p,
                 const Modulation& mod) noexcept
    {
        if (! active)
            return;

        const float chunkSeconds = (float) numSamples / (float) sampleRate;

        // Slow, deterministic per-voice drift. Nothing in an analogue polysynth
        // ever sits perfectly still, and that tiny motion is most of what makes
        // stacked voices sound alive rather than sampled.
        driftPhase1 += 0.071f * chunkSeconds;
        driftPhase2 += 0.113f * chunkSeconds;

        if (driftPhase1 >= 1.0f)
            driftPhase1 -= 1.0f;

        if (driftPhase2 >= 1.0f)
            driftPhase2 -= 1.0f;

        const float vintage = clampf (p.vintage, 0.0f, 1.0f);
        const float driftCents = vintage * (driftOffset
                                          + 2.3f * std::sin (2.0f * kPi * driftPhase1)
                                          + 1.4f * std::sin (2.0f * kPi * driftPhase2));

        ampEnv.setTimeScale (vintageEnvScale);
        filterEnv.setTimeScale (vintageEnvScale);

        const float pressure = polyPressure >= 0.0f ? polyPressure : mod.channelPressure;
        const float aftertouch = pressure * p.aftertouch;

        const float velocityOctaves = (velocity - 0.5f) * p.velocity * 2.0f;

        // The CS-80's 12 dB low-pass gets liquid at high resonance; 13x gives
        // it a real singing peak without tipping into self-oscillation.
        const float lpfQ = 0.7f + p.lpfRes * 12.6f;
        const float hpfQ = 0.7f + p.hpfRes * 7.0f;

        const float drive = 1.0f + p.drive * 2.3f + p.lpfRes * 0.85f;
        const float driveComp = 1.0f / (0.62f * drive + 0.38f);
        const float filterSat = 0.7f + p.drive * 2.4f + p.lpfRes * 2.6f;

        const float velocityGain = 1.0f - p.velocity + p.velocity * velocity;
        const float ampGain = velocityGain * (1.0f + 0.16f * aftertouch);
        const float cutoffCoeff = smoothingCoefficient (0.0018f, sampleRate, kControlBlock);

        // Portamento is polyphonic: the new voice glides from the last note
        // played, while a stolen voice glides from its own pitch.
        const float glideSeconds = p.glide > 0.0005f ? p.glide : glideOverride;
        const bool glideActive = glideSeconds > 0.0005f;
        const float glideTau = std::max (glideSeconds / 4.5f, 1.0e-4f);

        float lfoPhase = mod.lfoPhase;
        float glidePitchLocal = glidePitch;

        for (int start = 0; start < numSamples; start += kControlBlock)
        {
            const int n = std::min (kControlBlock, numSamples - start);

            if (glideActive)
            {
                const float glideCoeff = 1.0f - std::exp (-(float) n
                                                          / (glideTau * (float) sampleRate));
                glidePitchLocal += (glideTarget - glidePitchLocal) * glideCoeff;
            }
            else
            {
                glidePitchLocal = glideTarget;
            }

            const float lfo = lfoWaveform (p.lfoWave, lfoPhase);
            lfoPhase += mod.lfoRate * (float) n / (float) sampleRate;

            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;

            const float modAmount = lfo * mod.lfoFade;
            const float keytrack = (glidePitchLocal - 60.0f) / 12.0f * p.filterTrack;

            const float hpCutoff = clampf (p.hpfCutoff * std::exp2 ((glidePitchLocal - 60.0f) / 12.0f * 0.15f),
                                           20.0f, 6000.0f);
            hpf.setCoefficients (sampleRate, hpCutoff, hpfQ);

            // Bend and ribbon glides are interpolated across the chunk, so the
            // pitch curve does not depend on the host's buffer size.
            const float progress = (float) start / (float) numSamples;
            const float baseSemis = lerpf (mod.bendSemisFrom, mod.bendSemisTo, progress)
                                  + lerpf (mod.ribbonSemisFrom, mod.ribbonSemisTo, progress)
                                  + (driftCents + vintagePitchCents) * 0.01f
                                  + p.tune * 0.01f;

            const float vibratoCents = modAmount
                                     * (p.lfoPitch * (18.0f + 42.0f * mod.modWheel) + aftertouch * 22.0f);

            const float pitchSemis = baseSemis + vibratoCents * 0.01f;

            const double f1 = 440.0 * std::exp2 ((glidePitchLocal - 69.0
                                                  + pitchSemis
                                                  + p.vco1Octave * 12.0
                                                  + p.vco1Detune * 0.01) / 12.0);

            const double f2 = 440.0 * std::exp2 ((glidePitchLocal - 69.0
                                                  + pitchSemis
                                                  + p.vco2Octave * 12.0
                                                  + p.vco2Detune * 0.01) / 12.0);

            osc1.setFrequency (sampleRate, f1);
            osc2.setFrequency (sampleRate, f2);
            subOsc.setFrequency (sampleRate, f1 * 0.5);

            const float pulseWidth = clampf (p.vco2Pw
                                             + modAmount * p.lfoPwm * 0.35f
                                             + aftertouch * 0.08f,
                                             0.03f, 0.97f);

            const float cutoffOctaves = keytrack
                                      + (filterEnv.getValue() - 0.35f) * 4.6f
                                      + modAmount * p.lfoFilter * 2.0f
                                      + aftertouch * 1.6f
                                      + velocityOctaves
                                      + vintageCutoffOct;

            const float targetCutoff = clampf (p.lpfCutoff * std::exp2 (cutoffOctaves),
                                               25.0f, (float) sampleRate * 0.45f);

            cutoffSmooth += (targetCutoff - cutoffSmooth) * cutoffCoeff;
            cutoffSmooth = clampf (flushDenormal (cutoffSmooth), 20.0f, (float) sampleRate * 0.45f);
            lpf.setCoefficients (sampleRate, cutoffSmooth, lpfQ);

            const float tremolo = 1.0f - p.lfoAmp * mod.lfoFade * (0.5f - 0.5f * lfo);

            for (int j = 0; j < n; ++j)
            {
                const float amp = ampEnv.process();
                filterEnv.process();

                const float saw = osc1.saw();
                const float pulse = osc2.pulse (pulseWidth);
                const float sub = subOsc.sub();
                const float noise = nextNoise();

                float raw = 0.50f * (saw * p.vco1Level
                                     + pulse * p.vco2Level
                                     + sub * p.vco1Sub * 0.85f)
                          + 0.30f * (saw * pulse) * p.ring
                          + 0.09f * noise * p.noise;

                raw = clampf (raw, -8.0f, 8.0f);

                const float highpassed = hpf.highpass (raw, 0.8f + p.drive * 1.4f);

                // Anti-aliased saturation is the heart of the voice's warmth.
                const float driven = driveAdaa.process (highpassed * drive) * driveComp;
                const float filtered = lpf.lowpass (driven, filterSat);

                float value = filtered * amp * ampGain * tremolo * vintageLevel;

                // On a steal, match the previous voice's last sample and
                // settle to the new waveform over a fraction of a millisecond.
                if (declickPending)
                {
                    declickCorrection = lastOutput - value;
                    declickRemaining = 32;
                    declickPending = false;
                }

                if (declickRemaining > 0)
                {
                    value += declickCorrection * ((float) declickRemaining / 32.0f);
                    --declickRemaining;
                }

                lastOutput = value;
                destination[start + j] += value;
            }
        }

        glidePitch = glidePitchLocal;

        if (! ampEnv.isActive())
            active = false;
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
    bool active = false;
    bool released = false;
    bool held = false;
    int note = 60;
    float velocity = 0.8f;
    float polyPressure = -1.0f;
    uint64_t age = 0;
    uint32_t noiseState = 0x12345678u;
    uint32_t varianceState = 0xfeedfaceu;

    Oscillator osc1, osc2, subOsc;
    Envelope ampEnv, filterEnv;
    SVF hpf, lpf;
    SoftClipAdaa driveAdaa;
    float cutoffSmooth = 1400.0f;
    float driftPhase1 = 0.0f, driftPhase2 = 0.0f, driftOffset = 0.0f;
    float glidePitch = 60.0f, glideTarget = 60.0f, glideOverride = 0.0f;
    float lastOutput = 0.0f;
    float declickCorrection = 0.0f;
    int declickRemaining = 0;
    bool declickPending = false;

    float vintagePitchCents = 0.0f;
    float vintageCutoffOct = 0.0f;
    float vintageLevel = 1.0f;
    float vintageEnvScale = 1.0f;

    float pan = 0.0f, panL = 0.70710678f, panR = 0.70710678f;
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
    std::array<std::atomic<float>, kScopeSize> scope {};
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
    bool hasRendered = false;
    float brillianceLpL = 0.0f, brillianceLpR = 0.0f;
    float dcX1L = 0.0f, dcY1L = 0.0f, dcX1R = 0.0f, dcY1R = 0.0f;

    std::array<float, kMixBlock> mixL {}, mixR {}, voiceBuffer {};

    Impl()
    {
        params = defaultParams();

        for (auto& a : activity)
            a.store (0.0f);

        for (auto& s : scope)
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
            v.ampEnv.setTimes (params.aA, params.aD, params.aR);
            v.ampEnv.setLevels (1.0f, params.aS);

            v.filterEnv.setTimes (params.fA, params.fD, params.fR);
            v.filterEnv.setLevels (params.fAl, params.fS);
        }
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

            if (spreadVoices)
                v.setPan (((voiceIndex % 2) == 0 ? -1.0f : 1.0f) * params.spread * 0.55f);
            else
                v.setPan (0.0f);

            std::fill_n (voiceBuffer.begin(), numSamples, 0.0f);
            v.render (voiceBuffer.data(), numSamples, params, mod);

            for (int i = 0; i < numSamples; ++i)
            {
                mixL[(size_t) i] += voiceBuffer[(size_t) i] * v.panL;
                mixR[(size_t) i] += voiceBuffer[(size_t) i] * v.panR;
            }

            const float level = v.active ? v.ampEnv.getValue() * v.velocity : 0.0f;
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

            l = fastTanh (l);
            r = fastTanh (r);

            // Belt and braces: the plugin never hands a host samples outside
            // [-1, 1], even if a pathological patch overshoots the limiter.
            l = clampf (l, -1.0f, 1.0f);
            r = clampf (r, -1.0f, 1.0f);

            left[i] = l;
            right[i] = r;

            peakL = std::max (peakL, std::abs (l));
            peakR = std::max (peakR, std::abs (r));

            scope[(size_t) writePos].store ((l + r) * 0.5f, std::memory_order_relaxed);
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
        std::fill_n (left, numSamples, 0.0f);
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

        destination[i] = impl->scope[(size_t) index].load (std::memory_order_relaxed);
    }

    return count;
}
} // namespace cs80
