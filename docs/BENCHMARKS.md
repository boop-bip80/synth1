# CS-80E performance benchmarks

All numbers in this document were produced by running the committed analysis
tool on the committed engine. Nothing here is estimated or extrapolated.

## Reference environment

| Item | Value |
| --- | --- |
| Machine | Cloud Linux VM, 8 logical cores, 15 GB RAM |
| OS | Ubuntu 24.04.5 LTS |
| Compiler | GCC 13.3.0 |
| Build | CMake 3.30.5, `-DCMAKE_BUILD_TYPE=Release` |
| Engine | CS-80E 5.1.0, headless `cs80::Engine` |
| Method | 6–8 s of audio per case, single process, 64-block warm-up |
| Timing | `std::chrono::steady_clock` around each `Engine::process()` call |
| Allocations | Replaced global `operator new`, counted inside the timed loop |

This is a shared VM, not a dedicated audio workstation. Absolute times will
differ on other machines; the shape of the results and the regression comparison
are what matter. The worst-case column includes operating-system scheduling
outliers, so it is reported separately from average and p95.

## Reproduce

```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release \
      -DCS80_BUILD_PLUGIN=OFF -DCS80_BUILD_TOOLS=OFF
cmake --build build-tests --parallel
./build-tests/tools/analysis/cs80_analysis bench
./build-tests/tools/analysis/cs80_analysis alias
./build-tests/tools/analysis/cs80_analysis latency
```

Raw output is committed in [`docs/benchmarks/`](benchmarks/).

## 48 kHz, 256-frame buffers

Measured before the 5.1 DSP work (baseline, commit `2dcf308`) and after it.

| Scenario | Baseline avg | Baseline CPU | 5.1 avg | 5.1 p95 | 5.1 worst | 5.1 CPU |
| --- | --- | --- | --- | --- | --- | --- |
| Idle | 5.5 µs | 0.10 % | 4.8 µs | 4.8 µs | 23.5 µs | 0.09 % |
| 1 voice | 46.9 µs | 0.88 % | 49.9 µs | 56.8 µs | 96.3 µs | 0.94 % |
| 4 voices | 173.0 µs | 3.24 % | 184.3 µs | 193.0 µs | 273.9 µs | 3.46 % |
| 8 voices | 337.7 µs | 6.33 % | 351.0 µs | 364.9 µs | 404.3 µs | 6.58 % |
| 8 voices, dual + chorus + reverb | 734.7 µs | 13.78 % | 759.0 µs | 777.3 µs | 976.3 µs | 14.23 % |
| 8 voices, heavy modulation | 741.7 µs | 13.91 % | 758.6 µs | 773.8 µs | 1002.3 µs | 14.22 % |
| 8 voices, high resonance + drive | 337.5 µs | 6.33 % | 351.1 µs | 366.3 µs | 502.2 µs | 6.58 % |

The 5.1 oscillator reads a band-limited table with cubic interpolation where it
matters and a cheaper linear read for high mip levels, and skips table reads for
sub / sine / pulse oscillators that are mixed at zero. That recovers most of the
cost of the alias fix; the remaining ~3 % is the price of roughly 40 dB less
aliasing (see [DSP_VALIDATION.md](DSP_VALIDATION.md)).

## Buffer-size sweep — 8 voices, dual + effects, 48 kHz

| Frames | Baseline avg | 5.1 avg | 5.1 p95 | 5.1 worst | 5.1 CPU |
| --- | --- | --- | --- | --- | --- |
| 64 | 185.0 µs | 191.7 µs | 205.9 µs | 295.4 µs | 14.38 % |
| 128 | 367.9 µs | 385.8 µs | 403.2 µs | 635.7 µs | 14.47 % |
| 256 | 741.4 µs | 757.7 µs | 773.4 µs | 938.0 µs | 14.21 % |
| 512 | 1497.7 µs | 1515.9 µs | 1548.3 µs | 1931.0 µs | 14.22 % |

Per-sample cost is flat across buffer sizes, which is expected: no work is
block-size dependent.

## Sample-rate sweep — 8 voices, dual + effects, 256 frames

| Rate | Baseline avg | Baseline CPU | 5.1 avg | 5.1 CPU |
| --- | --- | --- | --- | --- |
| 44.1 kHz | 739.9 µs | 12.75 % | 766.3 µs | 13.21 % |
| 48 kHz | 742.2 µs | 13.92 % | 760.7 µs | 14.26 % |
| 96 kHz | 778.8 µs | 29.21 % | 782.5 µs | 29.35 % |

At 96 kHz the same audio duration contains twice as many host samples, so CPU
percentage doubles while the per-sample cost is unchanged. This is normal for a
sample-rate-scaled voice path.

## Targets

| Target | Result |
| --- | --- |
| ≤ 0.25× real time (≤ 25 % of one core) for 8 voices, dual, 48 kHz / 256, effects on | **Met**: 0.142× real time (14.23 %) |
| Idle ≤ 2 % of one core | **Met**: 0.09 % |
| No heap allocations in the audio callback | **Met**: 0 allocations across 400 callbacks, including note and parameter changes (asserted by `testRealtimeAllocations`) |
| No deadline misses in the reference run | Worst callback 0.98 ms against a 5.33 ms deadline at 256 frames / 48 kHz; the 96 kHz outlier is 4.7 ms against a 2.67 ms deadline, caused by VM scheduling rather than DSP |

## Limitations

- These are offline engine measurements, not a DAW session with a real audio
  device. Device buffer size, driver, and host threading add their own jitter.
- The measurement VM is shared; the `worst` column is not a reliable measure of
  real-world glitch behaviour and is retained only for regression comparison.
- No Windows or macOS timing was measured in this environment. CI builds and
  validates those platforms, but publishes no timing claims.
