// Offline analysis tool: reproducible CPU benchmarks and spectral (aliasing)
// measurements for the CS-80E engine. No JUCE, no audio device.
//
//   cs80_analysis bench
//   cs80_analysis alias
//   cs80_analysis latency

#include "../../src/dsp/Dsp.h"
#include "../../src/dsp/Engine.h"
#include "../../src/dsp/Presets.h"
#include "../../src/dsp/tests/Spectrum.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <thread>
#include <vector>

using namespace cs80;

//==============================================================================
// Allocation counter (measurement only).
namespace
{
std::atomic<unsigned long long> gAllocations { 0 };
std::atomic<bool> gCountAllocations { false };
} // namespace

void* operator new (std::size_t size)
{
    if (gCountAllocations.load (std::memory_order_relaxed))
        gAllocations.fetch_add (1, std::memory_order_relaxed);

    if (size == 0)
        size = 1;

    if (auto* p = std::malloc (size))
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
//==============================================================================
struct BenchCase
{
    const char* name;
    int voices;
    bool dualAndEffects;
    bool heavyModulation;
    bool extremeDrive;
};

struct BenchRow
{
    std::string scenario;
    double sampleRate = 0.0;
    int blockSize = 0;
    double averageUs = 0.0;
    double p95Us = 0.0;
    double worstUs = 0.0;
    double realtimeRatio = 0.0; // audio seconds / processing seconds
    double cpuPercent = 0.0;
    unsigned long long allocations = 0;
};

Params benchPatch (const BenchCase& benchCase)
{
    Params p = defaultParams();
    p.layerMode = benchCase.dualAndEffects ? 1.0f : 0.0f;
    p.layerLevel = benchCase.dualAndEffects ? 0.45f : 0.0f;
    p.ensemble = benchCase.dualAndEffects ? 0.55f : 0.0f;
    p.space = benchCase.dualAndEffects ? 0.40f : 0.0f;
    p.spread = 0.7f;
    p.vintage = 0.35f;

    if (benchCase.heavyModulation)
    {
        p.lfoRate = 7.5f;
        p.lfoPitch = 1.0f;
        p.lfoPwm = 1.0f;
        p.lfoFilter = 1.0f;
        p.lfoAmp = 1.0f;
        p.lfoDelay = 0.0f;
        p.aftertouch = 0.8f;
        p.ring = 0.6f;
        p.ringSpeed = 820.0f;
    }

    if (benchCase.extremeDrive)
    {
        p.lpfRes = 1.0f;
        p.hpfRes = 1.0f;
        p.drive = 1.0f;
        p.lpfCutoff = 900.0f;
        p.ring = 0.5f;
    }

    return p;
}

BenchRow runBench (const BenchCase& benchCase, double sampleRate, int blockSize,
                   double seconds)
{
    Engine engine;
    engine.prepare (sampleRate);
    engine.setParams (benchPatch (benchCase));

    static const int notes[8] = { 36, 43, 48, 52, 55, 60, 64, 67 };

    for (int i = 0; i < benchCase.voices; ++i)
        engine.noteOn (notes[i], 0.8f);

    std::vector<float> left ((size_t) blockSize), right ((size_t) blockSize);
    const auto totalBlocks = (int) std::lround (seconds * sampleRate / (double) blockSize);

    // Warm up, including any lazy coefficient / table setup.
    for (int i = 0; i < 64; ++i)
        engine.process (left.data(), right.data(), blockSize);

    std::vector<double> times;
    times.reserve ((size_t) totalBlocks);

    const auto wallStart = std::chrono::steady_clock::now();
    gAllocations.store (0);
    gCountAllocations.store (true);

    for (int block = 0; block < totalBlocks; ++block)
    {
        const auto start = std::chrono::steady_clock::now();
        engine.process (left.data(), right.data(), blockSize);
        const auto end = std::chrono::steady_clock::now();

        times.push_back (std::chrono::duration<double, std::micro> (end - start).count());
    }

    gCountAllocations.store (false);
    const auto wallEnd = std::chrono::steady_clock::now();

    const double wallSeconds = std::chrono::duration<double> (wallEnd - wallStart).count();
    std::sort (times.begin(), times.end());

    BenchRow row;
    row.scenario = benchCase.name;
    row.sampleRate = sampleRate;
    row.blockSize = blockSize;

    double sum = 0.0;
    for (double t : times)
        sum += t;

    row.averageUs = sum / (double) times.size();
    row.p95Us = times[(size_t) std::min<size_t> (times.size() - 1, (size_t) (times.size() * 0.95))];
    row.worstUs = times.back();
    row.realtimeRatio = wallSeconds > 0.0 ? seconds / wallSeconds : 0.0;
    row.cpuPercent = wallSeconds > 0.0 ? 100.0 * wallSeconds / seconds : 0.0;
    row.allocations = gAllocations.load();

    return row;
}

void printBenchHeader()
{
    std::printf ("| Scenario | SR | Block | Avg us | p95 us | Worst us | x realtime | CPU %% | Allocs |\n");
    std::printf ("| --- | --- | --- | --- | --- | --- | --- | --- | --- |\n");
}

void printBenchRow (const BenchRow& row)
{
    std::printf ("| %s | %.0f | %d | %.1f | %.1f | %.1f | %.2fx | %.2f | %llu |\n",
                 row.scenario.c_str(), row.sampleRate, row.blockSize,
                 row.averageUs, row.p95Us, row.worstUs,
                 row.realtimeRatio, row.cpuPercent, row.allocations);
}

int runBenchSuite()
{
    const BenchCase cases[] = {
        { "idle", 0, false, false, false },
        { "1 voice", 1, false, false, false },
        { "4 voices", 4, false, false, false },
        { "8 voices", 8, false, false, false },
        { "8 voices, dual + chorus + reverb", 8, true, false, false },
        { "8 voices, heavy modulation", 8, true, true, false },
        { "8 voices, high resonance + drive", 8, false, false, true },
    };

    std::printf ("CS-80E CPU benchmark\n");
    std::printf ("====================\n");
    std::printf ("Reference machine: %d hardware threads, single process, Release build.\n\n",
                 (int) std::thread::hardware_concurrency());

    printBenchHeader();

    for (const auto& benchCase : cases)
    {
        printBenchRow (runBench (benchCase, 48000.0, 256, 8.0));
        std::fflush (stdout);
    }

    std::printf ("\nBuffer-size sweep (8 voices, dual + effects, 48 kHz)\n");
    printBenchHeader();

    const BenchCase dual = { "8 voices, dual + chorus + reverb", 8, true, false, false };

    for (int blockSize : { 64, 128, 256, 512 })
    {
        printBenchRow (runBench (dual, 48000.0, blockSize, 6.0));
        std::fflush (stdout);
    }

    std::printf ("\nSample-rate sweep (8 voices, dual + effects, 256 frames)\n");
    printBenchHeader();

    for (double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        printBenchRow (runBench (dual, sampleRate, 256, 6.0));
        std::fflush (stdout);
    }

    return 0;
}

//==============================================================================
double noteToHz (int note)
{
    return 440.0 * std::pow (2.0, ((double) note - 69.0) / 12.0);
}

Params aliasPatch (const char* id)
{
    Params p = defaultParams();
    p.layerMode = 0.0f;
    p.layerLevel = 0.0f;
    p.ensemble = 0.0f;
    p.space = 0.0f;
    p.spread = 0.0f;
    p.vintage = 0.0f;
    p.ring = 0.0f;
    p.noise = 0.0f;
    p.vco2Level = 0.0f;
    p.vco1Level = 1.0f;
    p.vco1Sub = 0.0f;
    p.vco1Detune = 0.0f;
    p.vco2Detune = 0.0f;
    p.vco1Octave = 0.0f;
    p.vco2Octave = 0.0f;
    p.filterTrack = 0.0f;
    p.fIl = p.fAl = p.fS = 1.0f;
    p.fA = p.fD = 0.01f;
    p.fR = 0.05f;
    p.aA = 0.002f;
    p.aD = 0.01f;
    p.aS = 1.0f;
    p.aR = 0.05f;
    p.lfoPitch = p.lfoPwm = p.lfoFilter = p.lfoAmp = 0.0f;
    p.aftertouch = 0.0f;
    p.velocity = 0.0f;
    p.masterDb = -12.0f;
    p.drive = 0.15f;
    p.hpfCutoff = 20.0f;
    p.hpfRes = 0.0f;
    p.lpfCutoff = 9000.0f;
    p.lpfRes = 0.0f;

    if (std::strcmp (id, "saw-low") == 0)
    {
        // reference: clean band-limited saw
    }
    else if (std::strcmp (id, "saw-high") == 0)
    {
        // high note pushes the polyBLEP correction hard
    }
    else if (std::strcmp (id, "pulse-narrow") == 0)
    {
        p.vco1Level = 0.0f;
        p.vco2Level = 1.0f;
        p.vco2Pw = 0.05f;
    }
    else if (std::strcmp (id, "resonant-drive") == 0)
    {
        p.drive = 1.0f;
        p.lpfRes = 1.0f;
        p.lpfCutoff = 850.0f;
    }
    else if (std::strcmp (id, "ring-mod") == 0)
    {
        p.ring = 1.0f;
        p.ringSpeed = 620.0f;
        p.vco2Level = 0.7f;
    }
    else if (std::strcmp (id, "max-brightness") == 0)
    {
        p.drive = 1.0f;
        p.vco2Level = 1.0f;
        p.lpfCutoff = 16000.0f;
        p.brilliance = 1.0f;
    }

    return p;
}

struct AliasCase
{
    const char* id;
    int note;
};

int runAliasSuite()
{
    const AliasCase cases[] = {
        { "saw-low", 36 },
        { "saw-high", 84 },
        { "pulse-narrow", 60 },
        { "resonant-drive", 60 },
        { "ring-mod", 57 },
        { "max-brightness", 72 },
    };

    std::printf ("CS-80E aliasing analysis\n");
    std::printf ("========================\n");
    std::printf ("Single sustained note, 2 s settle, 65536-sample Blackman-Harris FFT.\n");
    std::printf ("Values are dB relative to the fundamental amplitude.\n\n");

    for (double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        std::printf ("### %.1f kHz\n\n", sampleRate / 1000.0);
        std::printf ("| Patch | Note | Worst alias | Alias energy | Noise floor | Worst alias Hz |\n");
        std::printf ("| --- | --- | --- | --- | --- | --- |\n");

        for (const auto& aliasCase : cases)
        {
            Engine engine;
            engine.prepare (sampleRate);
            engine.setParams (aliasPatch (aliasCase.id));
            engine.noteOn (aliasCase.note, 0.9f);

            const int total = (int) std::lround (sampleRate * 2.0);
            std::vector<float> left ((size_t) total), right ((size_t) total);
            const int blockSize = 256;

            for (int done = 0; done < total; done += blockSize)
            {
                const int n = std::min (blockSize, total - done);
                engine.process (left.data() + done, right.data() + done, n);
            }

            const size_t fftSize = 65536;
            std::vector<float> window (left.end() - (long) fftSize, left.end());
            const auto report = cs80test::analyseAliasing (window, sampleRate,
                                                            noteToHz (aliasCase.note));

            std::printf ("| %s | %d | %.1f dB | %.1f dB | %.1f dB | %.0f |\n",
                         aliasCase.id, aliasCase.note, report.worstAliasDb,
                         report.aliasEnergyDb, report.noiseFloorDb, report.worstAliasHz);
            std::fflush (stdout);
        }

        std::printf ("\n");
    }

    return 0;
}

//==============================================================================
int runLatencySuite()
{
    std::printf ("CS-80E latency analysis\n");
    std::printf ("=======================\n\n");

    std::printf ("Reported engine latency: %d host samples (%.3f ms at 48 kHz)\n",
                 Engine::latencySamples(), 1000.0 * Engine::latencySamples() / 48000.0);

    std::printf ("Oversampled voice path: %.2fx host rate, decimator taps: %d\n",
                 Engine::oversamplingFactor(), Decimator2x::kTaps);

    std::printf ("\nDecimator impulse response (host-rate indices):\n");

    Decimator2x decimator;
    decimator.reset();

    std::vector<float> output;

    for (int n = 0; n < 48; ++n)
    {
        const float odd = 0.0f;
        const float even = n == 0 ? 1.0f : 0.0f;
        decimator.pushOdd (odd);
        output.push_back (decimator.pushEven (even));
    }

    int peakIndex = 0;
    float peak = 0.0f;

    for (int i = 0; i < (int) output.size(); ++i)
    {
        if (std::abs (output[(size_t) i]) > peak)
        {
            peak = std::abs (output[(size_t) i]);
            peakIndex = i;
        }
    }

    for (int i = 0; i < 24; ++i)
        std::printf ("  %2d: % .6f\n", i, output[(size_t) i]);

    std::printf ("\nImpulse peak at host sample %d (amplitude %.4f); engine reports %d.\n",
                 peakIndex, peak, Engine::latencySamples());

    return peakIndex == Engine::latencySamples() ? 0 : 1;
}
} // namespace

int main (int argc, char** argv)
{
    const std::string mode = argc > 1 ? std::string (argv[1]) : "bench";

    if (mode == "bench")
        return runBenchSuite();

    if (mode == "alias")
        return runAliasSuite();

    if (mode == "latency")
        return runLatencySuite();

    std::printf ("usage: %s [bench|alias|latency]\n", argv[0]);
    return 2;
}
