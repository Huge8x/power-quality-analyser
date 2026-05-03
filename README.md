# Power Quality Waveform Analyser

**Module:** UGMFGT-15-1 Programming for Engineers  
**Language:** C (C99)  
**Build system:** CMake

---

## What it does

Reads `power_quality_log.csv`, computes power-quality metrics for all three phases (RMS, peak-to-peak, DC offset, standard deviation, clipping count, EN 50160 compliance, bitmask status flags), prints a summary to the terminal, and writes a full report to `results.txt`.

---

## Building

### Option A – CLion (recommended)

1. Open the project folder in CLion.
2. CLion detects `CMakeLists.txt` automatically and configures the build.
3. Click **Build → Build Project** (or press Ctrl+F9 / ⌘F9).
4. The executable `power_analyser` appears in `cmake-build-debug/`.

### Option B – Command line (gcc)

```bash
gcc -std=c99 -Wall -Wextra -o power_analyser main.c waveform.c io.c -lm
```

---

## Running

```bash
./power_analyser power_quality_log.csv
```

The file `results.txt` is created in the working directory.

---

## Project structure

| File | Purpose |
|------|---------|
| `main.c` | Entry point; command-line handling; orchestration only |
| `waveform.h` | `WaveformSample` and `PhaseResult` struct definitions; analysis function declarations |
| `waveform.c` | All analysis functions: RMS, peak-to-peak, DC offset, std dev, clipping, compliance |
| `io.h` | I/O function declarations |
| `io.c` | CSV loader and report writer |
| `CMakeLists.txt` | Build configuration |

---

## Expected output (Phase A)

```
RMS           : ~229.7 V  [COMPLIANT]
Peak-to-peak  : ~650.0 V
DC offset     : ~0.000000 V
Clipped       : 20 per phase (sensor limit 324.9V)
Frequency     : 50.000 - 50.048 Hz
Power factor  : 0.956
THD           : 2.09%
```

---

## GitHub Repository

https://github.com/Huge8x/power-quality-analyser