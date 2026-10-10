# FPS-R Evaluation & Visualisation Lab: User & Workflow Guide

The **FPS-R Evaluation & Visualisation Lab** (`fpsr_demo.html`) is an interactive workbench designed for:
1. Visually diagnosing structural and phase behaviors across FPS-R algorithms, legacy state machines, and continuous spatial noise models.
2. Generating deterministic, frame-accurate test vectors for cross-platform and cross-language parity validation.
3. Live throughput profiling across different Levels of Detail (LOD 0, 1, 2).

---

## 1. Transport & Buffer Controls Overview

| Control | Type | Description |
| :--- | :--- | :--- |
| **Reset (New Seeds)** | Button | Generates new randomized offsets and seeds for the active engine, resets timeline position back to **Start Frame**, resets LOD to 0, and restores default time scale. |
| **Rewind** | Shuttle Button | Jumps the timeline playback head immediately to the frame specified in **Start Frame** and flushes the active sample buffer without altering seeds. |
| **Start Frame** | Numeric Input | Defines the rewind target and baseline evaluation origin. Supports arbitrary positive or negative integer frames (e.g., `-156` or `1000`). |
| **`0` (Start Frame Reset)** | Button | Instantly resets the **Start Frame** input to frame `0`. |
| **Reverse / Pause / Forward** | Transport Buttons | Controls playback direction in negative or positive frame increments, or halts playback. (Reverse is disabled for legacy stateful engines). |
| **Continuous Scroll** | Checkbox | When **checked**, frames stream indefinitely across the canvas. When **unchecked**, playback halts automatically once the internal buffer reaches **End Frame / Buffer Size**. |
| **End Frame / Buffer Size** | Numeric Input | Defines the maximum capacity of the sample buffer, the stop boundary for bounded playback, and the dataset length exported to clipboard. |
| **Native** | Button | Snaps **End Frame / Buffer Size** to match the exact pixel width of the visualizer canvas (`canvas.clientWidth`). |
| **Copy Values to Clipboard** | Button | Exports the active buffer as a Python-compatible array `[0.123456, 0.789012, ...]`. |
| **Scroll Speed / MAX** | Slider + Checkbox | Controls playback increment per render frame. **MAX** runs an unconstrained time-budgeted batch loop (~12ms/frame) for raw CPU throughput profiling. |
| **FPS-R Evals/Sec** | Metric Meter | Rolling 3-second average of algorithm evaluations computed per second. |

---

## 2. Standard Workflows

### Workflow A: Quick-Capture Visible Canvas
**Objective:** Export exactly what is currently rendered on screen for instant plotting or offline inspection.

1. Click **Native** next to **End Frame / Buffer Size** (locks the sample size to current canvas pixel width).
2. Click **`0`** and **Rewind** to anchor the start to frame 0, or let the visualizer run until a pattern of interest appears.
3. Click **Pause** to freeze the frame buffer.
4. Click **Copy Values to Clipboard**.
5. The clipboard contains an array of floats matching `canvas.clientWidth`, representing the exact visible waveform from left to right.

---

### Workflow B: Throughput & LOD Compute Cost Benchmarking
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

### Workflow C: HPQ Time-Dilation Analysis
**Objective:** Visually confirm the transition boundary between Mode 1 (Tape Varispeed) and Mode 2 (Telescopic Extension).

1. Select any FPS-R algorithm (**SM**, **TM**, **QS**, or **BD**).
2. Set `seg_block_length` to `5`.
3. Set `frame_multiplier` to `1.0` (Normal speed). Observe the baseline phrase lengths.
4. Reduce `frame_multiplier` down to `0.25` (4× slow-down):
   - Notice the waveform stretching smoothly (Tape Varispeed).
5. Reduce `frame_multiplier` below `0.20` ($< 1 / \text{segBlockLength}$):
   - Notice the telescopic phrase extension generating new mutations within extended hold gaps while preserving master phrase boundaries.

---

### Workflow D: Headless Cross-Platform Parity Validation
**Objective:** Verify bit-exact deterministic parity across disparate JavaScript engines (V8, Gecko, JavaScriptCore), hardware architectures (x86_64, ARM64), and client OSes without visual canvas rendering overhead.

#### Architectural Overview
The validator operates on an asymmetric Master/Participant ledger model mediated via system clipboard JSON capsules:
* **Master (Origin):** Evaluates a baseline frame span headlessly, records raw ALU execution time, and exports a lightweight **Config Capsule** containing parameters and seeds (withholding values to enforce unbiased client-side evaluation).
* **Participant (Target):** Ingests the Master Config Capsule, automatically configures its local engine, evaluates the identical frame span headlessly, and generates a return **Payload Capsule** containing its computed output stream and benchmark timing.
* **Tri-State Verdict Engine:** When participant payloads are imported into the Master ledger, each frame undergoes bit-exact IEEE 754 64-bit float comparison:
  * <span style="color: #22c55e; font-weight: bold;">PARITY</span>: Identical engine configuration and 100% bit-exact float equality across all evaluated frames.
  * <span style="color: #ef4444; font-weight: bold;">DISPARITY</span>: Identical engine configuration and frame bounds, but one or more output values deviate (diagnostic flags the exact first diverging frame).
  * <span style="color: #9ca3af; font-weight: bold;">INVALID</span>: Parameter divergence (e.g., mismatched algorithm ID, seeds, or HPQ time-dilation settings).

---

#### Step-by-Step Procedure

##### Step 1: Establish the Master Baseline
1. In the top toolbar, switch from **Visualiser & Benchmark Mode** to **Validator Mode (Offline Parity)**.
2. Verify the active algorithm, seeds, and evaluation bounds (**Start Frame** and **Buffer Size / End Frame**). To modify bounds, click **Adjust in Visualizer**.
3. In **1. Master (Origin)**, specify a master identifier (e.g., `000`) and descriptor (e.g., `[Master] Chrome (Win32 x86_64)`). The `[Master]` tag prefix is optional but recommended for clarity in multi-participant sessions. All other participants descriptions would merely be `<Engine Name> <OS> <Arch>` (e.g., `Firefox Gecko Linux x86_64`) where a `[Participant]` prefix is optional.
4. Click **Initiate & Copy Master Config**:
   - The engine computes the baseline headlessly, records execution latency, and commits entry `[000]` to the ledger.
   - The lightweight **Master Config Capsule** JSON is copied to the system clipboard.

##### Step 2: Execute Headless Run on Participant
1. Open `fpsr_demo.html` on the participant target (either another browser on the same device or a remote device via network/file/message transfer) and switch to **Validator Mode**.
2. Set a **Participant ID** (e.g., `001`) and descriptor (e.g., `Firefox Gecko` or `Safari iOS ARM64`).
3. Ensure the Master Config Capsule JSON is in the participant's clipboard, then click **Load Config, Run & Copy Payload**:
   - The participant automatically mirrors the Master's configuration and executes the frame span headlessly.
   - The populated **Payload Capsule** (containing output values and execution timing) is copied to the participant clipboard.

##### Step 3: Register Payload & Inspect Parity Verdict
1. Transfer the participant's Payload Capsule back to the Master machine's clipboard.
2. On the Master instance, under **Validation Session (The Ledger)**, click **Add Participant Capsule**.
3. The participant entry is appended to the ledger displaying:
   - **Verdict Banner:** **`PARITY`**, **`DISPARITY`**, or **`INVALID`**.
   - **Deviation Readout:** Exact frame index and value delta if a disparity occurred.
   - **Execution Latency:** High-resolution headless evaluation time (`⏱️ ms`).

##### Step 4: Archive or Restore Validation Sessions (Optional)
* **Export Session:** Click **Export Session** to copy the entire multi-device session (Master baseline + all participant payloads) as an archival JSON document. The exported session content in the clipboard can then be saved to disk for future reference, regression testing, or cross-team collaboration.
* **Import Session:** Click **Import Session** A previously exported session JSON can be pasted into the clipboard and loaded on any client to restore a previously saved ledger for offline analysis or documentation appendices. With the session restored, the Master can re-validate all participant payloads, remove and append new participants, and re-run parity checks without requiring participants to re-run their engines.

---