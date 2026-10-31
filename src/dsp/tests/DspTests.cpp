// Headless test suite for the CS-80 engine. No JUCE, no audio device, no
// display: this is the fast feedback loop that also runs on every CI platform.

#include "../Engine.h"
#include "../Presets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace cs80;

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
    p.ensemble = 0.0f;
    p.space = 0.0f;
    p.spread = 0.0f;
    p.ring = 0.0f;
    p.noise = 0.0f;
    p.vco2Level = 0.0f;
    p.vco1Level = 1.0f;
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
    CHECK (paramSpecs().size() >= 30);

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
        const float mid = (spec.minValue + spec.maxValue) * 0.5f;
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
    CHECK (presets.size() >= 8);

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
    (void) render (engine, 4800); // let the DC blocker settle after the hard kill
    auto after = render (engine, 4800);
    CHECK (engine.activeVoiceCount() == 0);
    CHECK (peakOf (after) < 1.0e-4f);
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
    testOctaveSwitch();
    testFilterBrightness();
    testStereoAndChorus();
    testReverbTail();
    testSustainPedal();
    testDeterminism();
    testExtremeSettings();
    testScopeAndRetrigger();
    testAftertouch();
    testPresetsRender();

    std::printf ("===================\n");
    std::printf ("%d checks, %d failure%s\n", gChecks, gFailures, gFailures == 1 ? "" : "s");

    return gFailures == 0 ? 0 : 1;
}
