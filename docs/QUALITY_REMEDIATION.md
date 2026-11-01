# CS-80E quality remediation register

Live engineering register for the 5.1 overhaul. Every row records the defect,
its root cause, the correction, the acceptance test, and the actual verification
result. Items that are not complete stay marked **OPEN** or **BLOCKED**; they are
not described as done.

Baseline: `main` at `2dcf308`, version 5.0.0, 3,928 headless checks passing.

Verification commands used throughout:

```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release \
      -DCS80_BUILD_PLUGIN=OFF -DCS80_BUILD_TOOLS=OFF
cmake --build build-tests --parallel
./build-tests/src/dsp/cs80_tests
./build-tests/tools/analysis/cs80_analysis bench
./build-tests/tools/analysis/cs80_analysis alias
./build-tests/tools/analysis/cs80_analysis latency
```

---

## Q-01 — Voice-stealing test never forced a steal

- **Severity:** High (false confidence in the most failure-prone voice path)
- **Root cause:** `testVoiceStealingIsSmooth()` started one note, then a second.
  Eight voice slots were available, so no voice was stolen and the declick path
  was never executed.
- **Affected files:** `src/dsp/tests/DspTests.cpp`
- **Correction:** Added an eight-voice fill plus a ninth note, voice-slot
  inspection (`Engine::voiceNote`, `Engine::voiceIsActive`), and tests for
  attack/decay/sustain/release steals, sustain pedal, repeated notes, 500 rapid
  retriggers, dual layers, high resonance/drive, poly pressure and note-off after
  a steal. The declick hand-over was changed from a 32-sample linear ramp with a
  residual step to a 64-sample raised cosine with exact endpoints.
- **Acceptance:** exactly eight active voices; the oldest voice is replaced;
  note-off for the stolen note does not release the new one; boundary
  discontinuity no larger than normal waveform movement.
- **Verification:** `testVoiceStealingPolicyAndContinuity`,
  `testStealDuringEveryEnvelopePhase`, `testSustainPedalStealing`,
  `testRepeatedNotesAndRapidRetrigger`,
  `testStealWithDualLayersDriveAndPressure`.
- **Result:** PASS. Boundary step 0.0056 against a steady 99.9th-percentile step
  of 0.0745; early-window maximum 0.031 against 0.053 later in the new note.
  All eight voices remain allocatable, no stuck notes in 500 retriggers.
- **Commit:** "Fix voice stealing, latency reporting and effect tails"

## Q-02 — Reported plugin latency was zero

- **Severity:** High (PDC, offline bounce and automation alignment)
- **Root cause:** `prepareToPlay()` called `setLatencySamples(0)` while the voice
  path ran through a 33-tap half-band decimator with 8 host samples of group
  delay.
- **Affected files:** `src/plugin/PluginProcessor.cpp`, `src/dsp/Dsp.h`,
  `src/dsp/Engine.*`
- **Correction:** `Decimator2x::latencyHostSamples()` derives the delay from the
  filter topology; `Engine::latencySamples()` exposes it; the processor reports
  it. `getTailLengthSeconds()` raised from 8 s to 12 s to match the plate.
- **Acceptance:** reported latency equals measured decimator peak within zero
  samples; constant input has unity gain; value correct at 44.1–192 kHz.
- **Verification:** `cs80_analysis latency`, `testLatencyAndDecimator`.
- **Result:** PASS. Impulse peak at host sample 8, reported 8; DC gain 0.99997.
- **Remaining limitation:** the JUCE report itself was not observed in a host
  here (no JUCE build); CI's pluginval/auval exercise the plugin.
- **Commit:** "Fix voice stealing, latency reporting and effect tails"

## Q-03 — Audible oscillator aliasing

- **Severity:** High (sound quality)
- **Root cause:** PolyBLEP leaves significant alias energy on high notes and
  narrow pulses. Measured worst alias: −38 dB for a C6 saw, −38 dB for a 5 %
  pulse, −38 dB for a maximum-brightness patch.
- **Affected files:** `src/dsp/Oscillator.h` (new), `src/dsp/Engine.cpp`,
  `src/dsp/Dsp.h`
- **Correction:** Additive quarter-octave wavetable mip maps limited to 45 % of
  the host rate, with cubic interpolation for low bands and linear above 96
  harmonics. Pulse = difference of two band-limited saw reads. The VCA and output
  transformer moved to a cubic saturator with first-order ADAA.
- **Acceptance:** ≥ 65 dB below the fundamental for normal and extreme
  oscillator patches; no regression at 44.1/48/96 kHz.
- **Verification:** `cs80_analysis alias`, `testAliasingTargets`.
- **Result:** PASS. Worst case −66.3 dB at 48 kHz (was −38.0 dB); high saw −79 dB
  (was −38 dB); narrow pulse −71 dB (was −38 dB). Full tables in
  [DSP_VALIDATION.md](DSP_VALIDATION.md).
- **Commit:** "Replace PolyBLEP oscillators with band-limited wavetables"

## Q-04 — Effects froze their delay lines and replayed stale audio

- **Severity:** High (audible burst when automating chorus/reverb)
- **Root cause:** `Chorus::process()` and `PlateReverb::process()` returned
  immediately when the mix was near zero. Their delay buffers and feedback
  states kept the last audio; re-enabling the effect replayed it. A first fix
  introduced a `cold` flag but never cleared it when processing resumed, which
  made the freeze permanent; this was caught by the new regression test.
- **Affected files:** `src/dsp/Engine.cpp`
- **Correction:** Per-sample smoothed wet/dry gains, immediate `cold = false`
  when processing resumes, and a genuine cold clear (buffers, feedback, filters)
  once the mix is zero and the tail is below −120 dB.
- **Acceptance:** no burst or step on re-enable after a long silent period;
  smooth live toggling; stable feedback; buffer-size independent.
- **Verification:** `testEffectBypassIsSmooth` (peak and step after re-enable),
  `testReverbTail`.
- **Result:** PASS. Re-enable peak 0.000000 for both effects; live chorus toggle
  edge 0.077 vs 0.105 steady.
- **Commit:** "Fix voice stealing, latency reporting and effect tails"

## Q-05 — Decimator coefficients could initialise on the audio thread

- **Severity:** Medium (real-time safety)
- **Root cause:** The FIR table was a function-local `static` built by a lambda
  on first use; the first use can be inside the audio callback.
- **Affected files:** `src/dsp/Dsp.h`, `src/dsp/Engine.cpp`
- **Correction:** `Decimator2x::prepare()` builds the table; `Layer::prepare()`
  calls it. The wavetable bank is likewise built in `Engine::prepare()`.
- **Acceptance:** zero heap allocations in the callback; coefficients ready
  before processing.
- **Verification:** `testRealtimeAllocations` (replaced global allocator),
  `cs80_analysis bench` allocation column.
- **Result:** PASS. 0 allocations across 400 callbacks with MIDI and parameter
  changes; 0 allocations in every benchmark case.
- **Commit:** "Move oscillator and FIR preparation off the audio thread"

## Q-06 — Preset "edited" state was lost after reopening a project

- **Severity:** Medium (user data honesty)
- **Root cause:** `setStateInformation()` set `presetParams = snapshotParams()`,
  so the saved parameters also became the baseline. A modified factory patch
  reopened looking unmodified.
- **Affected files:** `src/plugin/PluginProcessor.cpp`
- **Correction:** The state now stores a `CS80E_PRESET_BASELINE` child with the
  baseline value of every parameter, plus `stateVersion = 2`. Legacy states
  without a baseline reconstruct the factory patch for the saved program.
- **Acceptance:** a tweaked preset restores as edited; an unmodified one restores
  as unmodified; old states still load.
- **Verification:** source inspection and state-path review. **No JUCE host test
  was available in this environment**, so this is not claimed as host-verified.
- **Result:** Implemented; **verification pending in a DAW/pluginval run**.
- **Commit:** "Persist the preset baseline with plugin state"

## Q-07 — Tiny, low-contrast UI legends

- **Severity:** Medium (usability)
- **Root cause:** Several engraved legends used 5.0–6.2 px type, and the whole
  panel is uniformly scaled down at 1080×720, reducing them further. Secondary
  text colours were low contrast against the graphite panel.
- **Affected files:** `src/plugin/ui/Theme.cpp`, `src/plugin/ui/Theme.h`
- **Correction:** Sub-8 px type is lifted by 12 % + 0.45 px in the shared font
  factory; `textDim` and `textFaint` were lightened. No layout rectangles were
  moved, so nothing is re-flowed.
- **Acceptance:** essential labels legible at 1080×720 and at design size; no
  clipping.
- **Verification:** **BLOCKED locally** — no JUCE/X11 toolchain in this
  environment. CI renders the real editor from the pushed commit; the resulting
  screenshots must be inspected before release.
- **Result:** Change implemented; visual sign-off pending.
- **Commit:** "Raise small UI type and secondary contrast"

## Q-08 — Test coverage gaps

- **Severity:** High (regression risk)
- **Root cause:** Assertion count was high, but the suite did not force voice
  stealing, measure latency, measure aliasing, count allocations, test the
  effects' bypass behaviour, or unit-test filter/envelope response.
- **Affected files:** `src/dsp/tests/DspTests.cpp`,
  `src/dsp/tests/Spectrum.h` (new)
- **Correction:** Added nine test groups and a shared FFT/alias analysis header.
- **Acceptance:** each defect above has a regression test that would fail on the
  old code.
- **Verification:** suite now 4,007 checks, 0 failures (was 3,928).
- **Result:** PASS.
- **Commit:** "Add spectral, latency, stealing and real-time regression tests"

## Q-09 — CI did not gate publication on analysis or sanitizers

- **Severity:** Medium (release integrity)
- **Root cause:** `publish-latest` depended only on build, screenshots and the
  two pluginval jobs. There was no version/tag consistency check, no sanitizer
  job, and no committed benchmark artifact.
- **Affected files:** `.github/workflows/ci.yml`
- **Correction:** Added `version-check`, `analysis` (bench + alias + latency with
  uploaded logs) and `sanitizers` (ASan + UBSan) jobs; both publishing jobs now
  require them; version and tag must agree before any release job runs.
- **Acceptance:** publication fails if the engine tests, analysis, sanitizers or
  version checks fail.
- **Verification:** workflow inspection; the first post-change run is the
  evidence.
- **Result:** Implemented; CI run status recorded in the release report.
- **Commit:** "Gate releases on analysis, sanitizers and version checks"

---

## Open items (not complete in 5.1)

| Item | Status | Reason |
| --- | --- | --- |
| User preset save / load / rename / import / export / search | **OPEN** | Requires a JUCE-side preset manager and UI; not attempted partially |
| Modulation matrix (8 slots) | **OPEN** | DSP + 24 parameters + UI; not implemented rather than half-wired |
| Microtuning / Scala `.scl` / `.kbm` | **OPEN** | Not implemented |
| MPE zone handling and per-note expression | **OPEN** | Channel pressure and poly pressure work; MPE zones do not |
| MIDI learn | **OPEN** | Not implemented |
| Responsive layout beyond uniform scaling | **OPEN** | Type floor and contrast fixed; a two-layout responsive redesign was not attempted |
| DAW compatibility matrix | **OPEN** | No DAW available in this environment |
| New screenshots generated locally | **BLOCKED here** | No JUCE/X11/Xvfb toolchain; CI renders them from the pushed commit |
| Versioned GitHub Release / tag | **BLOCKED here** | No GitHub release or tag API access in this environment; CI publishes the rolling `latest` release from `main` |
| Hardware CS-80 comparison | **OPEN** | No hardware or legal recordings available |

---

## Unreleased engine work (post-5.1, not yet exposed in the panel)

## Q-10 — No modulation matrix

- **Severity:** Medium (expressiveness; the engine only had fixed LFO routings)
- **Root cause:** Modulation was hard-wired: LFO to pitch/PWM/filter/amplitude.
  There was no way to route velocity, pressure, envelopes or randomness.
- **Affected files:** `src/dsp/Params.*`, `src/dsp/Engine.cpp`,
  `src/dsp/tests/DspTests.cpp`
- **Correction:** Eight routing slots with ten sources (LFO, velocity, channel
  and poly pressure, mod wheel, key track, filter and amp envelopes, per-note
  random) and eight destinations (pitch, pulse width, cutoff, resonance,
  amplitude, pan, drive, layer level). Amounts are bipolar; destinations are
  smoothed per control block; pan is evaluated per chunk through the existing
  pan glide. Slots with an off source/destination or zero depth are removed
  before the voice loop.
- **Acceptance:** velocity -> pitch at +0.5 is exactly one octave; amplitude
  and pan routes change the rendered balance; cutoff routes change high-
  frequency energy; random -> resonance at full depth stays bounded; an LFO
  pitch sweep has no sample-step discontinuity; all 24 parameters round-trip.
- **Verification:** `testModulationMatrix` (direct `g++` build of the committed
  engine; CMake is not installed in this environment).
- **Result:** PASS. 5,176 checks, 0 failures. Octave shift measured at
  440 Hz vs 220 Hz reference; pan route leaves <0.005 RMS left and >0.02 RMS
  right; cutoff route triples first-difference energy; peak stays <1.01.
- **Remaining limitation:** the matrix has parameters and host automation but
  **no panel controls yet**. It is not claimed as a shipped UI feature.

## Q-11 — No microtuning

- **Severity:** Medium (expressiveness / non-Western repertoire)
- **Root cause:** Pitch was a closed-form 12-TET calculation inside the voice.
- **Affected files:** `src/dsp/Tuning.h` (new), `src/dsp/Engine.*`,
  `src/dsp/tests/DspTests.cpp`
- **Correction:** A 128-note tuning table with Scala `.scl` (cents and ratio
  intervals) and `.kbm` mapping support, published to the engine through two
  preallocated slots and an atomic index. 12-TET at A4=440 is the default and
  reproduces the previous frequencies exactly. Malformed files are rejected
  without changing the active tuning.
- **Acceptance:** default table matches 440 * 2^((n-69)/12); a five-note scale
  is correct at the octave; a .kbm with a 432 Hz reference renders A3 at
  216 Hz; bad input leaves 12-TET intact.
- **Verification:** `testMicrotuning`; engine render measured at 216.0 Hz.
- **Result:** PASS. 5,176 checks, 0 failures.
- **Remaining limitation:** there is no file chooser or tuning panel yet, and
  the tuning is not serialised by the plugin state. Engine-only.

## Verified measurements after the engine work

`cs80_analysis` was rebuilt directly with `g++ -O2` (no CMake in this
environment). Raw output: `docs/benchmarks/v5.2-modmatrix-bench.txt`,
`v5.2-modmatrix-alias.txt`, `v5.2-modmatrix-latency.txt`.

- Latency remains 8 host samples; impulse peak at sample 8.
- 8 voices, dual + chorus + reverb, 48 kHz/256 frames: 800 µs average,
  15.0 % of one core, 0 allocations. That is within run-to-run variation of the
  5.1 figure (759 µs / 14.2 %); no CPU improvement is claimed.
- The idle path still costs 4.9 µs per callback.
