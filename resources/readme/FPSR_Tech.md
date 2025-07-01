# FPS-R Tech Documentation

## Algorithmic Detail, Code Structure, Usage Notes


## 🔩 How FPS-R Works

At the heart of FPS-R is **coordinate-indexed determinism**: whether working in time (`@Frame`, `$F`) or space (`x`, `uv`, `position`), FPS-R uses arithmetic logic—modulo cycles, quantisation bins, and deterministic `rand()` calls—to generate values that appear to "hold" across ranges.  

There is no state stored. Each evaluation is independent. Yet, what emerges feels intentional: persistent regions, jumpy intervals, and layered memory illusions.  
It’s a sleight of hand using math: *perceived continuity without simulation.*

---

## 🌀 How It Works: Stacked Modulo (SM)

SM uses **layered modulus operations** combined with shifting `rand()` seeds to create output that seems to "hold" values across multiple frames or spatial coordinates. The result: a pattern of persistent values interrupted by unexpected jumps.

### Core Mechanism

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

> QS is not about rhythm—it’s about **selection**. And it works beautifully when combined with SM to modulate *when* changes happen, and *what* they reveal.
