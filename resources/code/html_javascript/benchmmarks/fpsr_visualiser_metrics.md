
# FPS-R Visualiser Performance Metrics

Using the FPS-R Visualizer default settings for each algorithm, the following approximate performance metrics were observed.

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
### Paradigm A (Legacy)
- **Description:** A stateful pre-calculated accumulator loop. Upon a state transition, it generates both a stochastic payload value and a random hold duration ($t_{\text{target}} = t_{\text{curr}} + \text{min\_hold} + \text{rand}() \times \text{variance}$). The system caches `next_jump_frame` in memory and performs a conditional boundary check (`current_frame >= next_jump_frame`) on every subsequent frame to determine when to recalculate. Forward-only path-dependent ($O(N)$ scrubbing penalty).
- **Parameters with Maximum Impact on Compute:**
  - `MINHOLDFRAMES` (default: 1) / `MAXHOLDFRAMES` (default: 25): Governs the frequency of re-draw cycles. Shorter average holds increase the frequency of RNG payload and duration invocations.

### Paradigm B (Legacy)
- **Description:** A stateful continuous coin-flip loop. Generates a random payload value, then holds statically for at least `min_hold` baseline frames (Tier 1: Mandatory Baseline check, which cheaply bypasses RNG calls). Once that baseline elapses, it executes a continuous Bernoulli trial—a pseudo-random dice roll against a fixed probability threshold—on every single subsequent frame until a jump is triggered (Tier 2: The Probability Grind). Exhibits rapid geometric probability decay beyond `min_hold` and requires persistent memory locks (`last_jump_frame`, `held_value`).
- **Parameters with Maximum Impact on Compute:**
  - `jumpProbability` / threshold (default: 0.10): Directly controls the duration of the Tier 2 "probability grind" loop (evaluating `Math.random() < threshold` on consecutive frames).
  - `MINHOLDFRAMES` (default: 1): Extends the cheap Tier 1 bypass phase where RNG evaluation is skipped.

### Stateless Perlin
- **Description:** A stateless 1D implementation of classical gradient noise evaluated using fractional Brownian motion (fBm). Computes quintic polynomial fade curves ($6t^5 - 15t^4 + 10t^3$) between integer grid coordinates hashed to 1D gradients. To achieve discrete phrased plateaus, the resulting continuous floating-point manifold is actively suppressed via uniform post-process quantization ($\lfloor \text{noise} \times \text{steps} \rfloor$). Stateless and bidirectionally scrubbable ($O(1)$ coordinate access).
- **Parameters with Maximum Impact on Compute:**
  - `Octaves` (default: 3): Linearly multiplies the number of gradient hash lookups and quintic polynomial interpolations evaluated per coordinate ($2 \times \text{octaves}$ splitmix64 hashes per frame).
  - `Quantization Steps` (default: 32) & `finalRandSwitch` (default: checked): Post-processing quantization and optional final re-hash layer.

### Stateless Worley
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
Legacy Paradigm A | ~2, 430 k/s |
Legacy Paradigm B | ~2, 320 k/s |
Statelss Perlin | ~870 k/s |
Stateless Worley | ~600 k/s |
Stacked Modulo | ~1, 400 k/s |
Toggled Modulo | ~1, 740 k/s |
Quantised Switching | ~880 k/s |
Bitwise Decode (default 3 streams, blocksize 64) | ~333 k/s |

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
| Legacy Paradigm B | ~2, 800 k/s |
| Stacked Modulo | ~1, 580 k/s |
| Toggled Modulo | ~2, 130 k/s |
| Quantised Switching | ~920 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~388 k/s |

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
| Legacy Paradigm B | ~7, 600 k/s |
| Stacked Modulo | ~1, 060 k/s |
| Toggled Modulo | ~1, 750 k/s |
| Quantised Switching | ~660 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~352 k/s |

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
| Legacy Paradigm B | ~2, 930 k/s |
| Stacked Modulo | ~1, 830 k/s |
| Toggled Modulo | ~2, 280 k/s |
| Quantised Switching | ~1, 080 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~548 k/s |

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
| Legacy Paradigm B | 2,300 | 2,800 | 7,600 | 2,930 |
| Stacked Modulo | 1,400 | 1,580 | 1,060 | 1,830 |
| Toggled Modulo | 1,740 | 2,130 | 1,750 | 2,280 |
| Quantised Switching | 880 | 920 | 660 | 1,080 |
| Bitwise Decode (3 streams, blocksize 64) | 333 | 388 | 352 | 548 |
