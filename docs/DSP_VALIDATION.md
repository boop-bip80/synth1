# CS-80E DSP validation

This document records what was measured, how, and what remains unverified. It
deliberately separates **measured behaviour**, **published reference
information**, and **engineering design decisions**. CS-80E is a tribute
instrument built from original code; no claim is made that it is a
circuit-accurate reproduction of Yamaha hardware, and no hardware unit was
available for comparison during this work.

Reproduce every number below with the committed tools:

```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release \
      -DCS80_BUILD_PLUGIN=OFF -DCS80_BUILD_TOOLS=OFF
cmake --build build-tests --parallel
./build-tests/src/dsp/cs80_tests          # assertions, including spectral limits
./build-tests/tools/analysis/cs80_analysis bench
./build-tests/tools/analysis/cs80_analysis alias
./build-tests/tools/analysis/cs80_analysis latency
```

Raw runs: [`docs/benchmarks/`](benchmarks/).

### 5.3 regression update

The C++17 headless suite completed **5,255 checks with zero failures** on the
5.3 source using `g++ -O2 -pthread`. New gates check absolute Scala `.scl`
pitches, integer ratios, `.kbm` reference frequencies with decimals, mapping
anchored to the middle note, unassigned-key silence, invalid reference keys and
overflow rejection without corrupting the active tuning. A separate test
publishes 3,000 tunings from one thread while 3,000 audio blocks render in
another, ensuring finite audio and correct final tuning. The audio callback
reads a pinned immutable snapshot; publication uses three slots and a mutex
only on the control side. Cross-platform sanitizers and plugin-level state/UI
checks are run separately by CI and must not be inferred from this local run.

## 1. Processing latency

The voice path runs at 2× the host rate through a 33-tap linear-phase half-band
decimator. Group delay is `(N - 1) / 2` at the 2× rate, i.e. `(33 - 1) / 4 = 8`
host samples. The value is derived from the filter topology
(`Decimator2x::latencyHostSamples()`), not hard-coded in the processor, and is
reported to the host in `prepareToPlay()`.

Verification: an impulse is pushed through the decimator and the output peak is
located. `cs80_analysis latency` reports:

```
Reported engine latency: 8 host samples (0.167 ms at 48 kHz)
Oversampled voice path: 2.00x host rate, decimator taps: 33
Impulse peak at host sample 8 (amplitude 0.49997)
```

| Host rate | Reported latency | Time |
| --- | --- | --- |
| 44.1 kHz | 8 samples | 0.181 ms |
| 48 kHz | 8 samples | 0.167 ms |
| 88.2 kHz | 8 samples | 0.091 ms |
| 96 kHz | 8 samples | 0.083 ms |
| 192 kHz | 8 samples | 0.042 ms |

The half-band kernel sums to unity over both phases: a constant input produces
unity output (asserted in `testLatencyAndDecimator`). The centre tap alone is
0.5 by design, which is why the impulse peak amplitude is 0.5.

**Not verified here:** the JUCE processor's reported latency was not observed in
a running host in this environment, because a JUCE/X11 build was not available.
The processor calls `setLatencySamples(Engine::latencySamples())`; the value is
covered by the engine test and will be exercised by pluginval and `auval` in CI.

## 2. Aliasing

Method: one sustained note, 2 s to settle, 65,536-sample Blackman-Harris window,
radix-2 FFT. Bins within ±10 bins of every harmonic are excluded, as are bins
below 40 Hz. The largest remaining bin is reported relative to the fundamental
amplitude; "energy" is the sum of all remaining bins relative to the fundamental
power. The oscillator under test is a single layer with chorus, reverb, vintage
drift, LFO, noise and ring modulation disabled, and detune set to zero so the
harmonic grid is exact.

### Before 5.1 (commit `2dcf308`, polyBLEP oscillators)

| Patch | 44.1 kHz | 48 kHz | 96 kHz |
| --- | --- | --- | --- |
| Clean saw, low (A1) | −61.0 dB | −61.7 dB | −66.7 dB |
| Clean saw, high (C6) | −41.2 dB | −38.4 dB | −44.0 dB |
| Narrow pulse (C4, 5 % width) | −32.8 dB | −38.0 dB | −38.3 dB |
| Resonant + drive | −60.5 dB | −62.6 dB | −70.9 dB |
| Maximum brightness + drive | −39.4 dB | −38.0 dB | −46.0 dB |

### After 5.1 (band-limited wavetables)

| Patch | 44.1 kHz | 48 kHz | 96 kHz |
| --- | --- | --- | --- |
| Clean saw, low (A1) | −100.1 dB | −101.0 dB | −99.6 dB |
| Clean saw, high (C6) | −78.2 dB | −79.4 dB | −83.8 dB |
| Narrow pulse (C4, 5 % width) | −75.8 dB | −71.2 dB | −76.3 dB |
| Resonant + drive | −72.4 dB | −73.7 dB | −95.9 dB |
| Maximum brightness + drive | −66.7 dB | −66.3 dB | −72.4 dB |

The oscillators are now quarter-octave mip maps built additively at prepare time,
with every harmonic kept below 45 % of the host rate. A pulse is the difference
of two reads of the same band-limited saw, so it is band-limited by construction
at every pulse width. The worst case measured across the matrix is −66.3 dB,
against a target of −65 dB under normal conditions.

The selected patches are deliberately demanding. PWM at 5 % width, maximum
drive and maximum brilliance are extreme settings; they are reported separately
rather than hidden inside an average.

**Ring modulation is excluded from this table.** Its sidebands are intentional
non-harmonic content, so the harmonic-mask metric classifies them as "alias".
The analysis tool still prints the ring-mod row, labelled by patch name, and the
test suite does not assert an alias threshold for it. Measuring ring-modulator
aliasing would require a different metric (for example, comparing against an
analytically computed sideband set).

Regression gates in `testAliasingTargets` (48 kHz):

| Case | Gate |
| --- | --- |
| Clean saw A2 | < −80 dB |
| Clean saw C6 | < −65 dB |
| Narrow pulse C4 | < −65 dB |
| Maximum brightness C5 | < −60 dB |
| Sub-octave saw C1 | < −80 dB |

### What changed in the signal path

- PolyBLEP saw/pulse generation was replaced with additively generated,
  band-limited mip-mapped tables (`src/dsp/Oscillator.h`). Reads use Catmull-Rom
  interpolation for the low bands with many harmonics, and linear interpolation
  above 96 harmonics where the error is far below the noise floor.
- The VCA curve and the output transformer now use a normalised cubic saturator
  with first-order antiderivative anti-aliasing (`WarmSatAdaa`), replacing
  `fastTanh` at the host rate. This removes a second alias source that the
  oscillator fix alone would not have addressed.
- The 2× voice path and the 33-tap decimator are unchanged in topology. Their
  latency is now reported correctly.

## 3. Filters

Direct unit measurements of the topology-preserving state-variable filter at
48 kHz (`testFilterUnitResponse`):

| Measurement | Result |
| --- | --- |
| Passband gain at 0.1 × cutoff | 1.000 |
| Gain at cutoff, Q = 0.707 | 0.707 |
| Gain at 8 × cutoff | 0.0129 (−37.8 dB) |
| Gain at cutoff, Q = 14 | 14.0 (+22.9 dB) |
| Stability sweep 44.1 / 48 / 88.2 / 96 / 192 kHz, Q = 40, cutoff 20 Hz–20 kHz | finite, peak < 8.0 |

The filter is a TPT/ZDF state-variable design with saturation on the integrator
state. No component values, transformer models or CS-80 circuit measurements are
claimed. The filter is stable at extreme settings, does not self-oscillate
without input, and has the expected 12 dB/octave slopes.

**Not verified:** comparison against a physical CS-80 filter, or against
published measured response curves. The CS-80's actual HPF/LPF topology and
component values were not reproduced here. Treat the filter as a musical
approximation, not a historical model.

## 4. Envelopes

Direct measurement of the segment envelope at 48 kHz
(`testEnvelopeTiming`), A = 100 ms, D = 200 ms, R = 300 ms:

| Segment | Measured |
| --- | --- |
| Attack to 99.9 % | 100.0 ms |
| Decay to sustain | 200.0 ms |
| Release to idle | 300.0 ms |

The envelope forces the exact stage length at the end of each segment, so the
timing does not depend on the exponential coefficient reaching a threshold.

## 5. Effects

The chorus and plate reverb now run a per-sample smoothed wet/dry gain. When the
mix reaches zero and the effect's own tail has decayed below −120 dB, the delay
buffers and feedback states are cleared and the effect stops processing. When it
is re-enabled, the cleared buffers produce no stale audio and the wet gain ramps
in over ~20 ms.

Measured re-enable behaviour after 1–3 s of silence (`testEffectBypassIsSmooth`):

| Effect | Peak after re-enable | Largest sample step |
| --- | --- | --- |
| Five-tap ensemble | 0.000000 | 0.000000 |
| Plate reverb | 0.000000 | 0.000000 |

Toggling the chorus on under a sounding four-note chord produced an edge step of
0.077 against a steady-state 99.9th-percentile step of 0.105, i.e. no
discontinuity beyond the normal waveform movement.

The old implementation returned immediately when the mix reached zero, leaving
the delay lines frozen with old audio inside them; re-enabling the effect then
played that audio back as a burst. This is fixed and covered by regression tests.

## 6. Real-time safety

`testRealtimeAllocations` replaces the global allocator and counts allocations
across 400 consecutive 128-sample callbacks, including note-on, note-off,
poly/channel pressure, pitch bend, sustain, parameter replacement and
all-notes-off. Result: **0 allocations**.

The FIR coefficient table and the wavetable bank are now built in `prepare()`,
outside the audio callback. Previously the decimator's coefficient table was a
function-local static initialised on first use, which could have run on the audio
thread.

No locks, file access, logging or unbounded loops were found in the audio path.
The remaining `std::exp` calls occur in coefficient updates at control rate
(every 8 samples) and in per-block gain smoothing, not per sample.

## 7. Oscillator tuning

A4 measures 437.5–440 Hz with a zero-crossing estimator on 0.4–0.5 s windows at
44.1, 48, 88.2, 96, 192 kHz; the estimator quantises to whole crossings, so the
reported figure is within one cycle of the window. Octave switches, ±24-semitone
bend, master tune and the sub-oscillator (one octave below VCO 1) are covered by
the suite. Wavetable lookup preserves phase continuity, so no tuning drift was
introduced.

## 8. Known limitations and unverified areas

- **No hardware comparison.** No CS-80 or measured hardware recordings were
  available. All "CS-80 character" statements are design intent.
- **No DAW testing in this environment.** VST3/AU hosting, PDC, offline bounce
  and state restoration in a real project were not exercised locally. CI runs
  pluginval (level 5) and `auval`; those are automated validators, not proof of
  compatibility with every DAW.
- **No JUCE build locally.** The GUI type and colour changes were not rendered
  here. CI's screenshot job renders the real editor from the pushed commit.
- **No listening tests** were performed in this environment. The DSP changes are
  justified by measurement, not by a blind listening panel.
- **Aliasing at 192 kHz** was not measured in the supplied matrix; the analysis
  tool supports it and the test suite covers tuning and stability there.
- **Not implemented in 5.1:** a user preset manager with import/export, a
  modulation matrix, microtuning/Scala import, and MPE zone handling. These
  remain open in the quality register and are not claimed in the README.

## 12. Modulation matrix and microtuning (unreleased engine work)

Both features were verified with the headless suite built directly from the
committed sources. CMake and the JUCE/X11 toolchain are not available in this
environment, so the plugin UI was not built or rendered here.

### Modulation matrix

Eight slots, each with a source, destination and bipolar amount. The routing
test checks the whole signal path, not just parameter storage:

| Test | Expectation | Measured |
| --- | --- | --- |
| Velocity -> pitch, depth +0.5, full velocity | one octave | 440 Hz from a 220 Hz reference |
| Velocity -> amplitude, depth -1.0 | voice is ducked | below 35 % of reference RMS |
| Velocity -> pan, depth +1.0 | hard right | left RMS < 0.005, right RMS > 0.02 |
| Velocity -> cutoff, depth +1.0 | brighter | first-difference energy > 3x |
| Random -> resonance, depth +1.0 | bounded | finite, peak < 1.01 |
| LFO -> pitch, 5 Hz, depth +0.25 | continuous sweep | max sample step < 0.20 |

Unused slots are filtered out before the voice loop, so an empty matrix costs
one source/destination/depth comparison per slot per chunk.

### Microtuning

The tuning table is built on the message thread and published through an
atomic index; the callback only reads 128 doubles. Verification:

| Test | Result |
| --- | --- |
| Default 12-TET | A4 = 440.000 Hz, C4 = 261.626 Hz |
| Fractional note interpolation | matches the closed-form 12-TET value |
| Five-note Scala scale | C, D, E, G, A and the octave at the expected ratios |
| `.kbm` with 432 Hz reference | A4 = 432 Hz, A3 renders at 216.0 Hz |
| Malformed `.scl` | rejected; the previous tuning is unchanged |

**Not verified:** the panel has no matrix editor or tuning file chooser yet;
these controls are not claimed as shipped.
