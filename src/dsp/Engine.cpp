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
constexpr int kControlBlock = 16;
constexpr int kMixBlock = 512;

inline float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db / 20.0f);
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

struct Modulation
{
    float lfo = 0.0f;
    float lfoFade = 0.0f;
    float bendSemis = 0.0f;
    float ribbonSemis = 0.0f;
    float modWheel = 0.0f;
    float channelPressure = 0.0f;
};

//==============================================================================
/** Three-tap ensemble. The CS-80's chorus is a huge part of its character, so
    it lives on the voice bus rather than as a bypassable afterthought. */
class Chorus
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        bufferSize = std::max (64, (int) (sampleRate * 0.05) + 8);
        bufferL.assign ((size_t) bufferSize, 0.0f);
        bufferR.assign ((size_t) bufferSize, 0.0f);
        write = 0;
        phases = { 0.0f, 0.33f, 0.67f };
        feedbackL = feedbackR = 0.0f;
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        write = 0;
        phases = { 0.0f, 0.33f, 0.67f };
        feedbackL = feedbackR = 0.0f;
    }

    void process (float* left, float* right, int numSamples, float amount)
    {
        if (amount <= 0.0005f || bufferSize <= 0)
            return;

        const float baseDelay[3] = { 0.0095f, 0.0145f, 0.0210f };
        const float depth = 0.0024f;
        const float rate[3] = { 0.31f, 0.43f, 0.27f };
        const float tapGain[3] = { 0.5f, 0.32f, 0.22f };

        const float dryGain = 1.0f - 0.30f * amount;
        const float wetGain = 0.72f * amount;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = right[i];

            bufferL[(size_t) write] = inL + feedbackL * 0.12f;
            bufferR[(size_t) write] = inR + feedbackR * 0.12f;

            float wetL = 0.0f, wetR = 0.0f;

            for (int tap = 0; tap < 3; ++tap)
            {
                const float modL = std::sin (2.0f * kPi * phases[(size_t) tap]);
                const float modR = std::sin (2.0f * kPi * (phases[(size_t) tap] + 0.37f));

                const float delayL = (baseDelay[tap] + depth * modL) * (float) sampleRate;
                const float delayR = (baseDelay[tap] + depth * modR) * (float) sampleRate;

                wetL += tapGain[tap] * readInterpolated (bufferL, delayL);
                wetR += tapGain[tap] * readInterpolated (bufferR, delayR);

                phases[(size_t) tap] += rate[tap] / (float) sampleRate;
                if (phases[(size_t) tap] >= 1.0f)
                    phases[(size_t) tap] -= 1.0f;
            }

            feedbackL = wetL;
            feedbackR = wetR;

            left[i] = inL * dryGain + wetL * wetGain;
            right[i] = inR * dryGain + wetR * wetGain;

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
    std::array<float, 3> phases { 0.0f, 0.33f, 0.67f };
    float feedbackL = 0.0f, feedbackR = 0.0f;
};

//==============================================================================
/** Small Freeverb-flavoured plate. Adds the air the raw synth lacks without
    turning the instrument into an effects unit. */
class PlateReverb
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        const double scale = sampleRate / 44100.0;

        const int combDelays[4] = { 1116, 1188, 1277, 1356 };
        const int allpassDelays[2] = { 556, 441 };

        for (int i = 0; i < 4; ++i)
        {
            combL[(size_t) i].resize ((int) (combDelays[i] * scale) + 4);
            combR[(size_t) i].resize ((int) ((combDelays[i] + 23) * scale) + 4);
        }

        for (int i = 0; i < 2; ++i)
        {
            allpassL[(size_t) i].resize ((int) (allpassDelays[i] * scale) + 4);
            allpassR[(size_t) i].resize ((int) ((allpassDelays[i] + 17) * scale) + 4);
        }

        reset();
    }

    void reset()
    {
        for (auto& c : combL) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.index = 0; c.store = 0.0f; }
        for (auto& c : combR) { std::fill (c.buffer.begin(), c.buffer.end(), 0.0f); c.index = 0; c.store = 0.0f; }
        for (auto& a : allpassL) { std::fill (a.buffer.begin(), a.buffer.end(), 0.0f); a.index = 0; }
        for (auto& a : allpassR) { std::fill (a.buffer.begin(), a.buffer.end(), 0.0f); a.index = 0; }
    }

    void process (float* left, float* right, int numSamples, float mix)
    {
        if (mix <= 0.0005f)
            return;

        constexpr float feedback = 0.83f;
        constexpr float damping = 0.26f;
        const float dryGain = 1.0f - 0.30f * mix;
        const float wetGain = 0.30f * mix;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = (left[i] + right[i]) * 0.32f;
            float wetL = 0.0f, wetR = 0.0f;

            for (auto& comb : combL)
                wetL += processComb (comb, in, feedback, damping);

            for (auto& comb : combR)
                wetR += processComb (comb, in, feedback, damping);

            for (auto& ap : allpassL)
                wetL = processAllpass (ap, wetL);

            for (auto& ap : allpassR)
                wetR = processAllpass (ap, wetR);

            left[i] = left[i] * dryGain + wetL * wetGain;
            right[i] = right[i] * dryGain + wetR * wetGain;
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
    std::array<Comb, 4> combL, combR;
    std::array<Allpass, 2> allpassL, allpassR;
};

//==============================================================================
struct Voice
{
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        osc1.reset (0.0);
        osc2.reset (0.33);
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        hpf.reset();
        lpf.reset();
    }

    void start (int midiNote, float newVelocity, uint64_t newAge, const Params& p)
    {
        active = true;
        released = false;
        held = false;
        note = midiNote;
        velocity = clampf (newVelocity, 0.0f, 1.0f);
        age = newAge;
        polyPressure = -1.0f;

        osc1.reset (0.0);
        osc2.reset (0.33);

        ampEnv.trigger (0.0f);
        filterEnv.trigger (p.fIl);

        hpf.reset();
        lpf.reset();
        lastCutoff = p.lpfCutoff;
        noiseState = 0x9e3779b9u ^ ((uint32_t) midiNote * 2654435761u);

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
    }

    void render (float* destination, int numSamples, const Params& p,
                 const Modulation& mod) noexcept
    {
        if (! active)
            return;

        const float pressure = polyPressure >= 0.0f ? polyPressure : mod.channelPressure;
        const float aftertouch = pressure * p.aftertouch;

        const float vibratoCents = mod.lfo * mod.lfoFade
                                 * (p.lfoPitch * (18.0f + 42.0f * mod.modWheel) + aftertouch * 22.0f);

        const float pitchSemis = mod.bendSemis + mod.ribbonSemis + vibratoCents / 100.0f;

        const double f1 = 440.0 * std::exp2 ((note - 69.0
                                              + pitchSemis
                                              + p.vco1Octave * 12.0
                                              + p.vco1Detune / 100.0) / 12.0);

        const double f2 = 440.0 * std::exp2 ((note - 69.0
                                              + pitchSemis
                                              + p.vco2Octave * 12.0
                                              + p.vco2Detune / 100.0) / 12.0);

        osc1.setFrequency (sampleRate, f1);
        osc2.setFrequency (sampleRate, f2);

        const float pulseWidth = clampf (p.vco2Pw
                                         + mod.lfo * mod.lfoFade * p.lfoPwm * 0.35f
                                         + aftertouch * 0.08f,
                                         0.03f, 0.97f);

        const float keytrack = (float) (note - 60) / 12.0f * 0.35f;
        const float cutoffOctaves = keytrack
                                  + (filterEnv.getValue() - 0.35f) * 4.5f
                                  + mod.lfo * mod.lfoFade * p.lfoFilter * 2.0f
                                  + aftertouch * 1.6f;

        const float targetCutoff = clampf (p.lpfCutoff * std::exp2 (cutoffOctaves),
                                           25.0f, (float) sampleRate * 0.45f);
        lastCutoff += (targetCutoff - lastCutoff) * 0.35f;
        lastCutoff = flushDenormal (lastCutoff);

        const float hpCutoff = clampf (p.hpfCutoff * std::exp2 ((float) (note - 60) / 12.0f * 0.15f),
                                       20.0f, 6000.0f);

        hpf.setCoefficients (sampleRate, hpCutoff, 0.7f + p.hpfRes * 6.0f);
        lpf.setCoefficients (sampleRate, lastCutoff, 0.7f + p.lpfRes * 11.0f);

        const float velocityGain = 1.0f - p.velocity + p.velocity * velocity;
        const float tremoloDepth = p.lfoAmp * mod.lfoFade;
        const float ampGain = velocityGain * (1.0f + 0.15f * aftertouch);

        for (int i = 0; i < numSamples; ++i)
        {
            const float amp = ampEnv.process();
            filterEnv.process();

            const float saw = osc1.saw();
            const float pulse = osc2.pulse (pulseWidth);
            const float noise = nextNoise();

            const float raw = 0.55f * (saw * p.vco1Level + pulse * p.vco2Level)
                            + 0.32f * (saw * pulse) * p.ring
                            + 0.10f * noise * p.noise;

            const float filtered = lpf.lowpass (hpf.highpass (raw));
            const float tremolo = 1.0f - tremoloDepth * (0.5f - 0.5f * mod.lfo);

            destination[i] += filtered * amp * ampGain * tremolo;
        }

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

    double sampleRate = 48000.0;
    bool active = false;
    bool released = false;
    bool held = false;
    int note = 60;
    float velocity = 0.8f;
    float polyPressure = -1.0f;
    uint64_t age = 0;
    uint32_t noiseState = 0x12345678u;

    Oscillator osc1, osc2;
    Envelope ampEnv, filterEnv;
    SVF hpf, lpf;
    float lastCutoff = 2400.0f;
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
    float gainSmoothed = 0.6f;
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
        gainSmoothed = dbToGain (params.masterDb) * 0.80f;
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

        const int count = std::count_if (voices.begin(), voices.end(),
                                         [] (const Voice& v) { return v.active; });

        bendSmoothed += (bendTarget - bendSmoothed) * 0.25f;
        ribbonSmoothed += (ribbonTarget - ribbonSmoothed) * 0.45f;

        if (count == 0)
        {
            lfoElapsed = 0.0f;
        }
        else
        {
            lfoElapsed += (float) numSamples / (float) sampleRate;
        }

        lfoPhase += params.lfoRate * (float) numSamples / (float) sampleRate;
        while (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;

        const float lfo = lfoWaveform (params.lfoWave, lfoPhase);
        const float fade = params.lfoDelay <= 0.0f
                             ? (count > 0 ? 1.0f : 0.0f)
                             : clampf ((lfoElapsed - params.lfoDelay) / 0.35f, 0.0f, 1.0f);

        Modulation mod;
        mod.lfo = lfo;
        mod.lfoFade = fade;
        mod.bendSemis = bendSmoothed * params.bendRange;
        mod.ribbonSemis = ribbonSmoothed * 12.0f;
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

        chorus.process (mixL.data(), mixR.data(), numSamples, params.ensemble);
        reverb.process (mixL.data(), mixR.data(), numSamples, params.space);

        // A little fixed headroom trim keeps eight voices + ensemble from
        // sitting on the limiter, while the master fader keeps its dB scale.
        constexpr float outputTrim = 0.80f;
        const float targetGain = dbToGain (params.masterDb) * outputTrim;
        gainSmoothed += (targetGain - gainSmoothed) * 0.08f;

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

            l *= gainSmoothed;
            r *= gainSmoothed;

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
        impl->gainSmoothed = dbToGain (newParams.masterDb) * 0.80f;
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
        voice->start (midiNote, velocity, impl->ageCounter++, impl->params);
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
