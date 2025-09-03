# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## Unreleased

### To Be Added
- A wrapper version of each FPS-R function that 
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
- Higher Level of Determinism, Robustness and Optimisation
    - **double precision** using 64-bit memory allocation for floats in C.
    - A new `initialize_sine_luts()` **function** must be called **exactly once at program startup** to pre-populate the sine lookup tables. Failure to do so will result in undefined behavior or fallback to less deterministic `sin()` calls."
    - **baked sine curve** with multiple levels of detail (LOD) on sample count
        - performance
            - **performed once per session** the sine function is sampled and baked at various predefined resolutions _only once_ at the beginning storing the samples as a global constant. the results will be reused through look-up at interpolation.
            - side-steps the costly sin()
            - provides deterministic values even at very small increments.
        - increased deterministic accuracy through bit-for-bit repeatability as a source of truth through the baked sine curve.
        - **The `portable_rand()` function now utilizes the highest precision baked sine curve (LUT)** for its internal sine calculations, further enhancing its bit-for-bit determinism and robustness across all platforms.
    - All **time-based integer parameters** (e.g., `minHold`, `maxHold`, `reseedInterval`, `periodA`, `periodB`, `periodSwitch`, `streamsOffset`, `quantOffsets`) are now **internally scaled by** `FPSR_INFLATION_FACTOR` within the base algorithms to match the high-resolution `int_frame` timeline. This ensures absolute, bit-for-bit determinism for all modulo and timing calculations.
    - For Quantised Switching (QS), `baseWaveFreq` and `stream2FreqMult` are **internally deflated** by `FPSR_INFLATION_FACTOR` to correctly apply frequencies to the high-resolution `int_frame` timeline, preventing underflow and maintaining deterministic oscillation.
    

### To Be Changed
- N/A

### To Be Removed
- N/A

---
## [1.0.1] - 2025-09-04

### Added
- N/A

### Changed
- FPS-R QS implementations across all languages
    - streams 1 and 2 used to output `sin()` results that were -1 to 1. This has been normalised to 0 to 1 range.
        - this range is in consistent with many other noise generators (eg, worely, simlpex, perlin).
        - -1 to 1 when gone through quantisation, will result in double the number of quantised levels than intended.
    - **_This will break output consistency with prior versions. Please take note._**

### Removed
- N/A

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

### Changed
- N/A

### Removed
- N/A

### Fixed
- N/A