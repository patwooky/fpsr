# FPS-R fit explorer — Automatic Match (draft spec)

Drafted 17 Nov 2026 (evening, SGT) from Patrick's message that day:
"Automatic match given the current settings, current algorithm. Scoring users data
set across 2^64 numerical representation space. And return top k score by time, by
value, by integrated (combined). As a list. When user clicks on an entry, timeline
jumps to that point."

Status: draft for Patrick to correct. Nothing here is built yet. The scoring
definitions below are taken from the shipped `fpsr_fit.html` as it stands;
the search strategy is new and is the part that most needs his say-so.

Revision 18 Nov 2026: the v1 search strategy in §4 was rewritten. The 17 Nov
draft proposed random seed sampling with the scrub (phase) swept per seed;
Patrick's smart-match correction since then replaces the scrub sweep with
train alignment — the phase offset is *derived* from the jump trains, not
searched — and adds the phased plan (§4.3): seed sweep first, wedged
parameter sweep next. The scrub-sweep version is kept out of this spec;
it is superseded, not an alternative.

Revision 18 Nov 2026 (later): scoring additions from the Quantitative
Metrology cull — the weighted validation metric C_FPSR (§3.1), the
Gaussian soft-hit as a smoother for the Phase 2 hill-climb (§4.4),
and the ML spectrometer recorded as considered and rejected (§9).
The three losses in §3 are unchanged; C_FPSR complements them, it
does not replace them.

## 1. What it is

Today the explorer is an instrument: Patrick turns the knobs by hand (seeds,
scrub, scroll, zoom) and watches the three loss numbers move. Automatic Match
turns it into a search engine: the page does the turning, over many candidates,
and hands back the best fits ranked three ways, as a clickable list. Clicking
a row restores that candidate exactly and jumps the timeline to it.

It is the same scores, the same page, the same controls — only the search is
automated. That is also why it is only conceivable for FPS-R: every candidate
is `getFpsrValue(frame, algorithm, params)` — O(1) per frame, no walk from
genesis. A stateful stream could not even be sampled this way; each candidate
would have to be walked from frame 0.

## 2. What stays fixed in v1 (per Patrick's wording)

- **Algorithm:** the currently active one (SM / TM / QS / BD). No cross-algorithm
  search in v1 — that is a later step, and it would need a fair comparison
  across different parameter sets.
- **Structural parameters:** everything in the active algorithm's panel stays
  as Patrick set it (e.g. SM minHold / maxHold / reseedInterval, TM periods,
  QS frequencies / quantisation, BD ops). Only the search dimensions in §4 vary.
- **Fit controls:** wX, wY, tolX (± frames), epsY (% of range), per-point
  weights, and the mapping key (stream / X-means / Y-means / jump rule) stay
  as set. The search ranks by the scores those controls define; changing them
  re-ranks, it does not re-search — unless Patrick wants a re-run.
- **Window:** the current viewport window (pageStart … pageStart + viewport).
  Scores are defined per window in the current code, so v1 searches within
  the window Patrick is looking at. Within the window, matching is done on
  inflection-anchored stretches, not fixed blocks — see §4.1. Searching
  other window positions is a separate dimension — see §4, "pageStart".

## 3. The three scores (exactly as implemented today)

All three come from `scoreFit(user, weights, fpsr01, tol, epsFrac, wX, wY)`
in `fpsr_fit.html`. No new loss definition is needed for v1; §3.1
adds a validation score (C_FPSR) built on the same thresholds and
weights, for ranking finalists — it complements the three losses,
it does not replace them.

- **By time — X loss** (timing / phrasing). `xLoss = 0.5·(1 − cpMatch) +
  0.5·(1 − plateauCapture)`, where cpMatch is the share of the user's
  change-points matched by an FPS-R change-point within tolX frames
  (default 2), and plateauCapture is the share of the user's hold runs
  at least 50% overlapped by an FPS-R run. Lower is better; 0 is exact.
- **By value — Y loss** (amplitude). `yLoss = rmseNorm`: weighted RMSE
  between the user's values and the FPS-R values after a fitted
  least-squares stretch (scale + offset, weighted by the per-point
  weights), normalised by the user's value span and capped at 1.
  Lower is better; 0 is exact.
- **Integrated — combined loss.** `combined = wX·xLoss + wY·yLoss`
  (defaults wX = 1, wY = 1). Lower is better. This is the number that
  already drives the curve colour (green → amber → red).

Each candidate also carries the supporting diagnostics the page already
computes, so a row can show them without extra work: cpMatch,
plateauCapture, correlation, mean hold (user vs FPS-R), hold-histogram
overlap, and the fitted scale / offset. The manuscript's realistic
thresholds still apply — a primary fit capturing 60–70% of macroscopic
plateaus is good; there is no 95% pass/fail.

Confirmed 18 Nov 2026: Patrick's "by best value match, best timing match
and best integrated match" settles the reading — "by time" is the X
(timing) loss, "by value" the Y loss, "integrated" the combined loss.

## 3.1 Validation score for ranking finalists — C_FPSR

The three losses in §3 rank candidates during the search. For the
finalists — the rows of the top-k list — the search should also carry
a strict, human-verifiable validation score, taken from the book's
Quantitative Metrology section (culled 18 Nov 2026 from the ML
training plan; the metric itself is ML-independent — it scores any
fit, however found). It is built from the same thresholds and weights
the explorer already has: thresh_t is tolX (frames), thresh_v is
epsY (as a fraction of the user range), and W_i is the per-point
weight from the CSV weight column (arbitrary scalars accepted, then
dynamically normalised against the maximum to [0.0, 1.0]).

- **Boolean gate.** `Hit_i = 1` if `|Δt_i| ≤ thresh_t` and
  `|Δv_i| ≤ thresh_v`, else 0. A hit is a strict pass/fail: the
  predicted jump lands inside the tolerance box around the user's
  jump, or it does not.
- **Weighted confidence density.** `C_density = Σ(W_i · Hit_i) / Σ W_i`.
  The share of the user's jumps hit, weighted by structural
  importance — a hit on a load-bearing anchor (W_i = 50) counts
  fifty times a hit on ambient noise (W_i = 1).
- **Weighted precision / fit tightness.**
  `L_tightness = 1.0 − Σ(W_i · Hit_i · D_norm_i) / Σ(W_i · Hit_i)`,
  where D_norm_i is the normalised distance of the error inside the
  box (0 at dead centre, 1 at the edge); defined as 0.0 when there
  are no hits. Near-centre hits score higher than hits scraping the
  box edge.
- **Horizon.** `Horizon = log2(1 + N_hits) / log2(1 + N_total)`.
  A short matching streak can happen by coincidence; the horizon
  rewards volume of anchors across a prolonged timeline, at a
  diminishing (logarithmic) rate, and it can never exceed 1.0.
- **Integrated confidence.** `C_FPSR = C_density × L_tightness × Horizon`,
  bounded [0.0, 1.0]. **Higher is better** — the opposite direction
  from the three losses, so the UI must not confuse the two.

What it achieves: the three losses measure *how close* a candidate
is; C_FPSR measures *whether the fit is structurally sound* — did it
capture the load-bearing beats (density), how precisely (tightness),
and over a long enough stretch to rule out coincidence (horizon).
A candidate can have a decent combined loss while missing every
heavy anchor; C_FPSR catches that, because the weights make the
heavy anchors dominate. It is the score a human reads to verify a
fit, and it is the natural ranking score for the top-k lists (§5) —
with the combined loss kept as the default sort until Patrick says
otherwise (see §7).

## 4. The search space — and what "2^64" actually means

The 2^64 space is the **seed space**: seeds are 64-bit BigInts, hashed
through splitmix64 in `fpsr_core_lib.js`. It cannot be exhausted — that
is not a limitation to work around, it is the premise. The search must
sample it, and the design question is *how* to sample well.

Candidate dimensions in v1 (all else frozen, per §2):

| Dimension | What varies | Range |
|---|---|---|
| Seed | one seed field of the active algorithm (proposed: seedInner; SM/TM seedInner, QS stream offsets, BD valueSeedOffset — see open question) | uniform random 64-bit BigInt |
| Phase offset | FPS-R phase offset, in frames — **derived** from the jump trains (§4.1), not searched | −extent … +extent (the scrub slider's current range) |
| Page start | which user window is scored — fixed at the current pageStart in v1 | 0 … extent − viewport (not varied in v1) |

Two facts shape the strategy, and both matter:

1. **The phase is derived, not searched.** FPS-R is evaluated at frame
   `pageStart + i + scrub` against user frame `pageStart + i`. In the
   17 Nov draft the scrub was swept per seed; the smart match in §4.1
   derives it from the jump trains instead, which removes the whole
   phase dimension from the search. PageStart still chooses *which*
   user segment is scored — it is a different dimension from the phase,
   and it stays fixed in v1.
2. **The seed is discontinuous.** splitmix64 avalanches: seed 456 and seed
   457 produce unrelated curves. There is no "nearby seed" to hill-climb
   toward — gradient / hill-climbing on the raw seed value is meaningless.
   Hill-climbing only works on smooth dimensions (the derived phase, and
   later minHold / maxHold / periods). The seed must be **random-sampled
   and kept**, never hill-climbed.

### 4.1 Smart match — run-length-encoded trains, aligned whole

The search does not iterate `frame += 1` and does not sweep the phase.
It matches jump to jump.

- **Trains.** Both sides compress to their jump lists — run-length
  encoding. The user's data is a sequence of holds, so it reduces to a
  train of (time, value) tuples: one tuple per inflection, the plateaus
  implied, not stored. Example: user jumps at frames 12, 30, 47, 80
  give the train (12, 0.7), (30, 0.2), (47, 0.9), (80, 0.4). An
  FPS-R candidate's train is the same shape: its jumps are computable
  jump-to-jump, because a hold's start and duration are computable from
  the frame — O(1) per jump, no frame-by-frame walk, no re-walking from
  genesis.
- **Whole-train alignment, one scoring pass.** Per sampled seed, the
  full jump trains are aligned at once, with the tolerance-window
  change-point match the page already implements (§3). Every jump
  votes; none is the pivot. Pivoting on the first jump alone would
  bias the match, so there is no "transform pivot" — the alignment is
  one pass over all inflections per seed, cost O(jumps).
- **The phase is derived from the trains.** Pair the jumps, take the
  median of their time differences, and score a handful of neighbours
  around it. Worked example: FPS-R jumps at 5, 22, 41, 75 against the
  user train above give differences 7, 8, 6, 5 — median ≈ 7, so the
  phase offset is 7; shifted, the FPS-R jumps land at 12, 29, 48, 82,
  all within ±2 frames. The values at the matched jumps give the Y
  score. One score for the whole sub-train at once.
- **Inflection-anchored windows, not fixed blocks.** A stretch is not
  frames 1–100 then 101–200 — a fixed cut slices through holds
  arbitrarily. A stretch **starts at a user inflection** (e.g. frame
  30) and the whole sub-train from there is matched as one unit; the
  next stretch starts at the next user inflection after it, so a hold
  is never cut by an arbitrary boundary. Stretch length — the next K
  jumps, or the next W frames — is an open search parameter (see §7).
- **Cost per seed.** O(jumps) for the train extraction and alignment,
  plus O(window length) for the full `scoreFit` at the derived phase
  (§3), which produces the three losses the list is ranked by. The
  train alignment is what makes the phase cheap; the full scoring
  pass is still the dominant cost per seed, so total cost is roughly
  candidates × window length.

### 4.2 Proposed v1 strategy (for Patrick to approve or replace)

1. **Random seed sampling, phase derived.** For each sampled seed,
   extract the FPS-R jump train over the window (jump-to-jump), align
   it with the user's train, derive the phase from the median jump-time
   difference, and score the full window once at that phase with
   `scoreFit`. No scrub sweep per seed.
2. **Coarse-to-fine (optional in v1, recommended).** The train alignment
   itself is the cheap first pass: it yields the phase and a timing
   signal in O(jumps). Keep the top fraction of candidates by that
   signal, then re-score those with the full `scoreFit`. This is an
   optimisation, not a different score — the final ranking always uses
   the full score.
3. **Budget, not exhaustion.** The user sets a budget — a number of
   candidates, or a number of seconds — and the search stops there.
   Cost per candidate is O(window length) `getFpsrValue` calls plus
   O(window length) scoring, so total cost is roughly
   candidates × window length. No time estimate is given here: it must
   be measured in Patrick's browser on a real dataset before any
   default budget is chosen.
4. **Chunked, cancellable, in the browser.** The search runs in slices
   (e.g. via setTimeout / requestIdleCallback) so the page stays
   responsive, with a progress bar (candidates tried / best so far)
   and a Cancel button. A Web Worker is a later step, not v1.
5. **Reproducible.** The search itself takes a search-seed, so re-running
   the same search on the same data and settings reproduces the same
   list. Without that, a "best" result could never be re-found or
   checked — which would break the provenance habit the rest of this
   project keeps.

### 4.3 Phased plan — seed sweep first, wedged parameter sweep next

The search expands in phases, in this order:

- **Phase 1 — seed sweep (v1).** The seed space is searched with the
  smart match above, algorithm and structural parameters frozen. The
  operator / researcher provides the instinctive periodicity of the
  nested mechanism — time-scale knowledge that brackets the search, so
  it is not wandering 2^64 blind.
- **Phase 2 — wedged parameter sweep.** Every structural parameter can
  then take a bounded range with increment steps — e.g. min_hold 5–30,
  max_hold 20–50 — and the search sweeps the combinations, like
  wedging in VFX and bracketing in photography. The parameter space
  gets wild fast: just min_hold (26 values) × max_hold (31) is 806
  combinations before reseed and the seeds, and QS / BD spaces grow
  wilder still as the bracketed space increases. Pure grid search dies
  here, so Phase 2 needs sampling and coarse-to-fine over the parameter
  dimensions, with hill-climbing only on the smooth ones (minHold /
  maxHold / periods — never the raw seed, §4).
- **Bounding the seed space in Phase 2.** Because the parameter sweep
  multiplies the seed sweep, the input coordinate seed space itself
  may need bounding at this point — a seed interval, or a sampled
  subset of the seed space, rather than the full 2^64 — so the total
  search stays tractable. The exact bound (interval, subset, or count)
  is an open question (see §7).

### 4.4 Smoother scoring for the Phase 2 hill-climb — soft hits

Phase 2 hill-climbs the smooth structural parameters (minHold /
maxHold / periods). The binary hit/miss gate (§3.1) gives that climb
nothing to follow: a candidate 3 frames off scores exactly the same
as one 300 frames off, so the landscape is flat — the same
"zero-gradient desert" the ML plan faced, now met by the search
instead. The salvage from the culled Stage 1 formulation is the
Gaussian soft hit, used here as a *search* objective, not a training
loss:

- **Soft hit.** `S_i = exp(−½[(Δt_i/σ_t)² + (Δv_i/σ_v)²])` — the
  rigid Boolean gate relaxed into a smooth, two-dimensional Gaussian
  decay. A near-miss scores partial credit, falling off exponentially
  with distance; σ_t and σ_v are temperature hyperparameters
  controlling how sharply it falls off.
- **Soft loss.** `L_spec = 1 − (1/N_total) Σ S_i`, weighted by W_i
  in the natural way (each S_i multiplied by its point's weight
  before averaging). As predictions align, L_spec → 0.
- **Annealing.** Start σ wide (the climb can see far-away signal)
  and narrow it as the climb converges, so late steps discriminate
  frame-level differences.

What it achieves: the hill-climb becomes navigable — it can walk
toward the right parameter neighbourhood on a slope instead of
wandering a flat landscape. It must **not** be the ranking score:
final ranking always uses the exact discrete scores (§3 and §3.1),
or near-misses would be rewarded as if they were hits. The soft
score guides the climb; the hard score judges the result.

Later, not v1 or Phase 2: cross-algorithm search; searching pageStart
as a full dimension; per-point weights influencing the search order.

## 5. The top-k list

- **Three top-k lists, one per criterion** (rather than a single
  ranking): a list by **best timing match** (lowest X loss), a list
  by **best value match** (lowest Y loss), and a list by **best
  integrated match** (lowest combined loss). They share one table with
  a control to switch the sort, so the three lists are three views of
  the same candidate records. Default sort: combined. Default k: 10
  (open question — Patrick said "top k" without fixing k).
- **Each row shows:** rank, X loss, Y loss, combined loss, cpMatch,
  plateauCapture, correlation, and the candidate's identity — algorithm,
  seed(s), scrub, pageStart, viewport. The row's colour follows the
  same green → amber → red ramp as the curve. Each row also shows the
  validation score C_FPSR and its three components (density,
  tightness, horizon) from §3.1, so the human verification is visible
  next to the search ranking.
- **Deduplication:** near-duplicate candidates (same seed, scrub within
  a few frames of each other) should collapse to the best of the group,
  so the list is 10 genuinely different fits, not 10 framings of one.
  Exact dedup rule is an open question.
- **Empty / no-data state:** with no dataset loaded the search is
  disabled — there is nothing to score against. (In no-data mode scrub
  and scroll are already indistinguishable on the plot; searching there
  would be meaningless.)

## 6. Click-to-jump

Clicking a row restores the candidate exactly and jumps the timeline
to it. The state to restore is exactly the `best` object the page's
existing "Mark these settings as best so far" button already stores:

- algorithm + all panel parameters (including the seed(s)),
- scrub, pageStart, viewport,
- fit weights / tolerances (wX, wY, tolX, epsY),
- mapping key (stream / X-means / Y-means / jump rule).

Implementation is therefore: set the panel inputs to the stored values,
set scrub / pageStart / viewport, call the existing `refresh()`. The
plot, readouts, diagnostics and curve colour then show that candidate
with no new rendering code. The row stays highlighted so Patrick can
see which entry he is looking at, and "Mark as best" / the list can
share the same stored object.

## 7. Open decisions for Patrick

Decided (see the notes): "by time" = X loss, "by value" = Y loss,
"integrated" = combined loss (confirmed 18 Nov 2026); seed sweep first,
parameter sweep as Phase 2 (§4.3); pageStart fixed at the current window
in v1. Still open:

1. Which seed field varies in v1? seedInner only (others frozen), or
   seedInner + seedOuter together? QS has two stream offsets; BD has
   valueSeedOffset — same question per algorithm.
2. Default k (10 proposed) and default budget (candidates / seconds)?
3. Coarse-to-fine proxy scoring in v1, or full scoring for every
   candidate and accept the slower search? Note the train alignment in
   §4.1 already provides a cheap first pass (O(jumps)); the question
   is now whether its timing signal alone can rank candidates before
   the full `scoreFit`, or whether every candidate gets the full score.
4. Dedup rule for near-identical candidates in the top-k list?
5. Stretch length for inflection-anchored matching (§4.1): the next K
   jumps, or the next W frames — and what K / W?
6. Phase 2 seed-space bound (§4.3): what bounds the input coordinate
   seed space when the parameter sweep multiplies it — a seed interval,
   a sampled subset, a count cap?
7. Soft-hit smoother (§4.4): adopt the Gaussian soft score as
   the Phase 2 hill-climb objective, with σ annealed wide → narrow?
   (Ranking still on the exact scores — that part is not in
   question.)
8. C_FPSR as the default sort for the top-k lists (§3.1, §5), or
   keep the combined loss as the default sort and show C_FPSR as a
   column?

## 8. What this does not do

- It does not exhaust 2^64, and it must never claim to. The honest
  claim is: it samples the space FPS-R makes samplable, and ranks what
  it finds by the scores already defined.
- It does not change any score definition, and it does not add a new
  fit mode. Value vs hold is still the same X / Y split.
- It does not run server-side or send data anywhere: same local-only
  model as the file picker / drag-and-drop / paste loader.

## 9. Considered and rejected — the ML spectrometer (18 Nov 2026)

Before this spec settled on search, the book proposed a machine
learning model, the *FPS-R Spectrometer*: an inverse solver that
would ingest a dataset and directly predict the engine, seed,
phase, Δmin and Δmax. It was culled on 18 Nov 2026, after reasoning
it through, for one decisive reason: predicting the seed would
require learning the pattern within splitmix64, and there is no
pattern there to learn — the mixer avalanches by design, so a
one-bit seed change produces an unrelated curve, with no
neighbourhood, no gradient and no local structure for learning to
grip. The learnable part of the problem (the smooth statistical
correlations — longer plateaus mean larger min/max hold) is already
given directly by the User-data profile, and the only way a model
could "know" the right seed is to have memorised it, which across
a 2^64 seed space crossed with the parameter space is hoping for
an overfit. A closed-loop RL variant (guess → run FPS-R → loss →
correct) is still learned trial-and-error fuzzing: at use time it
must still propose, evaluate and correct, and its edge is confined
to the smooth dimensions, where coarse-to-fine already works
transparently. The search is the honest tool here — every candidate
it returns is a real (engine, params, seed) triple with an exact,
reproducible, inspectable score. What survived the cull: the
weighting framework (already in the explorer) and the validation
metric C_FPSR (§3.1), both ML-independent.
