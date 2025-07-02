# FPS-R Technical Documentation

# Table of Contents


---
# The Purpose of This Document {#ThePurpose}
This document will explain and explore the algorithms that make up the **Frame-Persistent Stateless Randomisation (FPS-R)**.

### Algorithmic Detail, Code Structure, Usage Notes {#AlgoDetail}
In this document I will explain the FPS-R algorithms by breaking down the code structure into their respective components.  
From the code fragments we will see how each component affects the resulting signal.


## 🔩 How FPS-R Works {#HowFPSRWorks}

At its core, FPS-R operates through **coordinate-indexed determinism**. Whether evaluated over time (e.g., `$F`, `@Frame`) or space (`@P`, `uv`, `x`), it applies arithmetic logic—such as `mod()` cycles, `floor()` binning, and seeded `rand()` functions—to produce values that appear to hold, snap, or jump across defined intervals.

Despite its expressive output, FPS-R is strictly **stateless**. Each evaluation is contextually local: it derives its result solely from the current coordinate, without referencing previous frames or adjacent spatial samples.

This yields a surprising property: **discontinuities feel intentional**, and regions of apparent memory emerge—without any simulation or history tracking. It's a sleight of hand through math:  
**perceived temporal coherence from purely evaluative logic.**

---

## ⚙️ Features of FPS-R {#FeaturesofFPSR}

Understanding how FPS-R achieves its behavior requires a closer look at what we mean by "stateless" and "deterministic":

### 🧳 Stateless {#Stateless}

Stateful systems rely on memory—they accumulate, simulate, or propagate values over time. In contrast, **FPS-R is stateless**: it evaluates in-place, without any dependency on past or future frames.

A typical FPS-R expression evaluates a jump or hold using only the current frame index and seeded math functions. No buffers. No temporal recursion. Just present-tense logic that _feels_ retrospective.

This quality makes FPS-R not only performant, but also **portable and resilient**. Its behavior can be trusted in multi-threaded, parallel, or distributed contexts without simulation overhead.

### 🧩 Deterministic {#Deterministic}

With the same inputs—frame number, seed parameters, and method—FPS-R always produces the same output. This deterministic footprint enables:

- 🧪 Reproducible behavior across simulations, tests, or procedural evaluations
- 🛠️ Reliable debugging and tuning—behavior is traceable and consistent
- 🎛️ Composable structure—multiple FPS-R layers can interact without uncertainty
- 📈 Cross-domain applicability—whether in robotics, interaction design, motion synthesis, or data-driven generative tools, repeatability ensures trust
- 🧠 Expressive layering: deterministic scaffolds enable deliberate rhythm clashes, controlled glitches, and predictable emergent timing behaviors between the signal systems

This is the foundation of what we've come to call:

> #### 🪞 Memoryless Mimicry {#MemorylessMimicry}  
> A simulation remembers so it can anticipate. FPS-R forgets, but still manages to feel like it remembers.


---

## 🌀 How It Works: Stacked Modulo (SM)

SM uses **layered modulus operations** combined with shifting `rand()` seeds to create output that seems to "hold" values across multiple frames or spatial coordinates. The result: a pattern of persistent values interrupted by unexpected jumps.

### Core Mechanism

#### One-Line Compact

1. **Primary Modulus Control**  
   The input coordinate (e.g. `@Frame`, `x`, or `uv.x`) is divided by a tunable modulus (e.g. `mod($F, 24)`), which segments the timeline or space into consistent-sized “holding zones.”

2. **Randomised Value per Zone**  
   Each segment outputs a deterministic `rand()` seeded by its segment index:
   ```c
   rand(floor($F / 24) + seed)
   ```

3. **Stacking for Complexity** 
    Multiple mod stages can be layered to create semi-regular but interfering hold patterns. Each layer contributes a different "rhythm," producing a combined signal that feels structured but unpredictable.
    - Think rhythmic polyrhythms or layered tectonic plates.
    - Outputs “memory without memory,” tuned by frequency and phase.

### Behavior
- Short mod spans → twitchy impulse
- Long mod spans → hesitant deliberation
- Layered mods → emergent switching logic, without simulation


---

## ✴ How It Works: Quantised Switching (QS)

While SM focuses on **temporal rhythm**, QS enables **value switching** across discrete options—like glitch matrices, logic multiplexers, or stylistic gates. QS can use any input signal (not just random) and quantise it into hold states.

### Core Mechanism

1. **Value Bank**  
   Define a set of discrete outcomes—values, palettes, animation states, functions.

2. **Signal Lookup**  
   Use a deterministic signal (random or procedural) to select an index:  
   `int index = floor(rand(@Frame) * numOptions);`  
   `output = valueBank[index];`

3. **Optional Quantisation**  
   Apply stepped or smoothed quantisation to the input signal to make transitions feel chunky, abrupt, or smooth.

### Behavior

- Acts like a **switcher**: swaps logic branches, colour palettes, or procedural regimes  
- Can bypass `rand()` entirely to create *structured switching* 
- Can bypass quantisation or posterisation entirely to reveal the original coherent continuous streams that are feeding the switcher.
- Can swap out the default sine waves streams for any signal generating function (noise, look-up array) to achieve completely different behaviours (in the temporal domain), and different aesthetic looks (in the spatial domain).
- (Time) Ideal for glitch logic, state machines, or recursive style variation
- (Space) Ideal for creating procedural geometrical structures.

> QS is not about rhythm—it's about **selection**. And it works beautifully when combined with SM to modulate *when* changes happen, and *what* they reveal.
