# Changelog

## Unreleased — modulation matrix and microtuning (engine only)

- Eight-slot modulation matrix in the engine: LFO, velocity, channel and poly
  pressure, mod wheel, key track, filter/amp envelopes and per-note random to
  pitch, pulse width, cutoff, resonance, amplitude, pan, drive and layer level.
  Destinations are smoothed; unused slots cost nothing.
- Scala `.scl` and `.kbm` microtuning in the engine, with 12-TET as the
  bit-compatible default. Tables are published to the audio thread atomically.
- The suite grew from 4,007 to 5,176 checks, including octave-accurate pitch
  routing, pan/amplitude/cutoff routing, resonance stability, Scala parsing and
  a 432 Hz keyboard mapping that renders A3 at 216 Hz.
- **Not shipped yet:** neither feature has panel controls or plugin-state
  persistence. They are reachable from host automation (matrix parameters) or
  engine API only. The 5.1 panel is unchanged and no new release was published
  from this work.

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
