# CS-80E

### A dual-channel, eight-voice CS-80 tribute — VST3 / AU / Standalone

[![CI](https://github.com/boop-bip80/synth1/actions/workflows/ci.yml/badge.svg)](https://github.com/boop-bip80/synth1/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/boop-bip80/synth1?label=latest&color=ff9d3c)](https://github.com/boop-bip80/synth1/releases/latest)
[![License: AGPL-3.0](https://img.shields.io/badge/license-AGPL--3.0-ff9d3c.svg)](LICENSE)
![Formats](https://img.shields.io/badge/formats-VST3%20%7C%20AU%20%7C%20Standalone-2b3038)
![Tests](https://img.shields.io/badge/unit%20checks-3%2C499-7fce9b)

<p align="center">
  <img src="docs/screenshots/01-blade-brass.png" width="100%" alt="CS-80E dual-channel panel">
</p>

CS-80E is a from-scratch polyphonic synthesizer written in C++17 with JUCE. The
sound is a deliberately characterful take on the Yamaha CS-80 voice: two
independent channel cards, two band-limited oscillators per card, a sub-octave,
a dedicated ring carrier, a resonant high-pass into a resonant low-pass, the
CS-80's two-level filter envelope, polyphonic aftertouch, polyphonic portamento
and a spring-loaded pitch ribbon.

Version 4.0 is a ground-up sound and surface rebuild. The CS-80's sine path is
now a first-class mix control that bypasses the VCF and feeds the VCA directly,
adding weight and body beneath the brass. A dedicated Initial Bend ("Twang")
control reproduces the CS-80's velocity-driven pitch snap at note-on. The two
channel cards drift independently, the VCA saturates gently, resonance is
gain-compensated, the ensemble is a four-tap BBD, and the plate uses four
slowly modulated allpasses. The factory bank has grown to 32 patches.

The entire panel was repainted in code — deep graphite, figured walnut cheeks,
brass rails, section-coloured fader caps and header lights, a chrome badge, warm
amber LEDs, a dual-trace phosphor scope and a bronze ribbon controller. No bitmap
skin, no external editor dependency.

---

## Download the builds

The links below always point at the newest successful build on `main`:

| Platform | Download | Formats |
| --- | --- | --- |
| **Linux** | [CS-80E-linux.zip](https://github.com/boop-bip80/synth1/releases/latest/download/CS-80E-linux.zip) | VST3 + Standalone |
| **macOS** | [CS-80E-macos.zip](https://github.com/boop-bip80/synth1/releases/latest/download/CS-80E-macos.zip) | VST3 + AU + Standalone |
| **Windows** | [CS-80E-windows.zip](https://github.com/boop-bip80/synth1/releases/latest/download/CS-80E-windows.zip) | VST3 + Standalone |
| **Renders + demo** | [CS-80E-previews.zip](https://github.com/boop-bip80/synth1/releases/latest/download/CS-80E-previews.zip) | ten PNGs + demo WAV |
| **Build manifest** | [latest.json](https://github.com/boop-bip80/synth1/releases/latest/download/latest.json) | JSON |
| **Checksums** | [SHA256SUMS.txt](https://github.com/boop-bip80/synth1/releases/latest/download/SHA256SUMS.txt) | SHA-256 |

Every push to `main` builds Linux, macOS and Windows, runs the headless suite,
renders the panel, validates the Linux VST3 with
[pluginval](https://github.com/Tracktion/pluginval), refreshes the rolling
**latest** release and writes a `latest.json` manifest with the current links.
A `v*` tag also publishes a versioned GitHub Release.

### Install

- **Linux:** copy `VST3/CS-80E.vst3` to `~/.vst3/` (or run the standalone binary).
- **macOS:** copy `VST3/CS-80E.vst3` to `~/Library/Audio/Plug-Ins/VST3/`,
  `AU/CS-80E.component` to `~/Library/Audio/Plug-Ins/Components/`, or run the
  app from `Standalone/`.
- **Windows:** copy `VST3\CS-80E.vst3` to
  `C:\Program Files\Common Files\VST3\`, or run `Standalone\CS-80E.exe`.

The CI builds are unsigned. On macOS, right-click the plugin/app and choose
**Open** the first time. On Windows, allow the app through SmartScreen.

## Screenshots

| | |
| --- | --- |
| ![Vangelis Lead](docs/screenshots/02-vangelis-lead.png) | ![Neon Pads](docs/screenshots/03-neon-pads.png) |
| ![Brass Stab](docs/screenshots/05-brass-stab.png) | ![Blade Runner](docs/screenshots/09-blade-runner.png) |

<p align="center">
  <img src="docs/screenshots/04-preset-library.png" width="100%" alt="CS-80E four-column preset library">
</p>

<p align="center">
  <img src="docs/screenshots/06-filter-closeup.png" width="49%" alt="Filter and filter-envelope detail">
  <img src="docs/screenshots/07-vca-sine.png" width="49%" alt="Sine mix and VCA envelope detail">
</p>

<p align="center">
  <img src="docs/screenshots/10-channel-two.png" width="49%" alt="CS-80E Channel II and LFO detail">
</p>

<p align="center">
  <img src="docs/screenshots/08-ribbon-keybed.png" width="100%" alt="Lower panel, ribbon and 61-key keybed detail">
</p>

CI renders these from the real editor at up to 2× resolution. On `main` it also
commits the refreshed images and demo back to `docs/`, and attaches them to the
latest release.

## Listen

[**cs80-demo.wav**](docs/audio/cs80-demo.wav) — 23 seconds of dual-channel Blade
Brass chords and aftertouch swells, rendered offline through the real processor
by the screenshot tool.

## What is new in 4.0

- **The CS-80 sine path.** A new Sine Mix fader bypasses the VCF and joins the
  signal at the VCA, exactly where the original's sine board sits. Push it under
  the brass for weight, or use it alone for glassy flutes and organs.
- **Initial Bend / Twang.** The CS-80 snaps into pitch at note-on. A dedicated
  velocity-scaled bend control now reproduces that expressive twang, with the
  offset decaying over the first few milliseconds of the note.
- **More alive, card by card.** Each channel card gets its own slow drift, and
  cutoff drift is independent of oscillator drift, so stacked layers move
  against each other instead of locking into one static composite.
- **A warmer, denser output.** The VCA saturates gently, resonance is gain
  compensated, the LPF drive stage is smoother, and the ensemble is a four-tap
  BBD with wider stereo movement.
- **A better plate.** Four allpasses with slowly modulated comb reads, 0.862
  feedback and 0.23 damping produce a longer, less metallic tail.
- **A 32-patch bank.** The library gains Chariots Brass, Love Theme, Glass Harp,
  Deep Choir, Brass Section, Ribbon Glide, Split Pad / Bell and Aftertouch
  Strings, alongside rebuilt versions of the classic patches.
- **A completely repainted panel:** deeper graphite and walnut, brass edge trim,
  ten colour-coded sections with engraved waveform watermarks, numbered card
  plates, larger chrome badge, richer scope and keybed, and a four-column
  32-patch library.
- **Release automation:** stable per-OS download links, versioned `latest.json`,
  checksums, ten refreshed renders and a rolling latest release, all driven by
  GitHub Actions.

## What is inside

- **Eight-voice polyphony** with click-free oldest-voice stealing, a short
  declick handover and sustain-pedal handling.
- **Dual channel cards** with independent level, octave, detune, pulse width,
  cutoff offset, resonance, envelope scale and pan.
- **Single / Dual / Split** channel modes, with a 36–96 split point.
- **Living voices:** every voice starts at a scattered phase, drifts a few cents
  at its own slow rate and has its own cutoff, level and envelope variance, so
  stacked chords breathe instead of phase-locking into one giant comb filter.
- **VCO 1:** band-limited polyBLEP saw, sub-octave square, ±2 octave switch,
  detune.
- **VCO 2:** band-limited polyBLEP pulse with DC-correct PWM, ±2 octave switch,
  detune.
- **Dedicated ring carrier** (VCO × sine) with speed control, plus a noise
  source.
- **12 dB/oct high-pass and low-pass** filters in series, each resonant, with
  key tracking, an anti-aliased drive stage and a gently saturating low-pass
  core.
- **CS-80-style filter envelope** with Initial Level, Attack Level and A/D/S/R.
- **Sine mix** that bypasses the filter and feeds the VCA, plus the amplitude
  ADSR, velocity that opens the filter as well as the VCA, and a transparent
  output limiter.
- **Polyphonic glide** from the last played note, plus master tune and per-voice
  vintage variance.
- **Buffer-size independence:** every glide is time-based and the filters and LFO
  update every eight samples, so the instrument sounds the same at any host
  block size.
- **LFO** (triangle / sine / square / saw) with delay and routings to pitch, PWM,
  filter and tremolo.
- **Expression:** channel and polyphonic aftertouch, mod wheel, pitch bend, a
  spring-loaded ribbon (±12 semitones) and a 61-key poly-touch keybed.
- **Ensemble chorus**, stereo-pre-delayed plate reverb and a brilliance tilt on
  the output.
- **32 factory patches**, a four-column preset library, tooltips and
  double-click parameter reset.
- **Resizable UI** that keeps its hardware proportions from 1080×720 upward.

### The panel

| Section | What it does |
| --- | --- |
| **VCO 1 / VCO 2** | Mix saw/pulse and sub, set pulse width, switch octaves, detune. |
| **RING** | Blend the ring carrier and noise, set the carrier speed. |
| **FILTER** | HPF and LPF cutoffs, resonances, drive and keyboard tracking. |
| **FILTER EG** | IL/AL set the envelope's start and peak; A/D/S/R shape it. |
| **VCA** | Sine mix (post-filter), then the amplitude ADSR. |
| **CHANNEL II** | Layer level, detune, octave, pan, cutoff, resonance, pulse width, envelope scale, mode and split point. |
| **LFO** | Speed, waveform, fade-in delay, and four modulation depths. |
| **PERFORMANCE** | Aftertouch, bend range, stereo spread, initial-bend twang, velocity, glide and vintage. |
| **OUTPUT** | Master, tune, brilliance, ensemble and space, with a stereo meter. |
| **RIBBON** | Drag left/right for pitch; release to spring back. |
| **KEYBED** | 61 keys, velocity from the vertical touch point, poly pressure from a held drag. |
| **LCD / LIBRARY** | Previous/next patch arrows; click the glass for the 32-patch library. |

## How the CI works

| Job | What it proves |
| --- | --- |
| `dsp-tests` | The engine builds and passes all headless checks without JUCE. |
| `build` | VST3 + Standalone compile on Linux, macOS and Windows (AU also on macOS); tests pass; zips are downloadable. |
| `screenshots` | The real editor is rendered offscreen to ten PNGs plus a demo WAV; refreshed renders are committed back to `main`. |
| `pluginval` | The Linux VST3 is validated by pluginval at strictness level 5. |
| `publish-latest` | Linux, macOS and Windows zips, checksums, `latest.json` and ten previews are attached to the rolling `latest` release only after the build, renders and validation succeed. |
| `release` | A `v*` tag publishes a versioned GitHub Release with all platform zips. |

Screenshots are generated by `Cs80Screenshots`, which instantiates the actual
plugin processor and editor, plays chords through the real MIDI path, and snaps
the component tree. Fonts are embedded, so the renderer is deterministic and
needs no system font packages.

## Build locally

Requirements: **CMake 3.22+**, a **C++17** compiler, and network access the first
time so CMake can fetch JUCE 8.

```bash
# Linux dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake \
  libasound2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

# Headless DSP suite
ctest --test-dir build -C Release --output-on-failure

# Panel screenshots + demo audio
./build/tools/Cs80Screenshots_artefacts/Release/Cs80Screenshots docs
```

Useful options:

```bash
-DCS80_BUILD_PLUGIN=OFF   # engine + tests only (no JUCE download)
-DCS80_BUILD_TOOLS=OFF    # plugin only, skip the offline renderer
-DCS80_BUILD_TESTS=OFF    # skip the unit test executable
```

The plugin targets are written to
`build/src/plugin/Cs80Plugin_artefacts/Release/`.

## Architecture

```
src/dsp/            Pure C++17 engine — no JUCE, fully unit-testable
  Params.*            parameter table + POD snapshot
  Dsp.h               polyBLEP oscillators, ADAA drive, nonlinear SVF,
                      segment envelopes
  Engine.*            eight dual-channel voices, glide, modulation, ring carrier,
                      ensemble, stereo-pre-delay plate, transparent limiter
  Presets.*           32 factory patches
  tests/              3,499 headless assertions

src/plugin/         JUCE wrapper
  PluginProcessor.*   APVTS, presets, MIDI dispatch, state persistence
  PluginEditor.*      panel layout and preset library wiring
  ui/                 theme, look and feel, hardware controls, dual scope,
                      meters, ribbon, 61-key poly-touch keybed and preset browser

tools/screenshot/   Offline editor renderer + demo WAV
.github/workflows/  Build, test, screenshot, pluginval, latest and release pipeline
assets/fonts/       Embedded Chakra Petch, JetBrains Mono and Michroma (OFL)
docs/               Committed screenshots and the demo render
```

Designed boundaries: the engine never includes JUCE, so the full signal path can
run in a plain test binary on every platform. The JUCE layer only translates
parameters, MIDI and state, and paints the panel.

## Tests

The headless suite covers:

- parameter table integrity, ranges, defaults and every spec↔struct mapping;
- preset application, uniqueness and clamping across all 32 patches;
- silence when idle; finite output at every parameter extreme;
- 8-voice polyphony, click-free voice stealing with a declick handover;
- dual-channel layering, keyboard splits, layer panning and the ring carrier;
- sub-oscillator tracking, polyphonic glide, master tune and vintage spread;
- sine-mix bypass level, initial-bend decay and independent card drift;
- identical rendering at 32- and 512-sample host block sizes;
- A4 tuning at 44.1, 48 and 96 kHz, octave switches and 24-semitone bend;
- filter brightness response, ring/chorus stereo behaviour and reverb decay;
- high drive/resonance stability, sustain pedal, retriggering, aftertouch/mod
  wheel and deterministic renders;
- every factory preset rendering with sane peak levels.

CI additionally builds the plugin on three operating systems and validates the
Linux VST3 with pluginval (`--strictness-level 5 --skip-gui-tests`).

## License

The project is licensed under **AGPL-3.0**. JUCE is used under its AGPLv3
option; see [LICENSE](LICENSE) and the [JUCE licensing page](https://juce.com/legal/juce-8-licence/).

Bundled fonts (Chakra Petch, JetBrains Mono, Michroma) are under the SIL Open
Font License 1.1; see [assets/fonts/OFL.txt](assets/fonts/OFL.txt).

CS-80 is a trademark of Yamaha Corporation. **CS-80E is an unofficial tribute and
is not affiliated with or endorsed by Yamaha.** All sound generation and UI
painting here is original code.

---

Built with C++17, JUCE 8, CMake and GitHub Actions.
