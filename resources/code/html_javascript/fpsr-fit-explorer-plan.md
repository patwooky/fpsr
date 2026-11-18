# FPS-R Fit Explorer — plan (draft, 16 Nov 2026)

Prepared for Patrick Woo, after his 16 Nov 2026 messages about building a utility
to graph FPS-R output against a user-supplied dataset and find a fit.
Source demo: https://patwooky.github.io/fpsr/resources/code/html_javascript/fpsr_demo.html
Repo: https://github.com/patwooky/fpsr — 4 algorithms SM, TM, QS, BD; Apache 2.0.

## Decision: separate page, shared core

- **Separate page** (e.g. `fpsr_fit.html` next to `fpsr_demo.html`), **not** a third
  mode in the demo file. The demo already has two modes — visualise and parity
  check — and a third would bloat it and risk breaking both on every fit change.
- **Shared `fpsr.js`** loaded by both pages, so functions are reused without
  duplicating them. This is the split Muse offered to sketch on 16 Nov 2026,
  if Patrick sends the demo file.

### What goes in shared `fpsr.js`
- The 4 algorithms: SM (Stacked Modulo), TM (Toggled Modulo), QS (Quantised
  Switching), BD (Bitwise Decode) — `fpsr(current_frame, …)` and the wrapper
  with rich output.
- Parameters already on the demo: `minHoldFrames` (4), `maxHoldFrames` (60),
  `reseedInterval` (108), `seedInner`, `seedOuter`, `finalRandSwitch`.
- Global HPQ time controls: `frame_multiplier` (playback speed),
  `seg_block_length` (segment length), `varispeed_hold_block_count`
  (anchor persistence).
- Helpers both pages need: frame → value, hold detection (when a value
  changes), normalising a value to 0–1 / to the user's data range.

### What stays in `fpsr_demo.html` only
- Visualise mode, parity-check mode, continuous scroll, scroll speed,
  Start/End Frame, Max Limit, FPS-R Evals/Sec, LOD display.

### What is new in `fpsr_fit.html` only
- CSV upload (read in the browser only — the file never leaves the user's
  machine, same as the client-side demo; still free on GitHub Pages).
- Overlay graph: user data + FPS-R output on the same axes.
- Fit controls and fit score (below).
- No drag/zoom in v1 — that is a layer on top of the same canvas later,
  as agreed in chat.

## What "fit" means — two modes, because Patrick's "Yes" (16 Nov, 10:44 UTC)
## did not pick one, and both are in the FPS-R README

The README already graphs both "Output Values" and "Held Steps" for each
algorithm, so the explorer should score both, with a toggle:

1. **Value fit** — do the values match?
   - Overlay FPS-R (scaled to the user's range) on the user series.
   - Score: RMSE and correlation, shown live as sliders move.
   - Use when the dataset is a value-per-frame time series.

2. **Hold / phrasing fit** — do the hold durations match?
   - Compare the run-lengths: how long each value is held before it jumps.
   - Score: mean hold, hold histogram, and how often change-points line up.
   - Use when the interesting thing is the move-and-hold phrasing, not the
     exact values — this is the FPS-R-specific fit, and the one a plain
     value score would miss.

Default view: show both scores side by side; let the user pick which one
drives the "best fit so far" marker. No automated search in v1 — the user
hunts by feel with the sliders, exactly as in the visualiser. A heavy
automated parameter search is the only thing that would later push this
server-side.

## User dataset format (v1)

- CSV, one row per frame/time step. Header row allowed.
- Accepted columns: `frame,value` — or `time,value`, or just `value`
  (frame = row number, starting at 0 or at the Start Frame).
- Values: numbers. Non-numeric / blank rows are skipped and counted, so a
  messy export fails loudly instead of silently shifting the graph.
- No upload to any server; parsing with `FileReader` in the browser.

## V1 controls (reuse the demo's, plus three new ones)

Reuse: algorithm selector (SM / TM / QS / BD), min/max hold, reseed interval,
seeds, frame multiplier, Start/End Frame.
New: file picker + "use sample data" button, value-fit vs hold-fit toggle,
scale/offset for FPS-R output (so it can be stretched onto the user's range
without changing the phrasing).

## V1 layout

1. Top: upload + dataset summary (rows, range, mean hold of the user data).
2. Middle: one overlay canvas — user data in one colour, FPS-R in another,
   change-points marked.
3. Bottom: the demo's sliders, plus two live scores (value score, hold score)
   and a "best so far" readout (parameters + scores) the user can copy.

## Update — draft built from the tech doc, 16 Nov 2026 (evening)

Muse did not wait for the demo file: the algorithms' C reference is in
Patrick's own `FPSR_Tech.md`, so a draft shared core is built and tested:

- `fpsr.js` — `portable_rand`, `fpsr_sm`, `fpsr_tm`, `fpsr_qs`, plus fit
  helpers (`runLengths`, `valueFit` RMSE + correlation, `holdFit` mean hold
  + change-point match, `parseCSV`). Tested with Node: determinism, hold
  ranges (SM 38 runs / 500 frames, mean hold 13.2 at demo defaults),
  identical-series fit scores (RMSE ~0.01 after range scaling, correlation 1,
  change-point match 100%), and CSV parsing all pass.
- `fpsr_fit.html` — working draft page loading `fpsr.js`: CSV upload, sample
  data, SM sliders (demo defaults), overlay canvas, live value + hold scores,
  "best so far" marker. Syntax-checked; not yet opened in a browser.
- Still missing, needs the demo file: **BD** (not in the tech doc, which
  covers only the SM/TM/QS "trinity") and the HPQ wrapper / LOD code.
- Verify before the demo switches to this file: (1) `portable_rand`
  precision — C uses float32, this JS uses doubles, so compare one SM value
  against the demo's parity check; (2) SM reseed form — the tech doc's
  expanded function quantises the frame *before* adding `seedInner`, the
  README one-liner quantises *after*; this draft follows the expanded form;
  (3) the tech doc's `fpsr_sm` signature omits `finalRandSwitch` although
  its body uses it — this draft includes it. All three are flagged at the
  top of `fpsr.js`.

## Update — three ways in, 17 Nov 2026

Patrick asked (17 Nov, 13:11 SGT) how a user would upload a file given browser
security, noting his own copy-paste workaround and Firefox's clipboard prompt.
The draft page now offers all three doors, all local-only: file picker
(`FileReader`, one file, no path/folder access), drag-and-drop onto a dashed
zone (dropping is the permission, same model as the picker), and a paste box
for small datasets (Ctrl+V is user-initiated, so no prompt). All three feed
one `loadCSVText` loader. Verified 17 Nov: `fpsr.js` and the page's inline
script pass `node --check`; all referenced element IDs exist; core re-tested
(deterministic, 38 runs / 500 frames, mean hold 13.2 at demo defaults).

## What is still needed from Patrick

- The demo file, if he wants the actual split sketched (which functions move
  to `fpsr.js`, line by line) rather than this plan.
- Or, if he builds it himself: confirm the CSV shape — is it value-per-frame,
  and does he want value fit, hold fit, or both first? The plan above covers
  both, so either answer works; it only changes which score is shown first.

## Addendum — 17 Nov 2026: the demo file arrived, and the shared core is real

Patrick sent `fpsr_visualiser.html` (3,192 lines). Its core (lines 1242–2125)
was extracted verbatim into `fpsr_core_lib.js` (931 lines), and an updated
`fpsr_visualiser.html` now loads it with
`<script src="fpsr_core_lib.js"></script>`. Two closure fixes were needed:
legacy A/B state now lives in `fpsrLegacyState` with `fpsr_legacy_reset()`,
and `getFpsrDetails` / `getFpsrValue` take `(appFrame, activeAlgorithm,
globalParams)` explicitly. Parity-tested in Node: 207 checks, 0 fails.
Important: this supersedes the draft `fpsr.js` from 16 Nov — that draft's
`portable_rand` used a sin-hash from the tech doc, while the visualiser's
uses splitmix64, and its SM reseed / `finalRandSwitch` details were
unverified. Rebuild the fit explorer from `fpsr_core_lib.js`, not `fpsr.js`.
Still open: which fit to show first, value or hold.

## Update — fit methodology from the manuscript, 17 Nov 2026

Patrick shared the manuscript's fitting methodology ("Multidimensional
Phenomenological Mapping"). It changes what the fit explorer scores and how
it presents a "good" fit. It does **not** change the separate-page /
`fpsr_core_lib.js` decisions above.

### 1. Separate X and Y loss multipliers (v1)

Fitting must have a loss multiplier for the X axis (duration / hold timing)
and the Y axis (amplitude / value) **separately**, so each domain decides
how severe a deviation is along each axis. A timing miss and a value miss
are different sins in different domains: a parliamentary analyst may care
far more about *when* a jump lands than its height; a basketball analyst
may care more about the pass distance than the possession length. One
combined RMSE hides that choice.

- The page shows an **X score** and a **Y score separately**, then a
  **combined weighted loss** = `lossX * wX + lossY * wY`, with `wX` and
  `wY` as sliders (default 1.0 each).
- X loss: change-point timing error — how far FPS-R's jumps land from the
  user's jumps, within a tolerance window (below), plus mean-hold and
  hold-histogram difference.
- Y loss: value error at matched frames — RMSE and correlation of the
  values, after the scale/offset stretch onto the user's range.
- The combined weighted loss drives the "best so far" marker, not either
  score alone.

### 2. Per-point deviation cost (later, v1.1 — CSV column reserved now)

Further down the line, some points matter more than others. A per-point
weight multiplies into the global X/Y loss for that point: a significant
event's deviation costs more than a routine one. The CSV format reserves
an optional third column, `weight` (default 1.0 when absent), so datasets
can carry it now without the UI needing the full per-point editor yet.
The score code already applies `weight` per point; the editor UI is
deferred.

### 3. Mapping is interpretive — the page needs a metric "key"

There is no single correct 1:1 mapping of a phenomenon onto the X
(duration) and Y (amplitude) axes. One chaotic event contains many
overlapping streams, and the architect chooses which "hold-then-break"
cadence to extract — e.g. basketball as possession length vs pass
distance, momentum shift vs point swing, or disruption vs penalty
severity; a parliamentary session as friction, partisan loyalty, thematic
filter, or latent success. The same dataset yields entirely different
streams under different lenses.

- The page therefore has **mapping-key fields**: stream name, what X
  means (seconds? sittings? possession?), what Y means, and the rule
  used to call a jump. These are saved with the "best so far" readout.
  Without the key, the curve is naked geometry and means nothing.
- The dataset summary states the mapping choices back to the user; the
  fit scores are meaningless without them.

### 4. Fuzzy jumps — consistently coherent interpretation

In most domains the exact instant a value "jumps" is rarely razor-sharp:
does possession end when the ball leaves the foot or when an opponent
intercepts it? The architect must make a decisive best-guess ruling on
how to define a boundary and apply that same principle uniformly across
the whole dataset. The explorer cannot fix a fuzzy boundary; it can only
score consistently against the boundary the user supplied. The tolerance
window (below) is the admission of that blur, not a fudge.

### 5. Structural tolerance, not pixel-perfect alignment

Because the empirical stream is interpretively built, flawless mirroring
is a mathematical improbability. The explorer must **not** grade a fit
pass/fail at 95%. Realistic thresholds from the manuscript:

- Primary fit: **60–70% of macroscopic holding plateaus** captured.
- Secondary supporting streams: **fewer than 30%** of mathematical
  inflections within the general temporal neighbourhood of the empirical
  data, with **under 5%** "almost touching" / exact frame alignment.

So the page reports structural scores — plateau capture %, change-points
matching within a user-set tolerance window (default ±2 frames), mean-hold
ratio, hold-histogram overlap — and lets the domain's thresholds judge
them. A fit is a structurally coherent projection within acceptable
domain thresholds, not an exact overlay.

### 6. What this changes in v1 (checklist)

- [ ] X-loss and Y-loss sliders (0–3, default 1.0), combined weighted
  loss shown live, drives "best so far".
- [ ] Tolerance window input for change-point matching (default ±2
  frames); plateau capture % shown next to it.
- [ ] X score, Y score, and combined loss shown as three separate
  readouts — never a single number alone.
- [ ] Mapping-key fields (stream name, X meaning, Y meaning, jump rule)
  above the scores, included in the copied "best so far" readout.
- [ ] CSV accepts an optional `weight` column (per-point cost, default
  1.0); the score code applies it, the editor UI is deferred to v1.1.
- [ ] No pass/fail verdict. The realistic thresholds (60–70% / <30% /
  <5%) are shown as guidance text beside the scores.
- [ ] Still open: which fit to show first, value or hold. Show both
  side by side, as before; the combined weighted loss is the tie-breaker.

## Update — rebuilt from fpsr_core_lib.js, 17 Nov 2026

`fpsr_fit.html` was rebuilt from `fpsr_core_lib.js` (not the superseded
`fpsr.js`) with the methodology above: X/Y loss sliders, tolerance window,
separate X/Y/combined scores, plateau capture %, hold-histogram overlap,
mapping-key fields saved with "best so far", optional CSV `weight` column,
and the three local-only data doors. Two real bugs were caught by the
Node smoke test (`hidden_files/fit_explorer_test.js`, all tests passing):
(1) `finalRandSwitch` was 0 in the first build, so SM/TM returned the raw
held integer state instead of a 0–1 value — it is 1 in the visualiser's
defaults (checked) and now in the fit page too; (2) the Y stretch was
forced to the user's min/max, which distorted even an identical series —
it is now a weighted least-squares fit (scale + offset), so an identical
series scores exactly combined 0.000, 100% change-points, 100% plateaus,
correlation 1.000. Sample fit (SM 8/30/60 vs default 5/60/120): 30%
change-points at ±2 frames, combined loss 0.622 — in line with the
manuscript's <30% secondary-stream expectation. Keep `fpsr_fit.html` next
to `fpsr_core_lib.js`; it loads it with
`<script src="fpsr_core_lib.js"></script>`.

## Update — visualiser port with paging, scrub and colour ramp, 17 Nov 2026

Patrick's verdict on the form-style rebuild: it feels like a form to fill
in. The page was rebuilt again as an exact port of the visualiser
(`hidden_files/build_fit_port.js` assembles it; the SM/TM/QS/BD panels
and the `<style>` block are copied verbatim from
`fpsr_visualiser.html`, same ids and HTML defaults — note the real
defaults are the HTML values, e.g. SM minHold 4 / reseed 108, TM
37/25/54, QS baseWaveFreq 0.0015 — not the `updateParams` fallbacks).
Same canvas, Start Frame / End Frame / Native transport, Reset (New
Seeds), Rewind, Copy Values, algorithm tabs and Global HPQ controls.
No continuous scrolling, no scroll speed, no validator mode.

What is new, and how it works:

- **Paging, not scrolling.** The page is filled once (one "page" of the
  curve buffer). End Frame is the page size / zoom level; Native resets
  it to the canvas width at 1 point = 1 pixel; a page slider scrolls the
  window through the user's data (e.g. window 800 over 2,000 rows).
- **Scrub (live).** A slider slides FPS-R against the user's curve as a
  phase offset in frames (`frame + scrub`, still O(1) per frame, nothing
  re-walked). Loss numbers and curve colour update on every drag.
- **Colour ramp.** The FPS-R curve is coloured by the normalised combined
  loss (combined / (wX + wY)): green → amber → red, with a legend and a
  swatch. Per-segment colouring is a later step, not v1.
- **Fit controls.** Thresholds per axis (X: change-point tolerance in
  frames; Y: jump-detection threshold as % of range) and loss multipliers
  per axis (wX, wY). Curve opacity sliders for the user and FPS-R curves.
- **Diagnostics (read-only).** X loss, Y loss, combined + normalised
  loss, change-point match, plateau capture, mean holds, histogram
  overlap, correlation, fitted stretch. Mapping-key fields and the
  per-point CSV `weight` column carry over from the previous build.
- Verified by `hidden_files/fit_port_test.js` (Node, DOM stub — all
  passing): sample loads, identical params score exactly 0.000 with 100%
  change-points, scrub +50 moves the loss and the ramp colour, paging
  End Frame 100 over 400 rows gives page max 300 and frames 300–399,
  TM tab refreshes, best-so-far carries the mapping key. No duplicate
  element ids (94 ids checked).
- **No-data mode (17 Nov 2026):** with no dataset loaded, FPS-R still
  plots (0..1 range) under a red watermark — “Upload or paste an
  empirical dataset” at 60% transparency — so scrolling and scrubbing
  can be tried immediately. The page slider roams a 10,000-frame stretch
  until data arrives; the scrub is the phase offset, as expected.
- **Zoom / negative frames (17 Nov 2026):** the flattening at frame 0 was
  the page's own `Math.max(0, …)` clamp, not the core — a Node probe
  (`hidden_files/negative_frames_probe.js`) shows all four algorithms
  vary deterministically with real hold structure at negative frames
  (SM 18 distinct values / 18 runs over −500..0, TM 32/35, QS 36/71,
  BD 207/207). The clamp is removed, so FPS-R now extends to negative
  frames. The controls are split cleanly: End Frame is the full extent
  of the timeline (data length, or the End Frame value with no data);
  the zoom slider is the viewport size (pull left to zoom in); the
  scroll slider is the window start; the scrub slider is the FPS-R phase
  offset (range ±extent, with a “0” snap button) — with a small viewport
  and a large extent, scrubbing walks FPS-R through the extent page by
  page (e.g. zoom 200 over extent 2000 = 10 pages). Two zoom snap
  buttons: “Actual pixels” (viewport = canvas width, 1:1) and “Full
  extent” (viewport = whole timeline). Verified in
  `hidden_files/fit_port_test.js` (all passing).
- **Slider fixes (17 Nov 2026, after Patrick's screenshot):** (1) the
  ghost knob was the zoom slider squeezed into a sliver by the long
  readout sharing its flex row — the readout now sits on its own line,
  so the zoom slider gets the full row like scroll/scrub. (2) In
  no-data mode scroll and scrub looked identical (both shift the
  FPS-R curve), so the readout now names the FPS-R frames too, e.g.
  “frames 0–872 of 2000 · FPS-R −2000–−1128 · zoom 873 · scrub −2000” —
  scroll moves the window frames, scrub moves only the FPS-R frames.
  (3) When the viewport already shows the whole extent, the scroll
  slider greys out with “nothing to scroll” instead of looking stuck.
  A Node simulation with real browser range-clamping semantics
  (`hidden_files/fit_sim.js`) confirms the logic: after Actual pixels
  the scroll range is 0–1140 and scrolling to 500 updates the readout;
  after Full extent the range is correctly 0. My cloud browser could
  not open the page to check it live (localhost rejected, the hosted
  copy blocked by its organisation policy, granted files not openable
  as a page), so the sim + the Node test are the verification.
- **Axis ticks + no-data reference (17 Nov 2026):** frame ticks run
  along the bottom of the plot — “Ticks per view” (default 10 major
  ticks across the viewport, 2–50) with a “Sub-ticks” checkbox for
  halfway minor ticks. Ticks mark window frames, so scrolling moves
  the labels while scrubbing shifts the curve under fixed labels.
  In no-data mode a faint grey reference curve shows FPS-R at scrub 0,
  fixed to the window, so the live curve has something to slide
  against; it is not drawn once user data is loaded (the user curve
  is the reference there). Verified in `hidden_files/fit_port_test.js`
  (tick labels, tick-count change, sub-ticks toggle, reference stroke
  present only with no data).
- **Frame-anchored adaptive ticks + mode pill (17 Nov 2026):** ticks are
  now anchored to absolute frame numbers — “Tick interval (frames)”,
  default 10, means one mark every 10 frames. Overcrowding is handled
  by the standard adaptive-tick scheme (the 1-2-5 ladder, as in d3 /
  matplotlib): the interval steps up 1, 2, 5, 10, 20, 50, 100, … to
  the smallest step that keeps the label count within what fits the
  canvas width, and a note beside the control shows the effective
  interval (“auto → every 100 frames”). Sub-ticks mark the halfway
  points when they fit. Control explanations moved from the permanent
  paragraph into hover tooltips (`title` on each control). The scrub
  readout is renamed “fps-r seed ±offset” for consistency with engine
  parameter naming. A Visualiser / Validator dual pill toggles the
  engines-and-params panels vs the fit-controls panel; the
  zoom/scroll/seed/ticks panel and the read-only diagnostics stay
  visible in both modes, so scrub-hunting keeps its eyes on the
  scores and curve colour with less screen real estate (aimed at
  mobile). Verified in `hidden_files/fit_port_test.js` (adaptive
  step-up 10 → 100 over 860 frames, every-frame ticks at zoom 10,
  sub-ticks, mode toggle) and `hidden_files/fit_sim.js`.
- **Normalised-loss colour, Y grid, profile + collapsibles (18 Nov
  2026):** the normalised-loss number above the colour ramp now
  colours green/red by its own previous value, like the three loss
  numbers. The plot has faint horizontal gridlines at 0, 0.2, …
  1.0 with labels down the left edge (the shared axis always covers
  [0, 1], so all six are always in range); the axis-extreme labels
  moved to the right edge so they never collide with the 1.0/0.0
  grid labels. A collapsible “User-data profile” sits under
  Diagnostics: hold statistics of the full dataset (same
  exact-change definition as the X-loss change-points) — frame
  count, hold count, min/max/median/mean hold in frames, value
  min–max and mean — the timings to tune min/max hold, reseed and
  switch intervals against. The mapping-key fields moved out of the
  upload panel into a collapsed “Mapping key” frame in the Validator
  panel, next to Mark-best, where they are used. The upload section
  is now collapsible: it starts open, and once data is loaded it
  tucks away (the row count stays visible in its header) while the
  profile opens; clicking the header expands it to re-upload.
  Verified in `hidden_files/fit_port_test.js` (normalised loss red
  on rise / green on fall; grid labels 0.0–1.0 present with the
  union axis in both windows; a known-hold paste profiles as
  3 holds · min 5 · max 20 · median 10 · mean 11.7; upload
  collapses on load and re-expands on click).
- **“User” wording + Graph display options (18 Nov 2026):** every
  visible “your” on the page became “user” / “the user's”
  (headings, intro, tooltips, diagnostics, metrics bubble) —
  verified by a test asserting no “your” remains in the built HTML.
  Tick interval + Sub-ticks moved out of the sliders panel into a
  collapsible “Graph display options” frame directly above the
  viewport sliders; the User curve / FPS-R curve opacity controls
  moved there from the Validator fit controls, side by side in one
  row after the tick options, so opacity changes show in the plot
  immediately below as they are adjusted. The frame starts open.
  Verified in `hidden_files/fit_port_test.js` (frame toggles both
  ways; the moved opacity control still drives a refresh).
  — the scroll row now has its own “0” button that jumps the window
  back to the start (that is the rewind). Start Frame is now the
  FPS-R neighbourhood anchor, decoupled from the data window: data
  frame 0 aligns to FPS-R Start Frame, and scrub (±extent) explores
  around it, so Start Frame 1000 with 200 data rows scrubs FPS-R
  800–1200; entering 3000 jumps to the 2800–3200 neighbourhood.
  (Before, Start Frame moved the window itself and was clamped to
  the data range, so 1000 did nothing with 200 rows; its “0” button
  also only set the field without refreshing — both fixed.) The three
  sliders had `w-full`, which forced their buttons onto a second row —
  they are now `flex-1 min-w-0`, so zoom / scroll / seed rows each
  stay on one row. Seed readout compacted to “seed +0”. Copy Values
  stays in the transport row with Reset / Start Frame / End Frame.
  The “realistic thresholds” note is now an (i) button beside the
  Diagnostics heading: click to open a bubble that stays until
  clicked away. Verified in `hidden_files/fit_port_test.js` (Start
  Frame 1000 → FPS-R 1000–1859, scrub +200 → 1200–2059, scroll 0
  rewind, no Rewind button in the HTML, info button present).

## What this does NOT change

- No hosting cost, no server, no paid tier. GitHub Pages as now.
- No change to the demo or parity check if the shared file is extracted
  carefully — that is the point of the split.
- No ISBN / KDP / manuscript decision is touched by this.

## Vision ahead — Automatic Match (phased) (18 Nov 2026)

The explorer is an instrument today: the user turns the knobs by hand
(seeds, scrub, scroll, zoom) and watches the three loss numbers move.
Automatic Match turns it into a search engine: the page does the turning,
over many candidates, and hands back the best fits as clickable lists.
The full specification lives in `fpsr-automatic-match-spec.md`; this is
the shape of it, as Patrick spec'd it (17–18 Nov 2026).

- **Phase 1 — automatic seed-space exploration (v1).** Algorithm and
  structural parameters frozen; the seed space (2^64, splitmix64) is
  random-sampled, never hill-climbed (the seed avalanches — there is no
  nearby seed). The operator provides the instinctive periodicity of the
  nested mechanism, the time-scale knowledge that brackets the search.
- **Smart match — run-length-encoded trains.** Both sides compress to
  their jump lists: the user data is a train of (time, value) tuples,
  one per inflection, plateaus implied; an FPS-R candidate's train is
  computable jump-to-jump, O(1) per jump, with no frame-by-frame walk.
  Per seed the whole trains are aligned at once (tolerance-window
  change-point match) — every jump votes, none is the pivot — and the
  phase offset is *derived* from the trains (median of the paired
  jump-time differences, neighbours scored), not swept. Windows are
  inflection-anchored, not fixed: a stretch starts at a user inflection
  and the whole sub-train from there is matched as one unit.
- **Three top-k lists.** One per criterion — best timing match (X loss),
  best value match (Y loss), best integrated match (combined loss) —
  as one table with a sort control; default k 10. Clicking a row
  restores the candidate exactly and jumps the timeline to it.
- **Phase 2 — wedged parameter sweep.** Every structural parameter
  takes a bounded range with increment steps (e.g. min_hold 5–30,
  max_hold 20–50), like wedging in VFX and bracketing in photography.
  The combinatorial explosion (26 × 31 = 806 combinations for two
  parameters alone, before reseed and seeds; QS / BD wilder still)
  means pure grid search dies here — Phase 2 needs sampling and
  coarse-to-fine, and may need to bound the input coordinate seed
  space itself (an interval, or a sampled subset) so the total search
  stays tractable.
- **Not built yet.** The spec is a draft for Patrick to correct; the
  open decisions (which seed field varies, default k and budget, dedup
  rule, stretch length, the Phase 2 seed-space bound) are listed in
  its §7. Do not build Automatic Match until he approves it.
