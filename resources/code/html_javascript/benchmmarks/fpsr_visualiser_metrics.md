
# FPS-R Visualiser Performance Metrics

Using the FPS-R Visualizer default settings for each algorithm, the following approximate performance metrics were observed.

Frame Persistent Stateless Randomisation (FPS-R) is a novel algorithmic paradigm that produces a pseudo-random stream of discrete values with a uniform distribution and a configurable minimum hold duration. It is designed to be stateless, bidirectionally scrubbable, and performant across a wide range of platforms.

---
## PC Specifications
| Specification | Details |
| :--- | --- |
| Processor | AMD Ryzen 9 3900X 12-Core, 3.80 GHz |
| Installed RAM | 64.0 GB |
| Graphics Card | NVIDIA GeForce RTX 2080 Ti (11 GB) |
| System Type | 64-bit operating system, x64-based processor |
| Operating System | Windows 10 Home |

---
## Defining the Algorithms

**FPS-R Algorithms**

FPS-R Stacked Modulo - FPS-R SM
FPS-R Toggled Modulo - FPS-R TM
FPS-R Quantised Switching - FPS-R QS
FPS-R Bitwise Decode - FPS-R BD

**Paradigm A (Legacy)**
- **Description:** A stateful pre-calculated accumulator loop. Upon a state transition, it generates both a stochastic payload value and a random hold duration ($t_{\text{target}} = t_{\text{curr}} + \text{min\_hold} + \text{rand}() \times \text{variance}$). The system caches `next_jump_frame` in memory and performs a conditional boundary check (`current_frame >= next_jump_frame`) on every subsequent frame to determine when to recalculate. Forward-only path-dependent ($O(N)$ scrubbing penalty).
- **Parameters with Maximum Impact on Compute:**
  - `MINHOLDFRAMES` (default: 1) / `MAXHOLDFRAMES` (default: 25): Governs the frequency of re-draw cycles. Shorter average holds increase the frequency of RNG payload and duration invocations.

**Paradigm B (Legacy)**
- **Description:** A stateful continuous coin-flip loop. Generates a random payload value, then holds statically for at least `min_hold` baseline frames (Tier 1: Mandatory Baseline check, which cheaply bypasses RNG calls). Once that baseline elapses, it executes a continuous Bernoulli trial—a pseudo-random dice roll against a fixed probability threshold—on every single subsequent frame until a jump is triggered (Tier 2: The Probability Grind). Exhibits rapid geometric probability decay beyond `min_hold` and requires persistent memory locks (`last_jump_frame`, `held_value`).
- **Parameters with Maximum Impact on Compute:**
  - `jumpProbability` / threshold (default: 0.10): Directly controls the duration of the Tier 2 "probability grind" loop (evaluating `Math.random() < threshold` on consecutive frames).
  - `MINHOLDFRAMES` (default: 1): Extends the cheap Tier 1 bypass phase where RNG evaluation is skipped.

**Stateless Perlin**
- **Description:** A stateless 1D implementation of classical gradient noise evaluated using fractional Brownian motion (fBm). Computes quintic polynomial fade curves ($6t^5 - 15t^4 + 10t^3$) between integer grid coordinates hashed to 1D gradients. To achieve discrete phrased plateaus, the resulting continuous floating-point manifold is actively suppressed via uniform post-process quantization ($\lfloor \text{noise} \times \text{steps} \rfloor$). Stateless and bidirectionally scrubbable ($O(1)$ coordinate access).
- **Parameters with Maximum Impact on Compute:**
  - `Octaves` (default: 3): Linearly multiplies the number of gradient hash lookups and quintic polynomial interpolations evaluated per coordinate ($2 \times \text{octaves}$ splitmix64 hashes per frame).
  - `Quantization Steps` (default: 32) & `finalRandSwitch` (default: checked): Post-processing quantization and optional final re-hash layer.

**Stateless Worley**
- **Description:** A stateless 1D implementation of Cellular / Voronoi noise evaluated using fractional Brownian motion (fBm). Divides coordinate space into unit cells, hashes a pseudo-random feature point into each cell, and calculates Euclidean distances from the sample point across a 4-cell neighborhood stencil to extract nearest-neighbor distance metrics ($F_1$, $F_2$, or $F_2 - F_1$). Like Perlin, flat plateaus are produced by quantizing the continuous distance field.
- **Parameters with Maximum Impact on Compute:**
  - `Octaves` (default: 3): Directly multiplies neighborhood search passes ($4 \times \text{octaves}$ splitmix64 hashes per frame, twice the hash operations of Perlin).
  - `Cellular Metric` (default: F1): Determines sorting and subtraction depth during candidate neighbor comparisons.

---
## HTML Preview in Visual Studio Code
### VS Code Version Information
| Specification | Details |
| :--- | --- |
| Version | 1.99.2 (user setup) |
| Electron | 34.3.2 |
| ElectronBuildId | 11161073 |
| Chromium | 132.0.6834.210 |
| Node.js | 20.18.3 |
| V8 | 13.2.152.41-electron.0 |
| OS | Windows_NT x64 10.0.19045 |

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Paradigm A | ~2, 483 k/s |
| Legacy Paradigm B | ~2, 298 k/s |
| Statelss Perlin | ~870 k/s |
| Stateless Worley | ~600 k/s |
| FPS-R SM | ~1, 385 k/s |
| FPS-R TM | ~1, 834 k/s |
| FPS-R QS | ~872 k/s |
| FPS-R BD (default 3 streams, blocksize 64) | ~350 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~590 k/s |
|   | 60 | ~580 k/s |
| 2 | 30 | ~444 k/s |
|   | 60 | ~446 k/s |
| 3 | 30 | ~340 k/s |
|   | 60 | ~333 k/s |
| 4 | 30 | ~296 k/s |
|   | 60 | ~289 k/s |
| 7 | 30 | ~179 k/s |
|   | 60 | ~151 k/s |

---
## Chrome Browser 
### Chrome Version Information
Version 151.0.7922.34 (Official Build) (64-bit)

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Paradigm A | ~2, 953 k/s |
| Legacy Paradigm B | ~2, 587 k/s |
| Statelss Perlin | ~1,016 k/s |
| Stateless Worley | ~691 k/s |
| FPS-R SM | ~1, 747 k/s |
| FPS-R TM | ~2, 135 k/s |
| FPS-R QS | ~956 k/s |
| FPS-R BD (default 3 streams, blocksize 64) | ~390 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~705 k/s |
|   | 60 | ~700 k/s |
| 2 | 30 | ~505 k/s |
|   | 60 | ~504 k/s |
| 3 | 30 | ~385 k/s |
|   | 60 | ~388 k/s |
| 4 | 30 | ~325 k/s |
|   | 60 | ~324 k/s |
| 7 | 30 | ~192 k/s |
|   | 60 | ~179 k/s |

---
## Firefox Browser
### Firefox Version Information
Version 153.0.3 (64-bit)

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Paradigm A | ~7, 710 k/s |
| Legacy Paradigm B | ~7, 300 k/s |
| Statelss Perlin | ~405 k/s |
| Stateless Worley | ~226 k/s |
| FPS-R SM | ~1, 075 k/s |
| FPS-R TM | ~1, 826 k/s |
| FPS-R QS | ~678 k/s |
| FPS-R BD (default 3 streams, blocksize 64) | ~348 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~646 k/s |
|   | 60 | ~605 k/s |
| 2 | 30 | ~435 k/s |
|   | 60 | ~440 k/s |
| 3 | 30 | ~371 k/s |
|   | 60 | ~352 k/s |
| 4 | 30 | ~309 k/s |
|   | 60 | ~301 k/s |
| 7 | 30 | ~178 k/s |
|   | 60 | ~170 k/s |

---
## Android Phone Specifications
| Specification | Details |
| :--- | --- |
| Samsung | Galaxy S23 Ultra 5G |
| Model | SM-S918B/DS |
| Processor | Qualcomm Snapdragon 8 Gen 2 |
| Memory | 12 GB |
| System Type | 64-bit operating system, arm64-based processor |
| Android Version | 16 |

## Android Chrome Browser
### Chrome Version Information
Version 151.0.7922.83

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Paradigm A | ~3, 010 k/s |$$
| Legacy Paradigm B | ~2, 724 k/s |
| Statelss Perlin | ~1, 076 k/s |
| Stateless Worley | ~725 k/s |
| FPS-R SM | ~1, 873 k/s |
| FPS-R TM | ~2, 296 k/s |
| FPS-R QS | ~1, 125 k/s |
| FPS-R BD (default 3 streams, blocksize 64) | ~423 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~830 k/s |
|   | 60 | ~820 k/s |
| 2 | 30 | ~680 k/s |
|   | 60 | ~682 k/s |
| 3 | 30 | ~540 k/s |
|   | 60 | ~548 k/s |
| 4 | 30 | ~375 k/s |
|   | 60 | ~390 k/s |
| 7 | 30 | ~210 k/s |
|   | 60 | ~200 k/s |

## A Consolidated Table of Approximate Performance Metrics (k/s) Across Platforms
| Algorithm | VS Code Preview | Chrome | Firefox | Android Chrome |
| :--- | ---: | ---: | ---: | ---: |
| Legacy Paradigm A | ~2,483 | ~2,953 | ~7,710 | ~3,010 |
| Legacy Paradigm B | ~2,298 | ~2,587 | ~7,300 | ~2,724 |
| Stateless Perlin | ~870 | ~1,016 | ~405 | ~1,076 |
| Stateless Worley | ~600 | ~691 | ~226 | ~725 |
| FPS-R SM | ~1,385 | ~1,747 | ~1,092 | ~1,873 |
| FPS-R TM | ~1,834 | ~2,135 | ~1,838 | ~2,296 |
| FPS-R QS | ~872 | ~956 | ~678 | ~1,125 |
| FPS-R BD (default 3 streams, blocksize 64) | ~350 | ~390 | ~328 | ~423 |

## Averaged Performance Metrics (k/s) Across Platforms
| Algorithm | Average Performance |
| :--- | ---: | 
| Legacy Paradigm A | ~4,039.00 |
| Legacy Paradigm B | ~3,727.25 |
| Stateless Perlin | ~841.75 |
| Stateless Worley | ~560.50 |
| FPS-R SM | ~1,524.25 |
| FPS-R TM | ~2,025.75 |
| FPS-R QS | ~907.75 |
| FPS-R BD | ~372.75 |

## Performance Relative to Legacy Paradigm A Across Platforms
Below, each result is normalized to **Legacy Paradigm A on the same platform**:
$$
R_{i,j}=\frac{\text{performance of algorithm }i\text{ on platform }j}
{\text{Legacy A performance on platform }j}
$$
Thus, Legacy A is \(1.000\), or \(100\%\), on every platform. A value of \(0.75\) means the algorithm achieves 75% of Legacy A’s throughput on that platform.

| Algorithm | VS Code Preview | Chrome | Firefox | Android Chrome | Median ratio |
|---|---:|---:|---:|---:|---:|
| Legacy Paradigm A | 1.000 / 100.0% | 1.000 / 100.0% | 1.000 / 100.0% | 1.000 / 100.0% | **1.000 / 100.0%** |
| Legacy Paradigm B | 0.926 / 92.6% | 0.876 / 87.6% | 0.947 / 94.7% | 0.905 / 90.5% | **0.915 / 91.5%** |
| Stateless Perlin | 0.350 / 35.0% | 0.344 / 34.4% | 0.053 / 5.3% | 0.358 / 35.8% | **0.347 / 34.7%** |
| Stateless Worley | 0.242 / 24.2% | 0.234 / 23.4% | 0.029 / 2.9% | 0.241 / 24.1% | **0.237 / 23.7%** |
| FPS-R SM | 0.557 / 55.7% | 0.591 / 59.1% | 0.142 / 14.2% | 0.622 / 62.2% | **0.574 / 57.4%** |
| FPS-R TM | 0.739 / 73.9% | 0.723 / 72.3% | 0.238 / 23.8% | 0.763 / 76.3% | **0.731 / 73.1%** |
| FPS-R QS | 0.351 / 35.1% | 0.324 / 32.4% | 0.088 / 8.8% | 0.374 / 37.4% | **0.337 / 33.7%** |
| FPS-R BD | 0.141 / 14.1% | 0.132 / 13.2% | 0.043 / 4.3% | 0.141 / 14.1% | **0.136 / 13.6%** |

The median-normalized ranking is therefore:
1. Legacy Paradigm A — **100.0%**
2. Legacy Paradigm B — **91.5%**
3. FPS-R TM — **73.1%**
4. FPS-R SM — **57.4%**
5. Stateless Perlin — **34.7%**
6. FPS-R QS — **33.7%**
7. Stateless Worley — **23.7%**
8. FPS-R BD — **13.6%**

The most striking result is that **FPS-R TM is consistently strong outside Firefox**:
- VS Code Preview: 73.9% of Legacy A
- Chrome: 72.3%
- Android Chrome: 76.3%

Firefox is the exception at 23.8%. This suggests that FPS-R TM has a relatively stable cross-platform relationship with Legacy A, while Firefox’s implementation strongly favors the legacy stateful loop.

For a generalized intuition, you could summarize your FPS-R variants as:
- **FPS-R TM:** roughly three-quarters as fast as Legacy A on most platforms
- **FPS-R SM:** roughly three-fifths as fast
- **FPS-R QS:** roughly one-third as fast
- **FPS-R BD:** roughly one-seventh as fast

The median ratio is particularly useful here because it describes the “typical platform relationship” without allowing Firefox’s Legacy A result to dominate the conclusion.