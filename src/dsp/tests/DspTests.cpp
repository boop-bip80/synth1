// Headless test suite for the CS-80 engine. No JUCE, no audio device, no
// display: this is the fast feedback loop that also runs on every CI platform.

#include "../Dsp.h"
#include "../Engine.h"
#include "../Presets.h"
#include "Spectrum.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace cs80;

//==============================================================================
// Allocation tracking. The audio callback must not touch the heap after
// prepare(); this replaces the global allocator so the test can prove it.
namespace
{
std::atomic<long> gAllocationCount { 0 };
std::atomic<bool> gTrackAllocations { false };
} // namespace

void* operator new (std::size_t size)
{
    if (gTrackAllocations.load (std::memory_order_relaxed))
        gAllocationCount.fetch_add (1, std::memory_order_relaxed);

    if (auto* p = std::malloc (size == 0 ? 1 : size))
        return p;

    throw std::bad_alloc();
}

void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void* operator new[] (std::size_t size) { return operator new (size); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

// The nothrow forms must be replaced alongside the throwing forms. MSVC's
// CRT keeps them in the same new.obj object; defining only the throwing
// overloads lets the linker pull that object in and report LNK2005.
void* operator new (std::size_t size, const std::nothrow_t&) noexcept
{
    try { return operator new (size); } catch (...) { return nullptr; }
}

void* operator new[] (std::size_t size, const std::nothrow_t&) noexcept
{
    try { return operator new[] (size); } catch (...) { return nullptr; }
}

void operator delete (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete[] (void* p, const std::nothrow_t&) noexcept { std::free (p); }

namespace
{
int gChecks = 0;
int gFailures = 0;

void check (bool condition, const char* message, int line)
{
    ++gChecks;

    if (! condition)
    {
        ++gFailures;
        std::printf ("  FAIL  %s  (line %d)\n", message, line);
    }
}

#define CHECK(cond) check ((cond), #cond, __LINE__)

void checkNear (float actual, float expected, float tolerance, const char* message, int line)
{
    ++gChecks;

    if (! (std::abs (actual - expected) <= tolerance))
    {
        ++gFailures;
        std::printf ("  FAIL  %s: got %.5f, expected %.5f +/- %.5f (line %d)\n",
                     message, actual, expected, tolerance, line);
    }
}

#define CHECK_NEAR(a, e, t) checkNear ((float) (a), (float) (e), (float) (t), #a, __LINE__)

struct StereoBuffer
{
    std::vector<float> left, right;

    explicit StereoBuffer (int samples) : left ((size_t) samples, 0.0f), right ((size_t) samples, 0.0f) {}
};

StereoBuffer render (Engine& engine, int samples, int blockSize = 64)
{
    StereoBuffer buffer (samples);

    for (int done = 0; done < samples; done += blockSize)
    {
        const int n = std::min (blockSize, samples - done);
        engine.process (buffer.left.data() + done, buffer.right.data() + done, n);
    }

    return buffer;
}

float peakOf (const StereoBuffer& buffer, int from = 0, int to = -1)
{
    if (to < 0 || to > (int) buffer.left.size())
        to = (int) buffer.left.size();

    float peak = 0.0f;

    for (int i = std::max (0, from); i < to; ++i)
        peak = std::max (peak, std::max (std::abs (buffer.left[(size_t) i]),
                                         std::abs (buffer.right[(size_t) i])));

    return peak;
}

float rmsOf (const StereoBuffer& buffer, int from = 0, int to = -1)
{
    if (to < 0 || to > (int) buffer.left.size())
        to = (int) buffer.left.size();

    double sum = 0.0;
    int count = 0;

    for (int i = std::max (0, from); i < to; ++i)
    {
        sum += (double) buffer.left[(size_t) i] * buffer.left[(size_t) i];
        sum += (double) buffer.right[(size_t) i] * buffer.right[(size_t) i];
        count += 2;
    }

    return count > 0 ? (float) std::sqrt (sum / count) : 0.0f;
}

bool allFinite (const StereoBuffer& buffer)
{
    for (size_t i = 0; i < buffer.left.size(); ++i)
        if (! std::isfinite (buffer.left[i]) || ! std::isfinite (buffer.right[i]))
            return false;

    return true;
}

int zeroCrossings (const std::vector<float>& samples, float threshold = 0.02f)
{
    int crossings = 0;
    int lastSign = 0;

    for (float value : samples)
    {
        if (value > threshold)
        {
            if (lastSign < 0) // rising crossings only: one per cycle
                ++crossings;

            lastSign = 1;
        }
        else if (value < -threshold)
        {
            lastSign = -1;
        }
    }

    return crossings;
}

float estimateFrequency (const std::vector<float>& samples, double sampleRate)
{
    return (float) zeroCrossings (samples) * (float) sampleRate / (float) samples.size();
}

Params plainPatch()
{
    Params p = defaultParams();
    p.layerMode = 0.0f;
    p.layerLevel = 0.0f;
    p.ensemble = 0.0f;
    p.space = 0.0f;
    p.spread = 0.0f;
    p.ring = 0.0f;
    p.noise = 0.0f;
    p.vco2Level = 0.0f;
    p.vco1Level = 1.0f;
    p.vco1Sub = 0.0f;
    p.filterTrack = 0.0f;
    p.drive = 0.25f;
    p.glide = 0.0f;
    p.tune = 0.0f;
    p.vintage = 0.0f;
    p.hpfCutoff = 20.0f;
    p.hpfRes = 0.0f;
    p.lpfCutoff = 9000.0f;
    p.lpfRes = 0.0f;
    p.fIl = 1.0f;
    p.fAl = 1.0f;
    p.fS = 1.0f;
    p.fA = 0.01f;
    p.fD = 0.01f;
    p.fR = 0.05f;
    p.aA = 0.002f;
    p.aD = 0.01f;
    p.aS = 1.0f;
    p.aR = 0.05f;
    p.lfoPitch = 0.0f;
    p.lfoPwm = 0.0f;
    p.lfoFilter = 0.0f;
    p.lfoAmp = 0.0f;
    p.aftertouch = 0.0f;
    p.velocity = 0.0f;
    p.masterDb = -6.0f;
    return p;
}

//==============================================================================
void testParameterTable()
{
    std::printf ("- parameter table and mappings\n");

    std::set<std::string> ids;
    CHECK (paramSpecs().size() >= 50);

    for (const auto& spec : paramSpecs())
    {
        CHECK (ids.insert (spec.id).second);
        CHECK (spec.minValue < spec.maxValue);
        CHECK (spec.defaultValue >= spec.minValue && spec.defaultValue <= spec.maxValue);
        CHECK (spec.skew > 0.0f);
        CHECK (isKnownParam (spec.id));
        CHECK (findSpec (spec.id) != nullptr);
        CHECK (specIndex (spec.id) >= 0);

        Params p = defaultParams();
        float mid = (spec.minValue + spec.maxValue) * 0.5f;

        // Stepped parameters (octave switches, choices, matrix routing) snap
        // to their interval, so the round-trip expectation must snap too.
        if (spec.steps > 0)
            mid = std::round (mid);

        setParam (p, spec.id, mid);
        CHECK_NEAR (getParam (p, spec.id), mid, 0.0001f);

        setParam (p, spec.id, spec.maxValue + 1000.0f);
        CHECK_NEAR (getParam (p, spec.id), spec.maxValue, 0.0001f);
    }

    Params defaults = defaultParams();

    for (const auto& spec : paramSpecs())
        CHECK_NEAR (getParam (defaults, spec.id), spec.defaultValue, 0.0001f);

    CHECK (! isKnownParam ("nope"));
    CHECK (getParam (defaults, "unknown") == 0.0f);
}

void testPresets()
{
    std::printf ("- factory presets\n");

    const auto& presets = factoryPresets();
    CHECK (presets.size() >= 24);

    std::set<std::string> names;

    for (const auto& preset : presets)
    {
        CHECK (names.insert (preset.name).second);
        CHECK (preset.category[0] != '\0');
        CHECK (preset.blurb[0] != '\0');

        Params p = defaultParams();
        applyPreset (p, preset);

        for (const auto& spec : paramSpecs())
        {
            const float value = getParam (p, spec.id);
            CHECK (value >= spec.minValue - 0.0001f && value <= spec.maxValue + 0.0001f);
        }

        for (const auto& override : preset.values)
            CHECK (isKnownParam (override.id));
    }

    CHECK (findPreset ("Blade Brass") != nullptr);
    CHECK (findPreset ("Does Not Exist") == nullptr);
}

void testBasicRenderAndSilence()
{
    std::printf ("- basic render, silence and finiteness\n");

    Engine engine;
    engine.prepare (48000.0);

    auto silence = render (engine, 4800);
    CHECK (allFinite (silence));
    CHECK (peakOf (silence) < 1.0e-7f);

    engine.setParams (defaultParams());
    engine.noteOn (60, 0.9f);
    auto sound = render (engine, 24000);
    CHECK (allFinite (sound));
    CHECK (peakOf (sound) > 0.01f);
    CHECK (peakOf (sound) <= 1.0f);
    CHECK (rmsOf (sound) > 0.001f);
    CHECK (engine.activeVoiceCount() == 1);

    Params dry = defaultParams();
    dry.ensemble = 0.0f;
    dry.space = 0.0f;
    engine.setParams (dry);
    engine.allNotesOff (true);

    // One second lets the ensemble / plate wet gains fade out and the DC
    // blocker recover from the hard kill. Their tails are intentionally
    // audible while fading; the steady state must be silent.
    (void) render (engine, 48000, 128);
    auto after = render (engine, 4800);
    std::printf ("    residual peak after kill+1s: %.9f\n", peakOf (after));
    CHECK (engine.activeVoiceCount() == 0);
    CHECK (peakOf (after) < 1.0e-5f);
}

void testPolyphonyAndStealing()
{
    std::printf ("- polyphony and voice stealing\n");

    Engine engine;
    engine.prepare (48000.0);
    engine.setParams (defaultParams());

    for (int note = 48; note < 60; ++note)
        engine.noteOn (note, 0.8f);

    auto buffer = render (engine, 4800);
    CHECK (allFinite (buffer));
    CHECK (engine.activeVoiceCount() <= Engine::kNumVoices);
    CHECK (engine.activeVoiceCount() == Engine::kNumVoices);
    CHECK (peakOf (buffer) <= 1.0f);

    float anyActivity = 0.0f;
    for (int v = 0; v < Engine::kNumVoices; ++v)
        anyActivity = std::max (anyActivity, engine.voiceActivity (v));

    CHECK (anyActivity > 0.0f);
    CHECK (engine.voiceActivity (-1) == 0.0f);
    CHECK (engine.voiceActivity (99) == 0.0f);
}

void testTuning()
{
    std::printf ("- tuning and pitch bend\n");

    for (double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        Engine engine;
        engine.prepare (sampleRate);
        engine.setParams (plainPatch());
        engine.noteOn (69, 1.0f);

        auto buffer = render (engine, (int) sampleRate / 2);
        const float frequency = estimateFrequency (buffer.left, sampleRate);

        std::printf ("    A4 at %.0f Hz -> %.1f Hz\n", sampleRate, frequency);
        CHECK (std::abs (frequency - 440.0f) < 440.0f * 0.03f);
    }

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.bendRange = 24.0f;
    engine.setParams (p);
    engine.pitchBend (1.0f);
    engine.noteOn (69, 1.0f);

    auto bent = render (engine, 24000);
    const float bentFrequency = estimateFrequency (bent.left, 48000.0);
    std::printf ("    A4 + 24 semitones -> %.1f Hz\n", bentFrequency);
    CHECK (std::abs (bentFrequency - 1760.0f) < 1760.0f * 0.06f);
}

void testOctaveSwitch()
{
    std::printf ("- oscillator octave switch\n");

    for (float octave : { -1.0f, 1.0f })
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.vco1Octave = octave;
        engine.setParams (p);
        engine.noteOn (69, 1.0f);

        auto buffer = render (engine, 24000);
        const float frequency = estimateFrequency (buffer.left, 48000.0);
        const float expected = 440.0f * std::pow (2.0f, octave);
        std::printf ("    octave %.0f -> %.1f Hz (expected %.0f)\n", octave, frequency, expected);
        CHECK (std::abs (frequency - expected) < expected * 0.05f);
    }
}

void testSubOscillator()
{
    std::printf ("- sub oscillator tracks an octave below VCO 1\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.vco1Level = 0.0f;
    p.vco1Sub = 1.0f;
    p.lpfCutoff = 3000.0f;
    engine.setParams (p);
    engine.noteOn (57, 1.0f); // A3 = 220 Hz; the sub should sit at 110 Hz

    auto buffer = render (engine, 48000);
    const float frequency = estimateFrequency (buffer.left, 48000.0);
    std::printf ("    sub frequency %.1f Hz (expected 110)\n", frequency);
    CHECK (std::abs (frequency - 110.0f) < 8.0f);
}

void testGlide()
{
    std::printf ("- polyphonic portamento glides between notes\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.aR = 0.001f;
    p.glide = 0.35f;
    engine.setParams (p);

    engine.noteOn (48, 1.0f); // C3
    (void) render (engine, 4800);
    engine.noteOff (48);
    (void) render (engine, 4800);

    engine.noteOn (60, 1.0f); // C4, glides up from C3
    auto early = render (engine, 2400);
    auto late = render (engine, 36000);

    const float earlyFrequency = estimateFrequency (early.left, 48000.0);
    const float lateFrequency = estimateFrequency (late.left, 48000.0);

    std::printf ("    glide early %.1f Hz, settled %.1f Hz (expected 261.6)\n",
                 earlyFrequency, lateFrequency);
    CHECK (earlyFrequency < lateFrequency * 0.92f);
    CHECK (std::abs (lateFrequency - 261.6f) < 261.6f * 0.05f);
}

void testMasterTune()
{
    std::printf ("- master tune\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.tune = 50.0f;
    engine.setParams (p);
    engine.noteOn (69, 1.0f);

    auto buffer = render (engine, 24000);
    const float frequency = estimateFrequency (buffer.left, 48000.0);
    const float expected = 440.0f * std::pow (2.0f, 50.0f / 1200.0f);
    std::printf ("    A4 +50 ct -> %.1f Hz (expected %.1f)\n", frequency, expected);
    CHECK (std::abs (frequency - expected) < expected * 0.02f);
}

void testVintageSpread()
{
    std::printf ("- vintage voice variance\n");

    auto renderWithVintage = [] (float vintage)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.vintage = vintage;
        p.ensemble = 0.0f;
        p.space = 0.0f;
        p.spread = 0.0f;
        engine.setParams (p);
        engine.noteOn (60, 0.8f);
        engine.noteOn (64, 0.8f);
        return render (engine, 12000);
    };

    const auto clean = renderWithVintage (0.0f);
    const auto aged = renderWithVintage (1.0f);

    bool differs = false;
    for (size_t i = 0; i < clean.left.size(); ++i)
        if (std::abs (clean.left[i] - aged.left[i]) > 1.0e-4f)
            differs = true;

    CHECK (differs);
    CHECK (allFinite (aged));
}

void testFilterBrightness()
{
    std::printf ("- filter cutoff changes brightness\n");

    auto renderWithCutoff = [] (float cutoff)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.lpfCutoff = cutoff;
        p.fS = 0.5f;
        p.fAl = 0.5f;
        p.fIl = 0.5f;
        engine.setParams (p);
        engine.noteOn (57, 1.0f);
        return render (engine, 24000);
    };

    auto darkness = renderWithCutoff (200.0f);
    auto brightness = renderWithCutoff (9000.0f);

    auto roughness = [] (const StereoBuffer& buffer)
    {
        double diff = 0.0, total = 0.0;

        for (size_t i = 1; i < buffer.left.size(); ++i)
        {
            const double d = (double) buffer.left[i] - buffer.left[i - 1];
            diff += d * d;
            total += (double) buffer.left[i] * buffer.left[i];
        }

        return total > 0.0 ? diff / total : 0.0;
    };

    const double darkRoughness = roughness (darkness);
    const double brightRoughness = roughness (brightness);
    std::printf ("    roughness dark=%.6f bright=%.6f\n", darkRoughness, brightRoughness);

    CHECK (brightRoughness > darkRoughness * 1.5);
    CHECK (rmsOf (darkness) > 0.0005f);
}

void testDualChannelLayer()
{
    std::printf ("- dual channel layer and layer pan\n");

    auto renderWithMode = [] (float mode)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.layerMode = mode;
        p.layerLevel = 0.48f;
        p.layerDetune = 9.0f;
        p.layerPan = 0.7f;
        p.spread = 0.0f;
        engine.setParams (p);
        engine.noteOn (60, 0.9f);
        return render (engine, 12000);
    };

    const auto single = renderWithMode (0.0f);
    const auto dual = renderWithMode (1.0f);

    double difference = 0.0, energy = 0.0;
    for (size_t i = 0; i < single.left.size(); ++i)
    {
        const double d = (double) dual.left[i] - single.left[i];
        difference += d * d;
        energy += (double) single.left[i] * single.left[i];
    }

    double stereoDifference = 0.0;
    for (size_t i = 0; i < dual.left.size(); ++i)
    {
        const double d = (double) dual.left[i] - dual.right[i];
        stereoDifference += d * d;
    }

    std::printf ("    dual layer difference %.2f%%, stereo %.2f%%\n",
                 energy > 0.0 ? difference / energy * 100.0 : 0.0,
                 energy > 0.0 ? stereoDifference / (energy + difference) * 100.0 : 0.0);
    CHECK (difference > energy * 0.03);
    CHECK (stereoDifference > (energy + difference) * 0.005);
    CHECK (allFinite (dual));
}

void testKeyboardSplit()
{
    std::printf ("- keyboard split routes channel II above the split point\n");

    auto renderNote = [] (int note)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.layerMode = 2.0f;
        p.splitPoint = 60.0f;
        p.layerLevel = 0.8f;
        p.layerPan = 0.92f;
        p.spread = 0.0f;
        engine.setParams (p);
        engine.noteOn (note, 0.9f);
        return render (engine, 12000);
    };

    const auto low = renderNote (48);
    const auto high = renderNote (72);

    double lowStereo = 0.0, highStereo = 0.0;
    for (size_t i = 0; i < low.left.size(); ++i)
    {
        lowStereo += std::abs ((double) low.left[i] - low.right[i]);
        highStereo += std::abs ((double) high.left[i] - high.right[i]);
    }

    std::printf ("    split stereo low=%.4f high=%.4f\n", lowStereo, highStereo);
    CHECK (lowStereo < 1.0e-4);
    CHECK (highStereo > 0.01);
    CHECK (allFinite (low) && allFinite (high));
}

void testRingModulator()
{
    std::printf ("- ring modulator carrier tracks its speed\n");

    auto renderRing = [] (float speed)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.ring = 0.9f;
        p.ringSpeed = speed;
        p.lpfCutoff = 9000.0f;
        engine.setParams (p);
        engine.noteOn (57, 1.0f);
        return render (engine, 24000);
    };

    const auto slow = renderRing (40.0f);
    const auto fast = renderRing (920.0f);

    double difference = 0.0, energy = 0.0;
    for (size_t i = 0; i < slow.left.size(); ++i)
    {
        const double d = (double) slow.left[i] - fast.left[i];
        difference += d * d;
        energy += (double) slow.left[i] * slow.left[i];
    }

    CHECK (difference > energy * 0.05);
    CHECK (allFinite (slow) && allFinite (fast));
}

void testStereoAndChorus()
{
    std::printf ("- stereo spread and ensemble\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    engine.setParams (p);
    engine.noteOn (60, 0.9f);

    auto mono = render (engine, 9600);
    bool identical = true;

    for (size_t i = 0; i < mono.left.size(); ++i)
        if (mono.left[i] != mono.right[i])
            identical = false;

    CHECK (identical);

    Engine wide;
    wide.prepare (48000.0);
    Params w = defaultParams();
    w.ensemble = 0.8f;
    w.space = 0.0f;
    w.spread = 0.9f;
    wide.setParams (w);
    wide.noteOn (60, 0.9f);
    wide.noteOn (64, 0.9f);

    auto stereo = render (wide, 24000);
    double difference = 0.0, energy = 0.0;

    for (size_t i = 0; i < stereo.left.size(); ++i)
    {
        const double d = (double) stereo.left[i] - stereo.right[i];
        difference += d * d;
        energy += (double) stereo.left[i] * stereo.left[i];
    }

    CHECK (allFinite (stereo));
    CHECK (difference > energy * 0.01);
}

void testEnsembleModes()
{
    std::printf ("- ensemble modes: chorus I, chorus II, stereo tremolo\n");

    auto renderMode = [] (int mode, float amount) -> StereoBuffer
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = defaultParams();
        p.ensembleMode = (float) mode;
        p.ensemble = amount;
        p.space = 0.0f;
        p.spread = 0.0f;
        p.aA = 0.005f;
        p.aD = 4.0f;
        p.aS = 1.0f;
        engine.setParams (p);
        engine.noteOn (48, 0.9f);
        engine.noteOn (55, 0.9f);
        engine.noteOn (64, 0.9f);
        return render (engine, 72000);
    };

    const auto one = renderMode (0, 0.85f);
    const auto two = renderMode (1, 0.85f);
    const auto tremolo = renderMode (2, 0.85f);

    CHECK (allFinite (one) && allFinite (two) && allFinite (tremolo));

    // The two chorus positions must be genuinely different tap sets, not just
    // the same effect under a different label.
    double difference = 0.0, energy = 0.0;
    for (size_t i = 0; i < one.left.size(); ++i)
    {
        const double d = (double) one.left[i] - two.left[i];
        difference += d * d;
        energy += (double) one.left[i] * one.left[i];
    }

    CHECK (difference > energy * 0.005);

    // Tremolo must show clear periodic amplitude movement once the envelope
    // has settled. A 50 ms window is short against the 4.6 Hz sweep.
    std::vector<float> windows;
    constexpr int window = 2400;

    for (int start = 24000; start + window <= (int) tremolo.left.size(); start += window)
    {
        double sum = 0.0;

        for (int i = 0; i < window; ++i)
        {
            const double s = (double) tremolo.left[(size_t) (start + i)];
            sum += s * s;
        }

        windows.push_back ((float) std::sqrt (sum / (double) window));
    }

    CHECK (windows.size() > 8);

    float low = 1.0f, high = 0.0f;

    for (float value : windows)
    {
        low = std::min (low, value);
        high = std::max (high, value);
    }

    CHECK (low > 0.0f);
    CHECK (high > low * 1.35f);
}

void testReverbTail()
{
    std::printf ("- reverb tail decays\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.space = 0.8f;
    p.aR = 0.01f;
    p.masterDb = -6.0f;
    engine.setParams (p);
    engine.noteOn (60, 1.0f);

    (void) render (engine, 4800);
    engine.allNotesOff (true);

    // Let the plate's stereo pre-delay fill before measuring the decay. The
    // first few milliseconds after a hard voice kill are the reverb onset,
    // not the tail.
    (void) render (engine, 4800);

    auto tailEarly = render (engine, 4800);
    auto tailLate = render (engine, 4800);

    const float early = rmsOf (tailEarly);
    const float late = rmsOf (tailLate);
    std::printf ("    tail rms early=%.5f late=%.5f\n", early, late);
    CHECK (early > 1.0e-5f);
    CHECK (late < early * 1.2f);
}

void testSustainPedal()
{
    std::printf ("- sustain pedal\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.aR = 0.01f;
    p.space = 0.0f;
    engine.setParams (p);

    engine.setSustainPedal (true);
    engine.noteOn (60, 1.0f);
    (void) render (engine, 4800);
    engine.noteOff (60);
    (void) render (engine, 9600);
    CHECK (engine.activeVoiceCount() == 1);

    engine.setSustainPedal (false);
    (void) render (engine, 9600);
    CHECK (engine.activeVoiceCount() == 0);
}

void testDeterminism()
{
    std::printf ("- deterministic rendering\n");

    auto run = []()
    {
        Engine engine;
        engine.prepare (48000.0);
        engine.setParams (defaultParams());
        engine.noteOn (55, 0.7f);
        engine.noteOn (62, 0.7f);
        return render (engine, 8192);
    };

    const auto a = run();
    const auto b = run();
    CHECK (std::memcmp (a.left.data(), b.left.data(), a.left.size() * sizeof (float)) == 0);
    CHECK (std::memcmp (a.right.data(), b.right.data(), a.right.size() * sizeof (float)) == 0);
}

void testHighDriveAndResonance()
{
    std::printf ("- high drive and resonance stay stable\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = plainPatch();
    p.lpfRes = 1.0f;
    p.hpfRes = 1.0f;
    p.drive = 1.0f;
    p.lpfCutoff = 220.0f;
    p.ring = 0.8f;
    p.noise = 0.5f;
    engine.setParams (p);

    for (int note : { 24, 36, 48, 60, 72, 84 })
        engine.noteOn (note, 1.0f);

    auto buffer = render (engine, 48000);
    CHECK (allFinite (buffer));
    CHECK (peakOf (buffer) <= 1.0f);
    CHECK (rmsOf (buffer) > 0.001f);
}

void testExtremeSettings()
{
    std::printf ("- every parameter at both extremes\n");

    Engine engine;
    engine.prepare (48000.0);

    bool allOk = true;

    for (const auto& spec : paramSpecs())
    {
        for (float value : { spec.minValue, spec.maxValue })
        {
            Params p = defaultParams();
            setParam (p, spec.id, value);

            if (spec.id == std::string (pid::aR) || spec.id == std::string (pid::fR))
                p.aR = p.fR = 0.05f;

            engine.setParams (p);
            engine.allNotesOff (true);
            engine.noteOn (48, 1.0f);
            engine.noteOn (67, 1.0f);

            auto buffer = render (engine, 2400);
            allOk = allOk && allFinite (buffer) && peakOf (buffer) <= 1.01f;
        }
    }

    CHECK (allOk);
    CHECK (engine.outputMeter (0) >= 0.0f);
    CHECK (engine.outputMeter (1) >= 0.0f);
}

void testScopeAndRetrigger()
{
    std::printf ("- scope buffer and retriggering\n");

    Engine engine;
    engine.prepare (48000.0);
    engine.setParams (defaultParams());
    engine.noteOn (60, 0.8f);

    (void) render (engine, 9600);

    std::vector<float> scope (512, 0.0f);
    const int got = engine.readScope (scope.data(), (int) scope.size());
    CHECK (got == 512);

    bool anySignal = false;
    for (float value : scope)
        anySignal = anySignal || std::abs (value) > 1.0e-6f;

    CHECK (anySignal);

    for (int i = 0; i < 20; ++i)
    {
        engine.noteOn (60, 0.9f);
        (void) render (engine, 256);
        engine.noteOff (60);
    }

    auto buffer = render (engine, 48000 * 3);
    CHECK (allFinite (buffer));
    CHECK (peakOf (buffer) <= 1.0f);
    CHECK (engine.activeVoiceCount() == 0);
}

void testAftertouch()
{
    std::printf ("- aftertouch and mod wheel\n");

    Engine engine;
    engine.prepare (48000.0);
    Params p = defaultParams();
    p.aftertouch = 1.0f;
    p.lfoPitch = 0.5f;
    engine.setParams (p);
    engine.noteOn (60, 0.8f);

    (void) render (engine, 2400);
    engine.setChannelPressure (1.0f);
    engine.setPolyPressure (60, 1.0f);
    engine.setModWheel (1.0f);

    auto buffer = render (engine, 24000);
    CHECK (allFinite (buffer));
    CHECK (peakOf (buffer) > 0.01f);

    float variance = 0.0f;
    for (size_t i = 1; i < buffer.left.size(); ++i)
        variance += std::abs (buffer.left[i] - buffer.left[i - 1]);

    CHECK (variance > 0.0f);
}

void testBlockSizeConsistency()
{
    std::printf ("- rendering independent of the host block size\n");

    auto run = [] (int blockSize)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = defaultParams();
        p.lfoDelay = 0.0f;
        engine.setParams (p);
        engine.setModWheel (0.7f);
        engine.noteOn (48, 0.8f);
        engine.noteOn (55, 0.8f);
        engine.pitchBend (1.0f);
        return render (engine, 48000, blockSize);
    };

    const auto small = run (32);
    const auto large = run (512);

    // Short-window RMS envelopes: two renders that differ only by a phase
    // shift are equivalent, but a level or timbre difference is not.
    constexpr int window = 256;
    double difference = 0.0, energy = 0.0;

    for (int i = 0; i + window <= (int) small.left.size(); i += window)
    {
        double a = 0.0, b = 0.0;

        for (int j = 0; j < window; ++j)
        {
            const double x = small.left[(size_t) (i + j)];
            const double y = large.left[(size_t) (i + j)];
            a += x * x;
            b += y * y;
        }

        a = std::sqrt (a / window);
        b = std::sqrt (b / window);
        difference += (a - b) * (a - b);
        energy += a * a;
    }

    const double relative = std::sqrt (difference / std::max (energy, 1.0e-12));
    // A percent or so of residual is the coarse zero-order hold of a 512
    // sample host block; anything time-based rather than sample based (the
    // old behaviour) lands an order of magnitude above this.
    std::printf ("    block 32 vs 512: %.3f%% envelope difference\n", relative * 100.0);
    CHECK (relative < 0.02);
}

float maxSampleStep (const StereoBuffer& buffer, int from, int to)
{
    from = std::max (1, from);
    to = std::min (to, (int) buffer.left.size());
    float worst = 0.0f;

    for (int i = from; i < to; ++i)
    {
        worst = std::max (worst, std::abs (buffer.left[(size_t) i] - buffer.left[(size_t) (i - 1)]));
        worst = std::max (worst, std::abs (buffer.right[(size_t) i] - buffer.right[(size_t) (i - 1)]));
    }

    return worst;
}

float percentileStep (const StereoBuffer& buffer, int from, int to, float percentile)
{
    std::vector<float> deltas;
    from = std::max (1, from);
    to = std::min (to, (int) buffer.left.size());

    for (int i = from; i < to; ++i)
    {
        deltas.push_back (std::abs (buffer.left[(size_t) i] - buffer.left[(size_t) (i - 1)]));
        deltas.push_back (std::abs (buffer.right[(size_t) i] - buffer.right[(size_t) (i - 1)]));
    }

    if (deltas.empty())
        return 0.0f;

    std::sort (deltas.begin(), deltas.end());
    const size_t index = std::min (deltas.size() - 1, (size_t) ((double) deltas.size() * percentile));
    return deltas[index];
}

int liveVoiceCount (const Engine& engine)
{
    int count = 0;

    for (int i = 0; i < Engine::kNumVoices; ++i)
        if (engine.voiceIsActive (i))
            ++count;

    return count;
}

bool anyVoiceHoldsNote (const Engine& engine, int note)
{
    for (int i = 0; i < Engine::kNumVoices; ++i)
        if (engine.voiceNote (i) == note)
            return true;

    return false;
}

void testLatencyAndDecimator()
{
    std::printf ("- decimator group delay and reported latency\n");

    // The impulse response peak must land exactly on the latency the engine
    // reports, otherwise host PDC and offline bounces are misaligned.
    Decimator2x decimator;
    decimator.prepare();
    decimator.reset();

    std::vector<float> response;

    for (int n = 0; n < 32; ++n)
    {
        decimator.pushOdd (0.0f);
        response.push_back (decimator.pushEven (n == 0 ? 1.0f : 0.0f));
    }

    int peakIndex = 0;
    float peak = 0.0f;

    for (int i = 0; i < (int) response.size(); ++i)
    {
        if (std::abs (response[(size_t) i]) > peak)
        {
            peak = std::abs (response[(size_t) i]);
            peakIndex = i;
        }
    }

    // A constant input must come out at unity: the half-band kernel is
    // normalised over both phases, so the centre tap alone is 0.5.
    Decimator2x dc;
    dc.prepare();
    dc.reset();
    float dcOut = 0.0f;

    for (int n = 0; n < 64; ++n)
    {
        dc.pushOdd (1.0f);
        dcOut = dc.pushEven (1.0f);
    }

    std::printf ("    impulse peak host sample %d (amplitude %.3f), reported %d, DC gain %.5f\n",
                 peakIndex, peak, Engine::latencySamples(), dcOut);

    CHECK (Engine::latencySamples() == Decimator2x::latencyHostSamples());
    CHECK (peakIndex == Engine::latencySamples());
    CHECK_NEAR (dcOut, 1.0f, 0.01f);
    CHECK (Engine::oversamplingFactor() == 2.0);
}

void testVoiceStealingPolicyAndContinuity()
{
    std::printf ("- voice stealing policy and continuity (8 voices + 1)\n");

    Engine engine;
    engine.prepare (48000.0);

    Params p = plainPatch();
    p.lpfCutoff = 450.0f;
    p.aA = 0.02f;
    p.aD = 1.0f;
    p.aS = 0.95f;
    p.aR = 1.0f;
    engine.setParams (p);

    const int notes[8] = { 48, 50, 52, 53, 55, 57, 59, 60 };

    for (int note : notes)
        engine.noteOn (note, 0.85f);

    (void) render (engine, 128, 64);

    CHECK (engine.activeVoiceCount() == Engine::kNumVoices);
    CHECK (liveVoiceCount (engine) == Engine::kNumVoices);

    for (int i = 0; i < 8; ++i)
        CHECK (engine.voiceNote (i) == notes[i]);

    const auto settled = render (engine, 24000, 64);
    const float steady = percentileStep (settled, (int) settled.left.size() - 4096,
                                         (int) settled.left.size(), 0.999f);

    // A ninth note must steal the oldest voice, not add a ninth.
    engine.noteOn (72, 0.9f);

    CHECK (engine.activeVoiceCount() == Engine::kNumVoices);
    CHECK (engine.voiceNote (0) == 72);
    CHECK (! anyVoiceHoldsNote (engine, 48));

    const auto after = render (engine, 1024, 64);
    const float boundaryStep = std::abs (after.left[0] - settled.left.back());
    const float earlyStep = maxSampleStep (after, 0, 96);
    const float lateStep = maxSampleStep (after, 256, 1024);

    std::printf ("    steady p99.9 step %.5f, boundary %.5f, early %.5f, late %.5f\n",
                 steady, boundaryStep, earlyStep, lateStep);

    CHECK (allFinite (after));
    CHECK (boundaryStep < std::max (steady * 3.0f, 0.004f));
    CHECK (earlyStep < lateStep * 2.0f + 0.004f);

    // A note-off for the note that was stolen must not release the new note.
    engine.noteOff (48);
    (void) render (engine, 256, 64);
    CHECK (engine.activeVoiceCount() == Engine::kNumVoices);
    CHECK (engine.voiceNote (0) == 72);

    // The new note's own note-off must work.
    engine.noteOff (72);
    (void) render (engine, 48000, 256);
    CHECK (liveVoiceCount (engine) == 7);
}

void testStealDuringEveryEnvelopePhase()
{
    std::printf ("- voice stealing during attack, decay, sustain and release\n");

    struct Phase
    {
        const char* name;
        float waitSeconds;
        bool releaseFirst;
    };

    const Phase phases[] = {
        { "attack", 0.004f, false },
        { "decay", 0.05f, false },
        { "sustain", 0.60f, false },
        { "release", 0.02f, true },
    };

    for (const auto& phase : phases)
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = plainPatch();
        p.aA = 0.06f;
        p.aD = 0.35f;
        p.aS = 0.7f;
        p.aR = 1.6f;
        p.lpfCutoff = 700.0f;
        engine.setParams (p);

        for (int i = 0; i < 8; ++i)
            engine.noteOn (40 + i * 3, 0.8f);

        (void) render (engine, (int) (phase.waitSeconds * 48000.0f), 64);

        if (phase.releaseFirst)
            for (int i = 0; i < 8; ++i)
                engine.noteOff (40 + i * 3);

        const auto before = render (engine, 2048, 64);
        engine.noteOn (84, 0.9f);
        const auto during = render (engine, 512, 64);

        const float steady = percentileStep (before, 0, 2048, 0.999f);
        const float boundary = std::abs (during.left[0] - before.left.back());
        const float early = maxSampleStep (during, 0, 96);
        const float late = maxSampleStep (during, 256, 512);

        std::printf ("    %-8s steady %.5f boundary %.5f early %.5f\n",
                     phase.name, steady, boundary, early);

        CHECK (allFinite (during));
        CHECK (engine.activeVoiceCount() <= Engine::kNumVoices);
        CHECK (boundary < std::max (steady * 4.0f, 0.01f));
        CHECK (early < late * 2.5f + 0.01f);
    }
}

void testSustainPedalStealing()
{
    std::printf ("- sustain pedal + stealing does not strand voices\n");

    Engine engine;
    engine.prepare (48000.0);

    Params p = plainPatch();
    p.aR = 0.05f;
    p.aS = 0.9f;
    engine.setParams (p);

    engine.setSustainPedal (true);

    for (int i = 0; i < 8; ++i)
        engine.noteOn (45 + i, 0.8f);

    for (int i = 0; i < 8; ++i)
        engine.noteOff (45 + i);

    (void) render (engine, 4096, 64);
    CHECK (engine.activeVoiceCount() == 8);

    for (int i = 0; i < 4; ++i)
    {
        engine.noteOn (76 + i, 0.85f);
        (void) render (engine, 512, 64);
    }

    CHECK (engine.activeVoiceCount() == 8);
    CHECK (allFinite (render (engine, 2048, 64)));

    engine.setSustainPedal (false);

    // The four new keys are still down; they must release normally after the
    // pedal comes up. Nothing may be left stranded by the earlier steal.
    for (int i = 0; i < 4; ++i)
        engine.noteOff (76 + i);

    (void) render (engine, 48000 * 2, 256);

    std::printf ("    voices after pedal release: %d\n", engine.activeVoiceCount());
    CHECK (engine.activeVoiceCount() == 0);
}

void testRepeatedNotesAndRapidRetrigger()
{
    std::printf ("- repeated notes and rapid retriggering\n");

    Engine engine;
    engine.prepare (48000.0);

    Params p = plainPatch();
    p.aR = 0.03f;
    p.aA = 0.01f;
    engine.setParams (p);

    for (int i = 0; i < 24; ++i)
    {
        engine.noteOn (60, 0.85f);
        (void) render (engine, 96, 32);
        engine.noteOff (60);
        (void) render (engine, 64, 32);
    }

    CHECK (engine.activeVoiceCount() <= Engine::kNumVoices);
    CHECK (allFinite (render (engine, 48000, 256)));
    CHECK (engine.activeVoiceCount() == 0);

    // Every one of 500 retriggers must leave exactly one voice assigned to the
    // note, and no voice may be left hanging after the final note-off.
    for (int i = 0; i < 500; ++i)
    {
        engine.noteOn (67, 0.7f);
        (void) render (engine, 24, 24);
        engine.noteOff (67);
    }

    (void) render (engine, 48000, 256);
    CHECK (engine.activeVoiceCount() == 0);
}

void testStealWithDualLayersDriveAndPressure()
{
    std::printf ("- stealing under dual layers, drive, resonance and aftertouch\n");

    Engine engine;
    engine.prepare (48000.0);

    Params p = defaultParams();
    p.layerMode = 1.0f;
    p.layerLevel = 0.55f;
    p.layerDetune = 12.0f;
    p.lpfRes = 1.0f;
    p.hpfRes = 0.9f;
    p.drive = 1.0f;
    p.lpfCutoff = 380.0f;
    p.aA = 0.01f;
    p.aS = 0.9f;
    p.aR = 0.8f;
    engine.setParams (p);

    for (int i = 0; i < 8; ++i)
        engine.noteOn (36 + i * 4, 0.9f);

    (void) render (engine, 9600, 64);

    engine.setPolyPressure (36, 1.0f);
    engine.setChannelPressure (0.9f);
    engine.setModWheel (1.0f);

    const auto before = render (engine, 2048, 64);
    engine.noteOn (79, 1.0f);
    const auto during = render (engine, 4096, 64);

    const float steady = percentileStep (before, 0, 2048, 0.999f);
    const float boundary = std::abs (during.left[0] - before.left.back());

    std::printf ("    dual/drive steady %.5f, boundary %.5f\n", steady, boundary);

    CHECK (allFinite (during));
    CHECK (peakOf (during) <= 1.0f);
    CHECK (boundary < std::max (steady * 5.0f, 0.02f));

    // Poly pressure addressed to the stolen note must not revive it.
    engine.setPolyPressure (36, 1.0f);
    engine.noteOff (36);
    (void) render (engine, 1024, 64);
    CHECK (! anyVoiceHoldsNote (engine, 36));
    CHECK (engine.activeVoiceCount() == Engine::kNumVoices);
}

void testEffectBypassIsSmooth()
{
    std::printf ("- chorus and reverb bypass are smooth and leave no stale tail\n");

    // Chorus: enable while a note is sounding, disable, let everything go
    // silent, then re-enable. A frozen delay line would burst stale audio here.
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = defaultParams();
        p.ensemble = 0.8f;
        p.space = 0.0f;
        p.aR = 0.01f;
        engine.setParams (p);
        engine.noteOn (60, 0.9f);

        (void) render (engine, 24000, 128);
        p.ensemble = 0.0f;
        engine.setParams (p);
        (void) render (engine, 48000, 128);
        engine.allNotesOff (true);
        (void) render (engine, 96000, 256);

        p.ensemble = 0.8f;
        engine.setParams (p);
        const auto burst = render (engine, 4800, 128);

        std::printf ("    chorus re-enable peak %.6f, max step %.6f\n",
                     peakOf (burst), maxSampleStep (burst, 1, 4800));

        CHECK (peakOf (burst) < 0.002f);
        CHECK (maxSampleStep (burst, 1, 4800) < 0.002f);
    }

    // Reverb: a full tail, then a silent buffer, then mix back up.
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = defaultParams();
        p.space = 0.9f;
        p.ensemble = 0.0f;
        p.aR = 0.01f;
        engine.setParams (p);
        engine.noteOn (60, 0.9f);

        (void) render (engine, 24000, 128);
        p.space = 0.0f;
        engine.setParams (p);
        (void) render (engine, 48000, 128);
        engine.allNotesOff (true);
        (void) render (engine, 48000 * 3, 256);

        p.space = 0.9f;
        engine.setParams (p);
        const auto burst = render (engine, 4800, 128);

        std::printf ("    reverb re-enable peak %.6f, max step %.6f\n",
                     peakOf (burst), maxSampleStep (burst, 1, 4800));

        CHECK (peakOf (burst) < 0.002f);
        CHECK (maxSampleStep (burst, 1, 4800) < 0.002f);
    }

    // Toggling the ensemble on while a chord sounds must not click.
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = defaultParams();
        p.ensemble = 0.0f;
        p.space = 0.0f;
        engine.setParams (p);

        for (int note : { 48, 55, 60, 64 })
            engine.noteOn (note, 0.8f);

        const auto before = render (engine, 24000, 128);
        p.ensemble = 0.9f;
        engine.setParams (p);
        const auto during = render (engine, 4800, 128);

        const float steady = percentileStep (before, 12000, 24000, 0.999f);
        const float edge = maxSampleStep (during, 0, 256);
        std::printf ("    chorus toggled live: steady %.5f, edge %.5f\n", steady, edge);

        CHECK (edge < steady * 4.0f + 0.01f);
        CHECK (allFinite (during));
    }
}

void testRealtimeAllocations()
{
    std::printf ("- audio callback performs zero heap allocations\n");

    Engine engine;
    engine.prepare (48000.0);
    engine.prepare (44100.0); // sample-rate change must also be allocation free
    engine.setParams (defaultParams());

    const int blockSize = 128;
    std::vector<float> left ((size_t) blockSize, 0.0f), right ((size_t) blockSize, 0.0f);

    engine.noteOn (60, 0.8f);
    (void) engine.process (left.data(), right.data(), blockSize);

    gAllocationCount.store (0);
    gTrackAllocations.store (true);

    for (int block = 0; block < 400; ++block)
    {
        engine.process (left.data(), right.data(), blockSize);

        if (block == 20)  engine.noteOn (67, 0.9f);
        if (block == 40)  engine.setPolyPressure (67, 0.7f);
        if (block == 60)  engine.setChannelPressure (0.5f);
        if (block == 80)  engine.setModWheel (0.9f);
        if (block == 100) engine.pitchBend (0.4f);
        if (block == 120) engine.setSustainPedal (true);
        if (block == 140) engine.noteOff (60);
        if (block == 160) engine.setSustainPedal (false);
        if (block == 180) engine.noteOn (72, 0.7f);
        if (block == 200) engine.setParams (defaultParams());
        if (block == 220) engine.noteOff (72);
        if (block == 240) engine.setParams (plainPatch());
        if (block == 300) engine.allNotesOff (false);
        if (block == 360) engine.allNotesOff (true);
    }

    gTrackAllocations.store (false);
    const long allocations = gAllocationCount.load();

    std::printf ("    allocations during 400 callbacks: %ld\n", allocations);
    CHECK (allocations == 0);
}

void testAliasingTargets()
{
    std::printf ("- aliasing stays below the documented thresholds\n");

    struct Case
    {
        const char* name;
        int note;
        float thresholdDb;
        int setup;
    };

    // setup: 0 clean saw, 1 narrow pulse, 2 max brightness, 3 very low saw
    const Case cases[] = {
        { "clean saw A2", 45, -80.0f, 0 },
        { "clean saw C6", 84, -65.0f, 0 },
        { "narrow pulse C4", 60, -65.0f, 1 },
        { "max brightness C5", 72, -60.0f, 2 },
        { "sub saw C1", 24, -80.0f, 3 },
    };

    for (const auto& testCase : cases)
    {
        Engine engine;
        engine.prepare (48000.0);

        Params p = plainPatch();
        p.lpfCutoff = 9000.0f;
        p.drive = 0.15f;
        p.masterDb = -12.0f;
        p.vco1Detune = 0.0f;
        p.vco2Detune = 0.0f;

        if (testCase.setup == 1)
        {
            p.vco1Level = 0.0f;
            p.vco2Level = 1.0f;
            p.vco2Pw = 0.05f;
        }
        else if (testCase.setup == 2)
        {
            p.drive = 1.0f;
            p.vco2Level = 1.0f;
            p.lpfCutoff = 16000.0f;
            p.brilliance = 1.0f;
        }

        engine.setParams (p);
        engine.noteOn (testCase.note, 0.9f);

        const int total = 48000 * 2;
        const auto buffer = render (engine, total, 256);

        std::vector<float> window (buffer.left.end() - 65536, buffer.left.end());
        const double f0 = 440.0 * std::pow (2.0, ((double) testCase.note - 69.0) / 12.0);
        const auto report = cs80test::analyseAliasing (window, 48000.0, f0);

        std::printf ("    %-18s worst alias %7.1f dB, energy %7.1f dB\n",
                     testCase.name, report.worstAliasDb, report.aliasEnergyDb);

        CHECK (report.worstAliasDb < (double) testCase.thresholdDb);
        CHECK (allFinite (buffer));
    }
}

void testFilterUnitResponse()
{
    std::printf ("- state-variable filter response, resonance and stability\n");

    constexpr double sampleRate = 48000.0;

    auto gainAt = [] (float cutoff, float q, double frequency)
    {
        SVF filter;
        filter.reset();
        filter.setCoefficients (sampleRate, cutoff, q);

        double sum = 0.0;
        constexpr int settle = 24000;
        constexpr int measure = 24000;

        for (int i = 0; i < settle + measure; ++i)
        {
            const float input = (float) std::sin (2.0 * cs80test::kPi * frequency
                                                  * (double) i / sampleRate);
            const float out = filter.lowpass (input, 0.0f);

            if (i >= settle)
                sum += (double) out * out;
        }

        return std::sqrt (sum / measure) * std::sqrt (2.0);
    };

    const float atCutoff = (float) gainAt (1000.0f, 0.707f, 1000.0);
    const float belowCutoff = (float) gainAt (1000.0f, 0.707f, 100.0);
    const float aboveCutoff = (float) gainAt (1000.0f, 0.707f, 8000.0);
    const float resonant = (float) gainAt (1000.0f, 14.0f, 1000.0);

    std::printf ("    gain below %.3f, at cutoff %.3f, above %.4f, resonant %.2f\n",
                 belowCutoff, atCutoff, aboveCutoff, resonant);

    CHECK (std::abs (belowCutoff - 1.0f) < 0.05f);
    CHECK (std::abs (atCutoff - 0.707f) < 0.06f);
    CHECK (aboveCutoff < 0.05f);
    CHECK (resonant > 6.0f);

    // Every supported rate, extreme cutoff and high Q must stay finite and
    // bounded; a resonant filter that explodes would poison the whole voice.
    bool stable = true;

    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
    {
        SVF filter;
        filter.reset();
        filter.setCoefficients (rate, 20.0f, 40.0f);

        float peak = 0.0f;

        for (int i = 0; i < 48000; ++i)
        {
            const float cutoff = 20.0f * std::exp2 (10.0f * (float) (i % 4800) / 4800.0f);
            filter.setCoefficients (rate, cutoff, 40.0f);
            const float out = filter.lowpass (i == 0 ? 1.0f : 0.0f, 3.0f);
            peak = std::max (peak, std::abs (out));

            if (! std::isfinite (out))
                stable = false;
        }

        if (peak > 8.0f)
            stable = false;
    }

    CHECK (stable);
}

void testEnvelopeTiming()
{
    std::printf ("- envelope segment timing\n");

    Envelope envelope;
    envelope.prepare (48000.0);
    envelope.setTimes (0.10f, 0.20f, 0.30f);
    envelope.setLevels (1.0f, 0.5f);
    envelope.trigger (0.0f);

    int attackSamples = 0;

    while (envelope.getValue() < 0.999f && attackSamples < 48000)
    {
        envelope.process();
        ++attackSamples;
    }

    int decaySamples = 0;

    while (envelope.getValue() > 0.501f && decaySamples < 48000)
    {
        envelope.process();
        ++decaySamples;
    }

    envelope.release();
    int releaseSamples = 0;

    while (envelope.isActive() && releaseSamples < 48000 * 2)
    {
        envelope.process();
        ++releaseSamples;
    }

    const float attackMs = (float) attackSamples / 48.0f;
    const float decayMs = (float) decaySamples / 48.0f;
    const float releaseMs = (float) releaseSamples / 48.0f;

    std::printf ("    attack %.1f ms, decay %.1f ms, release %.1f ms\n",
                 attackMs, decayMs, releaseMs);

    CHECK (std::abs (attackMs - 100.0f) < 4.0f);
    CHECK (std::abs (decayMs - 200.0f) < 8.0f);
    CHECK (std::abs (releaseMs - 300.0f) < 12.0f);
    CHECK (! envelope.isActive());
}

void testSampleRateChanges()
{
    std::printf ("- repeated prepare and sample-rate changes\n");

    Engine engine;
    bool finite = true;
    bool tuned = true;

    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0, 48000.0 })
    {
        engine.prepare (rate);
        engine.setParams (plainPatch());
        engine.noteOn (69, 1.0f);

        const auto buffer = render (engine, (int) (rate * 0.4), 128);
        finite = finite && allFinite (buffer);

        const float measured = estimateFrequency (buffer.left, rate);
        std::printf ("    %.0f Hz -> %.1f Hz\n", rate, measured);
        tuned = tuned && std::abs (measured - 440.0f) < 440.0f * 0.03f;

        engine.allNotesOff (true);
        (void) render (engine, 512, 128);
    }

    CHECK (finite);
    CHECK (tuned);
    CHECK (engine.sampleRate() == 48000.0);
}

void testVelocityOpensFilter()
{
    std::printf ("- velocity opens the filter\n");

    auto brightness = [] (float velocity)
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = plainPatch();
        p.velocity = 1.0f;
        p.lpfCutoff = 1200.0f;
        p.fIl = p.fAl = p.fS = 0.5f;
        engine.setParams (p);
        engine.noteOn (60, velocity);
        const auto buffer = render (engine, 24000);

        double high = 0.0, total = 0.0;
        float lowpass = 0.0f;

        for (float value : buffer.left)
        {
            lowpass += (value - lowpass) * 0.12f;
            const double highPart = (double) value - lowpass;
            high += highPart * highPart;
            total += (double) value * value;
        }

        return (float) std::sqrt (high / std::max (total, 1.0e-12));
    };

    const float soft = brightness (0.2f);
    const float hard = brightness (1.0f);
    std::printf ("    brightness soft=%.3f hard=%.3f\n", soft, hard);
    CHECK (hard > soft * 1.05f);
}

void testMicrotuning()
{
    std::printf ("- microtuning (Scala scl/kbm)\n");

    Tuning equal;
    CHECK (equal.isEqualTemperament());
    CHECK_NEAR (equal.noteFrequency (69), 440.0f, 0.001f);
    CHECK_NEAR (equal.noteFrequency (60), 261.6256f, 0.02f);
    CHECK_NEAR (equal.frequencyForNote (60.5),
                440.0 * std::exp2 (-8.5 / 12.0), 0.02f);

    // A five-note scale: C, D, E, G, A and the octave.
    Tuning pentatonic;
    std::string error = "not set";
    const std::string scale = "! test scale\nMajor pentatonic\n5\n200.0\n400.0\n700.0\n900.0\n2/1\n";
    CHECK (pentatonic.loadScala (scale, &error));
    CHECK (error.empty());
    CHECK (! pentatonic.isEqualTemperament());
    CHECK (pentatonic.scaleNoteCount() == 5);
    CHECK_NEAR (pentatonic.noteFrequency (60), 261.6256f, 0.05f);
    CHECK_NEAR (pentatonic.noteFrequency (61),
                261.6256 * std::exp2 (200.0 / 1200.0), 0.05f);
    CHECK_NEAR (pentatonic.noteFrequency (62),
                261.6256 * std::exp2 (400.0 / 1200.0), 0.05f);
    CHECK_NEAR (pentatonic.noteFrequency (65), 261.6256 * 2.0, 0.05f);

    // The standard Scala format gives ABSOLUTE pitches, not consecutive
    // intervals. The bare integer 2 represents the ratio 2/1, not 2 cents.
    Tuning integerRatio;
    CHECK (integerRatio.loadScala ("One octave\n1\n2\n", &error));
    CHECK_NEAR (integerRatio.noteFrequency (61), 261.6256 * 2.0, 0.05f);
    CHECK (integerRatio.name() == "One octave");

    Tuning descending;
    CHECK (descending.loadScala ("Negative note\n2\n-5.0\n2/1\n", &error));
    CHECK (descending.noteFrequency (61) < descending.noteFrequency (60));

    Tuning badRatio;
    CHECK (! badRatio.loadScala ("bad\n1\n2/1/1\n", &error));
    CHECK (badRatio.isEqualTemperament());
    CHECK (! badRatio.loadScala ("Overflowing period\n1\n100000.0\n", &error));
    CHECK (badRatio.isEqualTemperament());

    // Malformed input is rejected and leaves the previous tuning untouched.
    Tuning invalid;
    error.clear();
    CHECK (! invalid.loadScala ("not a scale\nbanana\n", &error));
    CHECK (! error.empty());
    CHECK (invalid.isEqualTemperament());
    CHECK_NEAR (invalid.noteFrequency (69), 440.0f, 0.001f);

    // A .kbm mapping can move the reference pitch without touching the scale.
    Tuning baroque;
    const std::string mapping = "! mapping\n12\n0\n127\n60\n69\n432.0\n12\n"
                                "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n";
    error.clear();
    CHECK (baroque.loadKeyboardMapping (mapping, &error));
    CHECK (error.empty());
    CHECK_NEAR (baroque.noteFrequency (69), 432.0f, 0.01f);
    CHECK_NEAR (baroque.noteFrequency (57), 216.0f, 0.01f);

    // .kbm reference frequencies may contain fractional Hz, and the first
    // mapping entry belongs to the declared MIDDLE note, not the first note.
    Tuning shifted;
    const std::string shiftedMapping = "5\n0\n127\n61\n61\n432.5\n12\n"
                                       "0\n2\n4\n6\n8\n";
    CHECK (shifted.loadKeyboardMapping (shiftedMapping, &error));
    CHECK_NEAR (shifted.noteFrequency (61), 432.5f, 0.01f);
    CHECK_NEAR (shifted.noteFrequency (62), 432.5 * std::exp2 (2.0 / 12.0), 0.02f);
    CHECK_NEAR (shifted.noteFrequency (66), 865.0f, 0.02f);

    Tuning unmapped;
    const std::string unmappedMapping = "3\n60\n72\n60\n60\n440.0\n3\n0\nx\n2\n";
    CHECK (unmapped.loadKeyboardMapping (unmappedMapping, &error));
    CHECK_NEAR (unmapped.noteFrequency (60), 440.0f, 0.01f);
    CHECK (unmapped.noteFrequency (61) == 0.0);
    CHECK_NEAR (unmapped.noteFrequency (59), 246.9417f, 0.02f);

    Engine muted;
    muted.prepare (48000.0);
    muted.setTuning (unmapped);
    muted.noteOn (61, 1.0f);
    CHECK (peakOf (render (muted, 2048)) < 0.000001f);

    CHECK (! unmapped.loadKeyboardMapping ("3\n60\n72\n60\n61\n440.0\n3\n0\nx\n2\n", &error));
    CHECK (unmapped.noteFrequency (61) == 0.0);
    CHECK (! unmapped.loadKeyboardMapping ("3extra\n60\n72\n60\n60\n440.0\n3\n0\nx\n2\n", &error));
    CHECK (! unmapped.loadKeyboardMapping ("1\n0\n127\n60\n60\n1e308\n1\n0\n", &error));
    CHECK (unmapped.noteFrequency (61) == 0.0);
    CHECK (std::isfinite (unmapped.frequencyForNote (61.5)));

    // The engine must actually use the table: an A3 should render at 216 Hz.
    Params p = plainPatch();
    p.sineLevel = 1.0f;
    p.vco1Level = 0.0f;
    p.vco2Level = 0.0f;
    p.lpfCutoff = 8000.0f;

    Engine engine;
    engine.prepare (48000.0);
    engine.setParams (p);
    engine.setTuning (baroque);
    engine.noteOn (57, 1.0f);

    // A full second, measured after the attack: zero-crossing counting is only
    // accurate to one cycle per window, so the window must be long enough.
    auto buffer = render (engine, 60000);
    std::vector<float> tail (buffer.left.begin() + 12000, buffer.left.end());
    const float frequency = estimateFrequency (tail, 48000.0);
    CHECK (allFinite (buffer));
    CHECK_NEAR (frequency, 216.0f, 1.5f);
}

void testConcurrentTuningPublication()
{
    std::printf ("- concurrent tuning publication\n");

    Engine engine;
    engine.prepare (48000.0);
    engine.setParams (plainPatch());
    engine.noteOn (69, 0.75f);

    Tuning concert, alternate;
    alternate.setEqualTemperament (432.5);
    std::atomic<bool> start { false };

    std::thread publisher ([&]
    {
        while (! start.load())
            std::this_thread::yield();

        for (int i = 0; i < 3000; ++i)
            engine.setTuning ((i & 1) != 0 ? concert : alternate);
    });

    start.store (true);
    std::array<float, 128> left {}, right {};
    bool finite = true;
    for (int i = 0; i < 3000; ++i)
    {
        engine.process (left.data(), right.data(), (int) left.size());
        for (float sample : left)
            finite = finite && std::isfinite (sample);
        for (float sample : right)
            finite = finite && std::isfinite (sample);
    }

    publisher.join();
    CHECK (finite);
    CHECK_NEAR (engine.tuning().noteFrequency (69), 440.0f, 0.01f);
}

void testModulationMatrix()
{
    std::printf ("- modulation matrix\n");

    Params routed = plainPatch();

    for (int slot = 0; slot < pid::kModSlots; ++slot)
    {
        const std::string sourceId = pid::modSourceIds[slot];
        const std::string destId = pid::modDestIds[slot];
        const std::string amountId = pid::modAmountIds[slot];

        setParam (routed, sourceId, 3.0f);
        setParam (routed, destId, 4.0f);
        setParam (routed, amountId, -0.6f);

        CHECK_NEAR (getParam (routed, sourceId), 3.0f, 0.0001f);
        CHECK_NEAR (getParam (routed, destId), 4.0f, 0.0001f);
        CHECK_NEAR (getParam (routed, amountId), -0.6f, 0.0001f);

        setParam (routed, amountId, 5.0f);
        CHECK_NEAR (getParam (routed, amountId), 1.0f, 0.0001f);

        setParam (routed, sourceId, 0.0f);
        setParam (routed, destId, 0.0f);
        setParam (routed, amountId, 0.0f);
    }

    auto renderNote = [] (int note, int source, int dest, float amount, float velocity)
    {
        Params p = plainPatch();
        p.sineLevel = 1.0f;
        p.vco1Level = 0.0f;
        p.vco2Level = 0.0f;
        p.lpfCutoff = 8000.0f;
        p.modSource[0] = (float) source;
        p.modDest[0] = (float) dest;
        p.modAmount[0] = amount;

        Engine engine;
        engine.prepare (48000.0);
        engine.setParams (p);
        engine.noteOn (note, velocity);
        return render (engine, 60000);
    };

    auto tailFrequency = [] (const StereoBuffer& buffer)
    {
        std::vector<float> tail (buffer.left.begin() + 12000, buffer.left.end());
        return estimateFrequency (tail, 48000.0);
    };

    const auto reference = renderNote (57, modsrc::off, moddest::off, 0.0f, 1.0f);
    const float referenceFrequency = tailFrequency (reference);
    CHECK_NEAR (referenceFrequency, 220.0f, 1.5f);

    // Velocity -> pitch, full velocity, +0.5 depth = exactly one octave.
    const auto pitchRouted = renderNote (57, modsrc::velocity, moddest::pitch, 0.5f, 1.0f);
    CHECK_NEAR (tailFrequency (pitchRouted), referenceFrequency * 2.0f, 3.0f);

    // Velocity -> amplitude at -1.0 must duck the voice hard.
    const auto ampRouted = renderNote (57, modsrc::velocity, moddest::amplitude, -1.0f, 1.0f);
    CHECK (rmsOf (ampRouted, 12000, 24000) < rmsOf (reference, 12000, 24000) * 0.35f);

    // Velocity -> pan at +1.0 pushes the voice to the right edge.
    const auto panRouted = renderNote (57, modsrc::velocity, moddest::pan, 1.0f, 1.0f);

    {
        double leftSum = 0.0, rightSum = 0.0;
        for (int i = 20000; i < 24000; ++i)
        {
            leftSum += (double) panRouted.left[(size_t) i] * panRouted.left[(size_t) i];
            rightSum += (double) panRouted.right[(size_t) i] * panRouted.right[(size_t) i];
        }

        CHECK (std::sqrt (leftSum / 4000.0) < 0.005);
        CHECK (std::sqrt (rightSum / 4000.0) > 0.02);
    }

    // Velocity -> cutoff: a 110 Hz saw through a 300 Hz low-pass should gain
    // substantial high-frequency energy when the route opens it four octaves.
    {
        auto renderSaw = [] (int source, int dest, float amount)
        {
            Params p = plainPatch();
            p.vco1Level = 1.0f;
            p.sineLevel = 0.0f;
            p.lpfCutoff = 300.0f;
            p.lpfRes = 0.2f;
            p.filterEnvAmt = 0.0f;
            p.fIl = 0.1f;
            p.fAl = 0.1f;
            p.fS = 0.1f;
            p.modSource[0] = (float) source;
            p.modDest[0] = (float) dest;
            p.modAmount[0] = amount;

            Engine engine;
            engine.prepare (48000.0);
            engine.setParams (p);
            engine.noteOn (45, 1.0f);
            return render (engine, 9600);
        };

        auto differenceEnergy = [] (const StereoBuffer& buffer)
        {
            double sum = 0.0;

            for (int i = 4801; i < 9600; ++i)
            {
                const double difference = (double) buffer.left[(size_t) i]
                                        - (double) buffer.left[(size_t) i - 1];
                sum += difference * difference;
            }

            return std::sqrt (sum / 4799.0);
        };

        const auto dull = renderSaw (modsrc::off, moddest::off, 0.0f);
        const auto bright = renderSaw (modsrc::velocity, moddest::cutoff, 1.0f);
        CHECK (differenceEnergy (bright) > differenceEnergy (dull) * 3.0);
    }

    // Random -> resonance at full depth and maximum Q must stay bounded.
    {
        const auto resonant = renderNote (57, modsrc::random, moddest::resonance, 1.0f, 1.0f);
        CHECK (allFinite (resonant));
        CHECK (peakOf (resonant) < 1.01f);
    }

    // An LFO-routed pitch sweep must stay continuous and actually move.
    {
        Params p = plainPatch();
        p.sineLevel = 1.0f;
        p.vco1Level = 0.0f;
        p.vco2Level = 0.0f;
        p.lfoRate = 5.0f;
        p.lfoDelay = 0.0f;
        p.modSource[0] = (float) modsrc::lfo;
        p.modDest[0] = (float) moddest::pitch;
        p.modAmount[0] = 0.25f;

        Engine engine;
        engine.prepare (48000.0);
        engine.setParams (p);
        engine.noteOn (57, 1.0f);

        const auto swept = render (engine, 48000);
        float maxStep = 0.0f;

        for (int i = 1; i < (int) swept.left.size(); ++i)
            maxStep = std::max (maxStep, std::abs (swept.left[(size_t) i]
                                                - swept.left[(size_t) i - 1]));

        CHECK (allFinite (swept));
        CHECK (maxStep < 0.20f);
        CHECK (peakOf (swept) > 0.01f);
    }
}

void testPresetsRender()
{
    std::printf ("- every preset renders\n");

    bool ok = true;

    for (const auto& preset : factoryPresets())
    {
        Engine engine;
        engine.prepare (48000.0);
        Params p = defaultParams();
        applyPreset (p, preset);
        engine.setParams (p);

        for (int note : { 48, 55, 60, 64, 67 })
            engine.noteOn (note, 0.75f);

        auto buffer = render (engine, 24000);
        const float peak = peakOf (buffer);

        std::printf ("    %-18s peak=%.3f\n", preset.name, peak);

        if (! allFinite (buffer) || peak <= 0.0005f || peak > 1.0f)
            ok = false;
    }

    CHECK (ok);
}
} // namespace

int main()
{
    std::printf ("CS-80E engine tests\n");
    std::printf ("===================\n");

    testParameterTable();
    testPresets();
    testBasicRenderAndSilence();
    testPolyphonyAndStealing();
    testTuning();
    testMicrotuning();
    testConcurrentTuningPublication();
    testModulationMatrix();
    testOctaveSwitch();
    testSubOscillator();
    testGlide();
    testMasterTune();
    testVintageSpread();
    testFilterBrightness();
    testDualChannelLayer();
    testKeyboardSplit();
    testRingModulator();
    testStereoAndChorus();
    testEnsembleModes();
    testReverbTail();
    testSustainPedal();
    testDeterminism();
    testHighDriveAndResonance();
    testExtremeSettings();
    testScopeAndRetrigger();
    testAftertouch();
    testBlockSizeConsistency();
    testLatencyAndDecimator();
    testVoiceStealingPolicyAndContinuity();
    testStealDuringEveryEnvelopePhase();
    testSustainPedalStealing();
    testRepeatedNotesAndRapidRetrigger();
    testStealWithDualLayersDriveAndPressure();
    testEffectBypassIsSmooth();
    testRealtimeAllocations();
    testAliasingTargets();
    testFilterUnitResponse();
    testEnvelopeTiming();
    testSampleRateChanges();
    testVelocityOpensFilter();
    testPresetsRender();

    std::printf ("===================\n");
    std::printf ("%d checks, %d failure%s\n", gChecks, gFailures, gFailures == 1 ? "" : "s");

    return gFailures == 0 ? 0 : 1;
}
