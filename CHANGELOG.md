# Changelog

## 5.3.0 — user presets, modulation and Scala tuning

### Play and save

- Added an **Advanced / Mod** panel (keyboard shortcut **M**) with all eight
  host-automatable modulation routes: source, destination and bipolar depth.
- Added a searchable on-disk `.cs80e` user-preset bank with save, load, rename,
  delete confirmation, import and export. Full patch state, unedited baseline and
  microtuning travel with each user preset.
- Added Scala `.scl` scales and optional `.kbm` keyboard mappings to the UI.
  Both are restored with DAW projects. Fixed Scala intervals being incorrectly
  summed, bare integer ratio handling, fractional reference frequencies,
  reference/middle key mapping and unmapped-key silence.
- Fixed a race where repeated tuning changes could overwrite the table being
  read by the audio thread. Publication now uses three pinned snapshots; the
  audio thread never acquires a lock.
- The preset LCD updates when a user preset is saved or loaded and identifies
  the personal bank rather than presenting a factory patch number.

### Verification and downloads

- Extended the headless suite to **5,255 checks** (0 failures in a local g++
  run), including concurrent tuning publication and Scala edge cases.
- Fixed an MSVC-only test compilation failure caused by a missing capture in
  the filter-response test; it was visible in the Windows Actions build log.
- The offline renderer now produces **13 screenshots**: the original ten plus
  modulation matrix, microtuning and user-preset views. CI validates each
  render, publishes them with the platform binaries and verifies release assets
  against repository renders.
- Tagged and rolling binary publication remains dependent on CI, pluginval,
  AU validation, analysis and sanitizer jobs. Real DAW/CS-80 comparisons remain
  separate validation activities, not claims made by the automated suite.

## 5.2.0 — performance console and ensemble modes

### Sound

- The CS-80 chorus/tremolo switch now has its three authentic positions.
  Chorus I uses three slower BBD taps with a narrower image; Chorus II keeps the
  wide five-tap halo; Tremolo is a 4.6 Hz stereo amplitude sweep with a 90°
  channel offset. Ensemble depth controls all three.
- Velvet Strings ships on Chorus I and Prog Organ switches to Tremolo, so the
  factory bank demonstrates the new settings.
- Added regression tests for the two chorus tap sets and the tremolo envelope:
  5,229 checks, all passing.

### Interface

- Rebuilt the panel around a warm lacquer-and-brass finish: walnut cheeks,
  brass rails, a chrome badge, ten colour-coded cards, calibration rails, slot
  screws and an engraved ivory slider cap with a back-lit value window.
- Brushed-metal knobs now carry a jewel-toned value arc, a lit pointer and a
  subtle halo; the value and legend type is larger throughout.
- Warmer, deeper preset LCD with a ghosted patch number, a darker sound library
  with stronger selection contrast, a larger dual-trace scope, and a richer
  bronze ribbon and keybed.
- The VCA card no longer clips its subtitle, and the footer reports V5.2.0.

## Previously unreleased engine work — modulation matrix and microtuning

- Eight-slot modulation matrix in the engine: LFO, velocity, channel and poly
  pressure, mod wheel, key track, filter/amp envelopes and per-note random to
  pitch, pulse width, cutoff, resonance, amplitude, pan, drive and layer level.
  Destinations are smoothed; unused slots cost nothing.
- Scala `.scl` and `.kbm` microtuning in the engine, with 12-TET as the
  bit-compatible default. Tables are published to the audio thread atomically.
- The suite grew from 4,007 to 5,176 checks, including octave-accurate pitch
  routing, pan/amplitude/cutoff routing, resonance stability, Scala parsing and
  a 432 Hz keyboard mapping that renders A3 at 216 Hz.
- **Historical note:** these were engine-only at the time of this change;
  version 5.3 adds the panel and state integration described above.

## 5.1.0 — DSP validation and reliability overhaul

### Sound

- Replaced the PolyBLEP oscillators with quarter-octave band-limited wavetable
  mip maps built at prepare time. A pulse is now the difference of two
  band-limited saw reads, so narrow pulse widths stay band-limited.
- Measured worst-case oscillator aliasing improved from −38 dB to −66 dB
  (48 kHz, extreme patches) and from −38 dB to −79 dB for a high saw. Full
  before/after tables are in `docs/DSP_VALIDATION.md`.
- The VCA curve and the output transformer now use a normalised cubic saturator
  with first-order antiderivative anti-aliasing instead of `fastTanh` at the host
  rate.
- Level changes: band-limited tables normalise differently from PolyBLEP, so
  patch peaks move by up to ~1 dB. Every factory patch still peaks below 0.94.

### Reliability

- Correct latency reporting: 8 host samples, derived from the decimator topology
  instead of reported as zero.
- Voice stealing is now genuinely tested with eight held voices and a ninth
  note. The declick hand-over is a 64-sample raised-cosine ramp with exact
  endpoints.
- Chorus and reverb no longer freeze their delay lines when the mix reaches
  zero. Wet/dry gains are smoothed per sample and the buffers are cleared once
  the tail has decayed. Re-enabling produces no stale burst.
- FIR and wavetable preparation moved off the audio thread. The test suite
  asserts zero allocations across 400 callbacks.

### Tests and tooling

- New analysis tool (`cs80_analysis`) for CPU, aliasing and latency
  measurements, with raw runs committed under `docs/benchmarks/`.
- Test suite grown from 3,928 to 4,007 checks, including filter/envelope unit
  tests, sample-rate changes, 500 rapid retriggers, effect bypass, allocation
  tracking and spectral alias gates.
- CI adds benchmark/analysis and ASan/UBSan jobs and gates both the rolling and
  versioned releases on them, plus a VERSION/CMake/tag consistency check.

### UI

- Sub-8 px labels are lifted and secondary text contrast is increased. A full
  responsive re-layout was not part of this release.

### Known gaps

- User preset management, a modulation matrix, microtuning, MPE zones and MIDI
  learn are not implemented in 5.1.
- No hardware CS-80 comparison was performed.

## 5.0.0

- 2× oversampled voice path with a 33-tap half-band decimator.
- Filter EG Depth, dual channel cards, five-tap BBD ensemble, stereo voice
  spread, output transformer.
- 36 factory patches, four-column preset library, repainted panel.

## Earlier versions

See the Git history for the 1.0–4.0 rebuilds.
