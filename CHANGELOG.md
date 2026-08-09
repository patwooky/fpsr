# FPS-R Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## Unreleased

## [3.0.2] - 2026-08-07
### Changed
- Algorithm Analysis Jupyter Notebook (`resources\code\data_analysis\fpsr_algoAnalysis.ipynb`)
    - Updated the FPSR analysis notebook to print per-sequence jump counts and jump frequency percentages after held-step calculations (e.g., `random.random()`, `Portable-Rand`, and FPS-R variants). This makes the notebook output easier to compare across generators without relying only on plots.
- Visualizer (`resources/code/html_javascript/fpsr_demo.html`)
    - Reworked the FPS-R demo controls by moving the transport bar up under the canvas and adding a dedicated speed/performance panel. The animation loop now supports adjustable playback speed, an unconstrained max mode, and a rolling evaluations-per-second display so users can both navigate and benchmark the demo more effectively.
    - Added a 'Legacy Stateful (Comparison)' tab to the FPSR demo that implements the classic hold-and-jump stateful accumulator loop. Includes UI panel with minHoldFrames and jumpProbability controls, disables reverse playback and HPQ time controls when active (since stateful memory cannot step backward), and resets legacy state variables on param changes.
    - Updated the demo playback loop so MAX speed mode now runs with a time-budgeted compute window per frame (instead of a fixed step count), improving uncapped playback responsiveness while keeping standard slider-based stepping behavior unchanged.
    - Changed the default BD algorithm parameters in the demo to a more balanced configuration (streamCount=2, blockSize=64) for better BD performance that still demonstrates its unique jump/twitch behavior. The previous streamCount=7 was unnecessarily heavy for the demo making BD appear much slower than the other algorithms.
- FPS-R Visualiser Performance Metrics(`resources\code\html_javascript\benchmmarks\fpsr_visualiser_metrics.md`)
    - Documented observed FPS-R visualizer throughput metrics in a new benchmark markdown file, including Bitwise Decode stream/blocksize breakdowns. 

## [3.0.1] - 2025-01-14

### Changed
- `resources/readme/FPSR_Unifying_Theory.md`
    - Expanded the theoretical foundation with concrete, multi-level analogies that make abstract concepts more accessible
    - Strengthened the argument that "hold-then-break" is the fundamental, recursive signature of how work, thought, and all biological activity unfolds
    - Enhanced the connection between abstract theory and observable reality through biological evidence


## [3.0.0] - 2025-11-11

### Major Release Summary
This major release represents a significant evolution of the FPS-R framework from v1.0.0 to v3.0.0, introducing groundbreaking features and comprehensive improvements that fundamentally expand the framework's capabilities.

### Highlights

#### 1. **New FPS-R Bitwise Decode (BD) Algorithm** 
The crown jewel of this release - a fourth algorithm that uses bit-flipping mechanics to create the most expressive FPS-R algorithm to date. BD offers unparalleled combinatory possibilities through its implemented parameters, producing quirky jumps and twitches that make it ideal for highly dynamic procedural generation.
- Implemented across C, Python, and Jupyter Notebook environments
- Features in the online visualizer for interactive exploration
- Achieves even output distribution comparable to SM and TM algorithms
- Performance: 3.0-6.2 seconds for 100,000 frames (depending on stream count)

#### 2. **Wrapper Versions for Rich Analytical Output** 
Complete wrapper implementations in both C and Python that transform FPS-R from a pure generator into a comprehensive analytical tool:
- **`has_changed`**: Binary indicator when output values jump
- **`hold_progress`**: Normalized progress (0.0-1.0) through current hold duration
- **`last_changed_frame`** and **`next_changed_frame`**: Temporal anchors for hold periods
- **Level-of-Detail (LOD) System**: Three tiers (0, 1, 2) for performance vs. detail trade-offs
- **Hierarchical Phrased Quantisation (HPQ)**: Revolutionary time-scaling with frame_multiplier
  - Two-tier stretch logic: "Tape Varispeed" (anchor) + "Telescopic Extension" (generative)
  - Intuitive playback speed convention: <1.0 for slow-motion, >1.0 for fast-motion
  - Segment block length control for time-stretched content

#### 3. **Online HTML/JavaScript Visualizer** 
Interactive web-based demonstration of all four FPS-R algorithms, hosted on GitHub Pages:
- Real-time visualization of SM, TM, QS, and BD algorithms
- Global HPQ Time Controls with playback speed manipulation
- Snap-to-value slider logic for precise control
- Deep-copy parameter handling with structuredClone() for correct BigInt support
- [Explore the live visualizer](https://patwooky.github.io/fpsr/resources/code/html_javascript/fpsr_demo.html)

#### 4. **FPS-R Profile Output Analysis** 
Comprehensive data analysis tools and documentation that reveal the "fingerprint" of each algorithm:
- **Jupyter Notebook** (`fpsr_algoAnalysis.ipynb`): Interactive analysis of all algorithms across 400+ time-steps
- **Visual Documentation**: 
  - Output value graphs (blue lines with red jump indicators)
  - Held steps graphs (time axis analysis)
  - Distribution graphs showing output characteristics
- **Performance Metrics**: Timing comparisons for 100,000 frames across all algorithms
- **README Integration**: Detailed explanations with embedded graphs for immediate understanding

### Additional Major Improvements

#### Enhanced Determinism and Cross-Platform Parity
- **Thread-safe sine lookup tables (LUTs)**: Platform-agnostic initialization for consistent results
- **Bit-for-bit parity**: C and Python implementations produce identical outputs
- **Flexible sine level-of-detail system**: Multiple LOD levels for performance optimization
- **FPSR_INFLATION_FACTOR**: Internal scaling for absolute determinism in timing calculations
- **Comprehensive overflow checks**: Robust memory allocation and bounds checking

#### Breaking Changes and Refinements
- **SM/TM Output Change (v2.0.4)**: Removed 100000ULL multiplier for theoretical purity in hashing
- **QS Stream Normalization (v1.0.1)**: Outputs normalized to 0-1 range (from -1 to 1) for consistency
- **Inverted frame_multiplier Convention (v2.0.7)**: Matches intuitive "playback speed" expectations

#### License Change
- **Apache 2.0 License**: Changed from MIT License to facilitate easier collaboration and adoption

### Changed
All changes from v2.0.0 through v2.0.9 are consolidated in this major release. See individual version entries below for detailed change logs.

### Commit Hash
Release commit: `64b6fa1`

---
## [2.0.9] - 2025-11-06
## Changed
- `code/data_analysis/fpsr_algoAnalysis.ipynb`
    - Refactored:
        - Made helper functions deterministic,
        - Added sine LUTs,
        - Updated all FPS-R base algorithm implementations for clarity and portability.
    - Added FPS-R Bitwise Decode (BD) algorithm
    - enhanced Quantised Switching (QS) logic. 
    - Improved code comments and structure for maintainability.
    - restored missing comment blocks from the main `fpsr_algorithms_wrap.py` file.
- `fpsr_algorithms_wrap_reference.c`
    - `fpsr_xx_get_details()` functions have abbreviated comments like `... algo params ...`. Replaced it with meaningful and parameter descriptions.
- Expanded and clarified the README with detailed explanations and new images for FPS-R algorithm output analysis. 

## [2.0.8] - 2025-11-05
## Changed
- `fpsr_algorithms_base_reference.c` and `fpsr_algorithms_wrap_reference.c`
    - Add thread-safe sine lookup tables with platform-specific initialization
    - Align Python FPSR algorithms with C reference implementation for deterministic parity
    - Fix quantization seeding logic in fpsr_qs to preserve level information
    - Add comprehensive overflow checks in fpsr_bd memory allocation
    - Implement flexible sine level-of-detail Look-up Table system for fpsr_qs
    - Remove incorrect seed multipliers in fpsr_sm and fpsr_tm to match C canonical logic
    - Update test parameters in main() to match C reference for direct comparison
- `fpsr_algorithms.py`
    - Add thread-safe sine lookup tables (LUTs) with platform-agnostic initialization
    - Rename fpsr_sm and fpsr_tm to fpsr_sm_base and fpsr_tm_base for C reference alignment
    - Implement flexible sine level-of-detail system in fpsr_qs with sine_lod_level parameter
    - Update hashing logic in fpsr_qs to use floor-based multiplier instead of struct.pack
    - Fix quantization seeding and add overflow guards in fpsr_qs

## Added
- `fpsr_algorithms_wrap.py`
    - ported the `wrap` version over from C

---
## [2.0.7] - 2025-11-04
## Changed
- `fpsr_algorithms_wrap_reference.c` 
    - **Major Refactor: Replaced "Fractal Coherence" with "Hierarchical Phrased Quantisation" (HPQ)**: The "fractal zoom" logic from `[2.0.6]` was found to be musically incoherent. It caused a chaotic "flicker" of values when time was stretched, rather than "stretching" the phrase. It has been completely replaced by the new, constructive HPQ algorithm.
    - **Added `seg_block_length` Parameter**: All _get_details functions now accept an int seg_block_length. This new parameter defines a "runway" or "chunk size" for quantizing time.
    - **New Two-Tier Stretch Logic (HPQ)**: The new algorithm for slow-motion (`frame_multiplier < 1.0`) is a two-mode system:
        - **Tier 1: "Tape Varispeed" (Anchor Block)**: For simple stretches (`segment_index == 0`), the logic now **repeats the `master_frame`'s value**. This provides the intuitive, musically-preserve "tape stretch" effect that was previously missing.
        - **Tier 2: "Telescopic Extension" (Generative Blocks)**: For extreme stretches (`segment_index > 0`), the algorithm generates a unique `gap_seed` for each new "runway" block. It then calls the `_base` function using the `local_progress_in_segment` as the frame input, generating new, coherent, and fully-phrased content.
    - **Inverted frame_multiplier Convention**: The `frame_multiplier` logic has been inverted to match the common, intuitive "playback speed" convention:
        - `frame_multiplier < 1.0` is now **Slow-Motion (Time Stretch)**.
        - `frame_multiplier > 1.0` is now **Fast-Motion (Time Compression)**.
    - **Simplified `LOD 2` Logic**: As a direct result of the HPQ refactor, the `hold_progress` calculation is no longer based on complex scaled math. It is now a simple, robust, linear percentage calculated purely on the "Application Timeline" (`(frame - last) / (next - last)`).
    - **Removed `_get_hierarchical_seed`**: This function, central to the old "fractal" model, is no longer needed by HPQ and has been deleted.
- `fpsr_demo.html`
    - **Propagated HPQ Frame_Multilpier logic to HTML visualiser**: HTML visualiser now features `Global HPQ Time Controls` section that features
        - **Playback speed `frame_multiplier`**: The speed that the "Application Timeline" (visualizer) plays back the "Content Timeline" (algorithm). Values `< 1.0` are Slow-Motion (stretch), and values `> 1.0` are Fast-Motion (compress).
        - **Segment Length `seg_block_length`**: The "runway" size for time-stretched segments. This defines the duration (in application frames) of the Tier 1 "Tape Varispeed" (hold) before the Tier 2 "Generative Extension" (new phrase) logic is triggered.
    - **Added UI features for `frame_multiplier`**:
        - Implemented "snap-to-value" logic for the slider, making it easy to select common increments like `0.25`, `0.5`, `1.0`, etc.
        - Added a "1.0x" reset button for instantly returning to normal speed.
    - Fixed `BigInt` error: Replaced the `JSON.stringify` copy method (which crashes on `BigInt`) with `structuredClone()` to correctly deep-copy parameters for the Tier 2 generative logic.

---
## [2.0.6] - 2025-10-30
### Changed
- `fpsr_algorithms_wrap_reference.c` 
    - **Added `frame_multiplier` and Hierarchical Time:** All `_get_details` wrappers now support time scaling.
    - Instead of repeating values, slowing down (`frame_multiplier > 1.0`) now "zooms in," procedurally generating new, consistent detail in the gaps between the original frames.
    - `LOD 2` search logic was updated to use recursive calls, allowing it to find change-points on this new, non-linear timeline.

---
## [2.0.5] - 2025-10-26
### Added
- A new html visualiser of the four FPS-R algorithms! Check it out here! [FPS-R Algorithms Visualiser on GitHub Pages](https://patwooky.github.io/fpsr/resources/code/html_javascript/fpsr_demo.html) ← interact with the page
[FPS-R Algorithms Visualiser in this Repo](./resources/code/html_javascript/fpsr_demo.html) ← view the code

### Changed
- `fpsr_algorithms_wrap_reference.c` 
    - moved to `./resources/code/C_reference` 
    - renamed to `fpsr_algo_wrap_reference.c`
    - the wrapped versions are working now, including the newly included BD.


---
## [2.0.4] - 2025-10-20
### Changed
- **Breaking Change**:
Changed the SM and TM output. In previous versions, the final stage where the seed was multiplied by a very large number (`100000ULL`) before being passed to `portable_rand_u64()`. This number is now removed, and this final number would be directly passed to `portable_rand_u64()`. This improves the theoretical purity of the hashing step.

---
## [2.0.3] - 2025-10-19
### Added
- an exciting addition of a **_fourth_** algorithm to the FPS-R framework, the **FPS-R: Bitwise Decode**.
- Updated Development Journal on the origins. [Read Here](./resources/readme/FPSR_Origins_Journal_Reflections.md#a-fourth-algorithm---deeper-into-the-bits-bitwise-decode)
- Updated reference C (`fpsr_algorithms_reference.c`) code, and Python code (`fpsr_algorithms.py`)

---
## [2.0.2] - 2025-09-21
### Added
- an English podcast style introduction to FPS-R

### Changed
- updated `README.md`, `README-CH.md`
- the wrapper functions are not working yet. C and Houdini `_wrap` have been moved to `./resources/code/_WIP`

---
## [2.0.1] - 2025-09-14
### Added 
- A wrapper version of each FPS-R function (SM, TM, QS):
    - enables rich analytic information on function output:
        - `has_changed` checking with the value output of previous frame, returning `1` or `True` if value has changed or "jumped"
        - `hold_progress` Normalized progress (0.0 to 1.0) through the current hold duration.
        - `last_changed_frame` and `next_changed_frame` The frame number where the current hold period began or will jump, respectively.
        - QS output
            - `randStreams[]` output of streams 1 and 2
            - `selected_stream` the index of the selected stream in `randStreams` array
    - **Level-of-Detail (LOD)**
        - **0** - directly call the existing "base" algorithms
        - **1** - gets the `has_changed` by checking the output has jumped from the previous time step
        - **2** - getting the full range of rich outputs stated above
    - **time-scaling** allows a **dynamic scaling of the algorithm's internal timeline** via the frame_multiplier argument, enabling tempo changes without altering the core rhythm. 
- Higher Level of Bit-for-Bit Determinism, Robustness and Optimisation
    - **double precision** using 64-bit memory allocation for floats in C.
    - A new `initialize_sine_luts()` **function** that is called once at program startup to pre-populate the sine lookup tables. Failure to do so will result in undefined behavior or fallback to less deterministic `sin()` calls.
    - **baked sine curve** with multiple levels of detail (LOD) on sample count.
        - performance
            - **performed once per session** the sine function is sampled and baked at various predefined resolutions _only once_ at the beginning storing the samples as a global constant. the results will be reused through look-up at interpolation.
            - side-steps the costly sin()
            - provides deterministic values even at very small increments.
        - increased deterministic accuracy through bit-for-bit repeatability as a source of truth through the baked sine curve.
        - **The `portable_rand()` function now utilizes the highest precision baked sine curve (LUT)** for its internal sine calculations, further enhancing its bit-for-bit determinism and robustness across all platforms.
    - All **time-based integer parameters** (e.g., `minHold`, `maxHold`, `reseedInterval`, `periodA`, `periodB`, `periodSwitch`, `streamsOffset`, `quantOffsets`) are now **internally scaled by** `FPSR_INFLATION_FACTOR` within the base algorithms to match the high-resolution `int_frame` timeline. This ensures absolute, bit-for-bit determinism for all modulo and timing calculations.
    - For Quantised Switching (QS), `baseWaveFreq` and `stream2FreqMult` are **internally deflated** by `FPSR_INFLATION_FACTOR` to correctly apply frequencies to the high-resolution `int_frame` timeline, preventing underflow and maintaining deterministic oscillation.
    - currently only available for C (canonical reference). Will update the ported versions progressively.

---
## [2.0.0] - 2025-09-13
### Added 
- `(root)/resources/code/data_analysis/fpsr_algoAnalysis.ipynb`, 
    - a jupyter notebook that provides the following analysis:
        - output of each algorithm, profiling each algorithm's "fingerprint" across 400 time-steps.
        - the distribution of output values over incremental steps
        - a "held-frames" graph that describes the number of held frames every time the output values jump.

### Changed
- Changed the licensing to Apache 2.0 License. Makes it easier for collaboration and adoption.
- updated all documentation and code to reflect the new Apache 2.0 License.


---
## [1.0.2] - 2025-09-05

### Added
- Unreal Engine 5.6 Project File. 
    - contains 1 level and 3 shader, 3 material intances for the three FPS-R algorithms: SM, TM and QS.

---
## [1.0.1] - 2025-09-04

### Changed
- FPS-R QS implementations across all languages
    - streams 1 and 2 used to output `sin()` results that were -1 to 1. This has been normalised to 0 to 1 range.
        - this range is in consistent with many other noise generators (eg, worely, simlpex, perlin).
        - -1 to 1 when gone through quantisation, will result in double the number of quantised levels than intended.
    - **_This will break output consistency with prior versions. Please take note._**

### Fixed
- Maya Mel code implementation was using C-style casts 
    eg `(int)(3.33 + 5.0)`
    this has been changed to `int(3.33 + 5.0)`

---
## [1.0.0] - 2025-08-14

### Added
- Initial Release v1.0.0
- MIT Software License
- Initial C reference implementations for the FPS-R algorithms (SM, TM, QS).
    Also including these ports:
    - GLSL
    - Python, (also Jupyter Notebook)
    - WebGL
    - Houdini Vex
    - Maya Mel
    - After Effects (Javascript)
- Comprehensive documentation including a manifesto (`README.md`), applications guide (`FPSR_Applications.md`), and unifying theory (`FPSR_Unifying_Theory.md`).
- A portable pseudo-random number generator (`portable_rand`) to ensure deterministic results across different platforms.

---
<!-- entry template -->
## [1.0.0] - yyyy-mm-dd

### Added
- N/A

### Changed
- N/A

### Removed
- N/A

### Fixed
- N/A