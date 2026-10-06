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

#### Method 1: Bounded Visualizer Capture (Manual)
**Objective:** Capture an exact, frame-bounded sequence from an arbitrary start index directly within the visualizer.

1. **Synchronize Parameters**: Ensure engine seeds, period/duration sliders, and HPQ multipliers are identical across environments.
2. **Set Start Frame**: Enter the target start frame into **Start Frame** (e.g., `-156` or `1001`).
3. **Click Rewind**:
   - For stateless algorithms (FPS-R, Perlin, Worley), this relocates the clock to the target frame immediately in $O(1)$ time.
   - For stateful models (Legacy A & B), this fast-forwards sequential simulation from frame 0 to frame $t-1$ to re-warm state memory.
4. **Set Sample Span**: In **End Frame / Buffer Size**, enter the desired sample count (e.g., `1000`).
5. **Disable Continuous Scroll**: Uncheck **Continuous Scroll** to enforce the buffer stop boundary.
6. **Trigger Playback**: Click **Forward**. Playback streams and automatically freezes once the buffer fills.
7. Click **Copy Values to Clipboard** and compare outputs.

#### Method 2: Headless Parity Validator (Automated Suite)
**Objective:** Run bit-exact parity validation and execution benchmarking across multiple devices/browsers without visual rendering overhead.

1. **Switch to Validator Mode**: On the designated **Master** instance, switch to **Validator Mode (Offline Parity)** via the top mode toggle. Visualizer start frame and buffer bounds are automatically mirrored into the Master setup.
2. **Initiate Master (Origin)**:
   - Enter a **Master ID** (default `000`) and optional descriptive tag (e.g., `[Master] Chrome Win32`).
   - Click **Initiate & Copy Master Config**. This benchmarks Master headless execution, anchors index 0 of the ledger, and copies a lightweight **Config Capsule** (omitting raw values and benchmark timing) to the system clipboard.

##### Path A: Local Cross-Browser Testing (Single Machine, Shared Clipboard)
*Use this when comparing different browser engines (e.g., Firefox Gecko vs. Chrome V8 vs. Edge) on the same computer where the OS clipboard is shared.*

3. **Run Participant (Client)**:
   - Switch to or open another browser instance on the same machine.
   - Navigate to the demo, switch to **Validator Mode**, set a **Participant ID** (e.g., `001`) and description.
   - Click **Load Config, Run & Copy Payload**. The client reads the Master config from the shared clipboard, synchronizes parameters, runs headless evaluation, records benchmark timing, and copies the evaluated **Payload Capsule** back to the clipboard.
4. **Aggregate to Ledger**:
   - Return to the **Master** browser tab.
   - Click **Add Participant Capsule**. The Master compares all frame values and outputs a **PARITY**, **DISPARITY**, or **INVALID** verdict with diagnostic diffs and performance stats.

##### Path B: Cross-Device / Remote Testing (Different Physical Machines / Mobile)
*Use this when validating across hardware architectures (e.g., PC x86_64 to Android ARM64 or iOS Apple Silicon).*

3. **Dispatch to Target Device**:
   - Paste the Master's copied **Config Capsule** into a messaging channel, email, text file, or shared note (e.g., WhatsApp, Slack, Notes, or file transfer).
   - On the target device (phone, tablet, or secondary PC), copy that JSON text directly to its local device clipboard.
   - Open `fpsr_demo.html` on the device, switch to **Validator Mode**, enter a device descriptor (e.g., `Pixel 7 Android`), and tap **Load Config, Run & Copy Payload**.
   - The device evaluates the sequence headless, benchmarks the time, and copies the **Payload Capsule** (with evaluation values) to its device clipboard.
4. **Return Payload to Master**:
   - Paste the device clipboard back into your transfer channel (email/chat/file) and copy that text onto the Master machine's clipboard.
   - On the Master machine's Validator tab, click **Add Participant Capsule** to append the remote device to the ledger.

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