# FPS-R Evaluation & Visualisation Lab: User & Workflow Guide

The **FPS-R Evaluation & Visualisation Lab** (`fpsr_demo.html`) is an interactive workbench designed for:
1. Visually diagnosing structural and phase behaviors across FPS-R algorithms, legacy state machines, and continuous spatial noise models.
2. Generating deterministic, frame-accurate test vectors for cross-platform and cross-language parity validation.
3. Live throughput profiling across different Levels of Detail (LOD 0, 1, 2).

---

## 1. Transport & Buffer Controls Overview

| Control | Type | Description |
| :--- | :--- | :--- |
| **Reset (New Seeds)** | Button | Re-seeds all parameters with new randomized offsets, resets rewind frame to 0, resets LOD to 0, and restores default time scale. |
| **Rewind** | Button + Numeric Input | Jumps the timeline to the frame specified in the adjacent input box. Resets the internal history buffer. |
| **`0` (Rewind Reset)** | Button | Instantly resets the rewind frame input to frame `0`. |
| **Reverse / Forward** | Toggle Buttons | Toggles timeline playback in negative or positive frame increments. In legacy stateful modes, Reverse is disabled. |
| **Continuous Scroll** | Checkbox | When **checked**, frames stream indefinitely across the canvas. When **unchecked**, playback halts automatically once the internal buffer reaches `Buffer Size`. |
| **Buffer Size** | Numeric Input | Defines the maximum size of the internal sample buffer and the length of the dataset copied to clipboard. |
| **Native** | Button | Resets `Buffer Size` to match the exact pixel width of the current canvas (`canvas.clientWidth`). |
| **Copy Values to Clipboard**| Button | Exports the active buffer as a Python-compatible array `[0.123456, 0.789012, ...]`. |
| **Scroll Speed / MAX** | Slider + Checkbox | Controls playback increment per render frame. **MAX** runs an unconstrained time-budgeted batch loop (~12ms/frame) for raw CPU throughput profiling. |
| **FPS-R Evals/Sec** | Metric Meter | Rolling 3-second average of algorithm evaluations computed per second. |

---

## 2. Standard Workflows

### Workflow A: Quick-Capture Visible Canvas
**Objective:** Export exactly what is currently rendered on screen for instant plotting or offline inspection.

1. Click **Native** next to the `Buffer Size` input (locks buffer size to the current canvas width).
2. Let the visualizer run until a pattern of interest appears, then click **Pause** (or **Reverse** to halt).
3. Click **Copy Values to Clipboard**.
4. The clipboard will contain an array with length equal to `canvas.clientWidth`, representing the exact visible waveform from left to right.

---

### Workflow B: Deterministic Cross-Browser & Cross-Language Parity Test
**Objective:** Generate identical, frame-bounded test vectors across different devices (e.g., Desktop Chrome vs. Android Chrome vs. C/Python test runners) starting from an arbitrary frame index.

1. **Synchronize Parameters**: Ensure seeds, period/duration sliders, and multipliers are set to identical values.
2. **Set Target Start Frame**: Enter the desired starting frame (e.g., `1001`) into the numeric input beside **Rewind**.
3. **Click Rewind**: 
   - For stateless algorithms (FPS-R, Perlin, Worley), this directly relocates the clock to frame `1001`.
   - For stateful models (Legacy A & B), this fast-forwards sequential simulation from frame 0 to frame 1000 so that state memory precisely matches the continuous run.
4. **Set Exact Sample Span**: In `Buffer Size`, enter the target test vector size (e.g., `400` or `1000`).
5. **Disable Continuous Scroll**: Uncheck the **Continuous Scroll** checkbox.
6. **Trigger Capture**: Click **Forward** (or **Reverse**).
7. The visualizer will fill the buffer with exactly the requested sample count and automatically freeze upon reaching the final frame.
8. Click **Copy Values to Clipboard**.
9. Diff the exported array against your C/Python reference output to verify bit-level or floating-point parity.

---

### Workflow C: Throughput & LOD Compute Cost Benchmarking
**Objective:** Profile how computational load changes under varying Levels of Detail (LOD 0, 1, and 2) or algorithm configurations.

1. Select the target algorithm tab (e.g., **FPS-R: SM**).
2. Set **LOD** to `0` (Pure scalar output).
3. Enable **MAX** scroll speed toggle.
4. Allow the **FPS-R Evals/Sec** counter to stabilize (3-second rolling window). Note the baseline throughput.
5. Increase **LOD** to `1`:
   - Evaluates $t-1$ to detect change boundaries (`has_changed`).
   - Observe the throughput reduction (theoretically ~50% baseline due to evaluating two points per step).
6. Increase **LOD** to `2`:
   - Activates two-phase exponential probing and binary boundary search to compute `last_changed_frame`, `next_changed_frame`, and `hold_progress`.
   - Observe the additional ALU cost required for multi-step convergence.

---

### Workflow D: HPQ Time-Dilation Analysis
**Objective:** Visually confirm the transition boundary between Mode 1 (Tape Varispeed) and Mode 2 (Telescopic Extension).

1. Select any FPS-R algorithm (**SM**, **TM**, **QS**, or **BD**).
2. Set `seg_block_length` to `5`.
3. Set `frame_multiplier` to `1.0` (Normal speed). Observe the baseline phrase lengths.
4. Reduce `frame_multiplier` down to `0.25` (4× slow-down):
   - Notice the waveform stretching smoothly (Tape Varispeed).
5. Reduce `frame_multiplier` below `0.20` ($< 1 / \text{segBlockLength}$):
   - Notice the telescopic phrase extension generating new mutations within extended hold gaps while preserving master phrase boundaries.