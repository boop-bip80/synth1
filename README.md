# CS-80E

### A dual-channel, eight-voice CS-80 tribute — VST3 / AU / Standalone

[![CI](https://github.com/boop-bip80/synth1/actions/workflows/ci.yml/badge.svg)](https://github.com/boop-bip80/synth1/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/boop-bip80/synth1?label=latest&color=ff9d3c)](https://github.com/boop-bip80/synth1/releases/latest)
[![License: AGPL-3.0](https://img.shields.io/badge/license-AGPL--3.0-ff9d3c.svg)](LICENSE)
![Formats](https://img.shields.io/badge/formats-VST3%20%7C%20AU%20%7C%20Standalone-2b3038)
![Tests](https://img.shields.io/badge/unit%20checks-3%2C928-7fce9b)

<p align="center">
  <img src="docs/screenshots/01-blade-brass.png" width="100%" alt="CS-80E dual-channel panel">
</p>

CS-80E is a from-scratch polyphonic synthesizer written in C++17 with JUCE. The
sound is a deliberately characterful take on the Yamaha CS-80 voice: two
independent channel cards, two band-limited oscillators per card, a sub-octave,
a dedicated ring carrier, a resonant high-pass into a resonant low-pass, the
CS-80's two-level filter envelope, polyphonic aftertouch, polyphonic portamento
and a spring-loaded pitch ribbon.

Version 5.0 is the deepest rebuild yet. The entire voice path now runs at twice
the host sample rate and returns through a 33-tap half-band decimator, so the
filter's saturation, the drive stage and the VCA curve generate their harmonics
above Nyquist instead of folding them back as digital fizz. A new Filter EG
Depth control scales the CS-80's famously deep envelope sweep. The ensemble is
a five-tap BBD with cubic delay reads, the voice field spreads evenly across the
stereo image instead of alternating hard left and right, and a fixed output
transformer adds low-end weight and a touch of second-harmonic colour. The
factory bank has grown to 36 patches.

The entire panel was repainted in code again — black lacquer and cold steel,
figured walnut cheeks, brass rails, ten jewel-toned sections, a chrome badge,
warm amber glass, a dual-trace phosphor scope and a bronze ribbon controller.
No bitmap skin, no external editor dependency.

---

## Download the builds

The links below always point at the newest successful build on `main`:

| Platform | Download | Formats |
| --- | --- | --- |
| **Linux** | [CS-80E-linux.zip](https://github.com/boop-bip80/synth1/releases/download/latest/CS-80E-linux.zip) | VST3 + Standalone |
| **macOS** | [CS-80E-macos.zip](https://github.com/boop-bip80/synth1/releases/download/latest/CS-80E-macos.zip) | VST3 + AU + Standalone |
| **Windows** | [CS-80E-windows.zip](https://github.com/boop-bip80/synth1/releases/download/latest/CS-80E-windows.zip) | VST3 + Standalone |
| **Renders + demo** | [CS-80E-previews.zip](https://github.com/boop-bip80/synth1/releases/download/latest/CS-80E-previews.zip) | ten PNGs + demo WAV |
| **Build manifest** | [latest.json](https://github.com/boop-bip80/synth1/releases/download/latest/latest.json) | JSON |
| **Checksums** | [SHA256SUMS.txt](https://github.com/boop-bip80/synth1/releases/download/latest/SHA256SUMS.txt) | SHA-256 |

Every push to `main` builds Linux, macOS and Windows, runs the headless suite,
renders the panel, validates the Linux and Windows VST3 builds with
[pluginval](https://github.com/Tracktion/pluginval) and the macOS AU with
`auval`, refreshes the rolling **latest** release and writes a `latest.json`
manifest with the current links. The release is then downloaded back and
verified against `docs/`. A `v*` tag also publishes a versioned GitHub Release
with its own `latest.json`.

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

## What is new in 5.0

- **A 2× oversampled voice path.** Oscillators, high-pass, low-pass, drive and
  the VCA curve now run at twice the host rate into a 33-tap half-band
  decimator. The top end stays silky and open even with high resonance and
  heavy drive, without the brittle aliasing that gives soft synths away.
- **Filter EG Depth.** The CS-80's envelope sweep is now a calibrated control,
  from a subtle movement to the full, vocal, multi-octave plunge. It is the
  seventh fader in the filter-envelope row and it is saved with the patch.
- **A wider, deeper stereo field.** Voices are distributed across the image in
  allocation order and glide to their new position, the two channel cards
  cross-pan, and the five-tap BBD ensemble uses cubic delay reads for a
  smoother, less granular halo.
- **An analog output transformer.** A fixed low-shelf and asymmetric
  second-harmonic stage adds weight and iron before the transparent limiter.
- **A 36-patch bank.** Empire Brass, Solaris Pad, Fifth Element and Chrome
  Strings join the 32 rebuilt classics.
- **A completely repainted panel:** black lacquer and cold steel, figured walnut
  cheeks, brass edge trim, ten jewel-toned sections with engraved watermarks,
  a larger chrome badge, a richer LCD, a category-filtered library with chips,
  twin voice-card lamps and a glossier 61-key poly-touch keybed.
- **Release automation:** stable per-OS download links, versioned `latest.json`,
  screenshot URLs, checksums, ten refreshed renders and a rolling latest
  release, all driven by GitHub Actions.

## What is inside

- **Eight-voice polyphony** with click-free oldest-voice stealing, a short
  declick handover and sustain-pedal handling.
- **Dual channel cards** with independent level, octave, detune, pulse width,
  cutoff offset, resonance, envelope scale and pan.
- **Single / Dual / Split** channel modes, with a 36–96 split point.
- **Living voices:** every voice starts at a scattered phase, drifts a few cents
  at its own slow rate and has its own cutoff, level and envelope variance, so
  stacked chords breathe instead of phase-locking into one giant comb filter.
- **2× oversampled voice path:** the oscillators, both filters, the drive stage
  and the VCA curve run at twice the host rate into a 33-tap half-band
  decimator, so their harmonics stay above Nyquist instead of folding back into
  the audible band. The top end stays open even at high resonance and drive.
- **VCO 1:** band-limited polyBLEP saw, sub-octave square, ±2 octave switch,
  detune.
- **VCO 2:** band-limited polyBLEP pulse with DC-correct PWM, ±2 octave switch,
  detune.
- **Dedicated ring carrier** (VCO × sine) with speed control, plus a noise
  source.
- **12 dB/oct high-pass and low-pass** filters in series, each resonant, with
  key tracking, an anti-aliased drive stage and a gently saturating low-pass
  core.
- **CS-80-style filter envelope** with Initial Level, Attack Level, A/D/S/R
  and a calibrated sweep depth control.
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
- **Five-tap BBD ensemble** with cubic delay reads, a stereo-pre-delayed plate
  reverb, a brilliance tilt and a fixed analog output transformer.
- **36 factory patches**, a category-filtered four-column preset library,
  tooltips and double-click parameter reset.
- **Resizable UI** that keeps its hardware proportions from 1080×720 upward.

### The panel

| Section | What it does |
| --- | --- |
| **VCO 1 / VCO 2** | Mix saw/pulse and sub, set pulse width, switch octaves, detune. |
| **RING** | Blend the ring carrier and noise, set the carrier speed. |
| **FILTER** | HPF and LPF cutoffs, resonances, drive and keyboard tracking. |
| **FILTER EG** | IL/AL set the envelope's start and peak; A/D/S/R shape it, DEPTH scales the sweep. |
| **VCA** | Sine mix (post-filter), then the amplitude ADSR. |
| **CHANNEL II** | Layer level, detune, octave, pan, cutoff, resonance, pulse width, envelope scale, mode and split point. |
| **LFO** | Speed, waveform, fade-in delay, and four modulation depths. |
| **PERFORMANCE** | Aftertouch, bend range, stereo spread, initial-bend twang, velocity, glide and vintage. |
| **OUTPUT** | Master, tune, brilliance, ensemble and space, with a stereo meter. |
| **RIBBON** | Drag left/right for pitch; release to spring back. |
| **KEYBED** | 61 keys, velocity from the vertical touch point, poly pressure from a held drag. |
| **LCD / LIBRARY** | Previous/next patch arrows; click the glass for the 36-patch library. |

## How the CI works

| Job | What it proves |
| --- | --- |
| `dsp-tests` | The engine builds and passes all headless checks without JUCE. |
| `build` | VST3 + Standalone compile on Linux, macOS and Windows (AU also on macOS); the macOS AU is validated with `auval`; tests pass; zips are downloadable. |
| `screenshots` | The real editor is rendered offscreen to ten PNGs plus a demo WAV; refreshed renders are committed back to `main`. |
| `pluginval` | The Linux and Windows VST3 builds are validated by pluginval at strictness level 5. |
| `publish-latest` | Linux, macOS and Windows zips, checksums, `latest.json` and ten previews are attached to the rolling `latest` release only after the build, renders and validation succeed. |
| `verify-latest` | The published rolling assets are downloaded back and checked against `docs/`, `SHA256SUMS.txt` and `latest.json`. |
| `release` | A `v*` tag publishes a versioned GitHub Release with all platform zips, renders, checksums and a versioned `latest.json`. |
| `verify-tagged-release` | The same integrity check runs against the versioned release. |

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
  Presets.*           36 factory patches
  tests/              3,928 headless assertions

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
- preset application, uniqueness and clamping across all 36 patches;
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

CI additionally builds the plugin on three operating systems, validates the
Linux and Windows VST3 builds with pluginval
(`--strictness-level 5 --skip-gui-tests`), and validates the macOS AU with
`auval`.

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
