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



1. **Primary Modulus Control**  
   A continuously incrementing input coordinate (e.g. `frame`, `x`, or `uv.x`) is divided by a tunable modulus (e.g. `frame % 24`), which segments the timeline or space into consistent-sized “holding zones.”

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

---

## Code - Stacked Modulo (SM)

### One-Line Compact
This is Stacked Modulo in a nutshell—a simple single-line that tells the whole story. 
```c 
frame - (23 + frame % (minHold + floor(rand(23 + frame - (frame % 10)) * (maxHold - minHold))))
```
At the heart, FPS-R:SM is a temporal modulation function, where the output adjusts the current frame value in a structured-random way. Let’s unpack it inside-out:

### 🧩 Component Breakdown
Here’s how the expression works, from the inside out:
1. `(frame % 10)`
   - **What it does:** This calculates the remainder when the current `frame` number is divided by 10.
   - **Observable Outcome:** It produces a simple, repeating sequence of integers: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3...`.
   - **Intent:** This creates a short, rhythmic, 10-frame cycle that acts as the foundational "pacemaker" for the entire system. It defines the smallest unit of time before a _potential_ change can be evaluated.
2. `(23 + frame - (frame % 10))`
   - **What it does:** It subtracts the 10-frame cycle from the current `frame` and adds a prime number offset (`23`).
   - **Observable Outcome:** This calculation effectively quantizes time into 10-frame blocks. For frames 0 through 9, the output is 23. For frames 10 through 19, the output is 33, and so on. The value remains constant for 10-frame intervals.
   - **Intent:** This is the core of the **reseeding mechanism**. By creating a stable value that only changes every 10 frames, it ensures that the random number generator produces the same result for that entire duration, establishing the "hold" phase. The `23` is a "magic number" used to create a unique starting point for the randomness. This ensures that the "outer" `frame` and the "inner" `frame` do not start off at the same time, minimising unexpected accumulated resonance and cancellation effects.
3. `rand(...) * (maxHold - minHold)`
   - **What it does:**  It uses the 10-frame seed to generate a random number between 0.0 and 1.0, then multiplies it by the _range_ of possible hold durations.
   - **Observable Outcome:** A random floating-point number between 0 and (`maxHold` - `minHold`). Because the seed is stable for 10 frames, this value is also stable for that duration.
   - **Intent:** This step calculates a random duration within your specified range.
4. `floor(...)`
   - **What it does:** It truncates the floating-point result from the previous step, converting it into an integer.
   - **Observable Outcome:** A random integer between 0 and (`maxHold - minHold - 1`).
   - **Intent:** This ensures the duration is a whole number, which is cleaner for frame-based calculations.
5. `minHold + floor(...)`
   - **What it does:** It adds the `minHold` value to the random integer.
   - **Observable Outcome:** A final random integer that is guaranteed to be between `minHold` and `maxHold - 1`.
   - **Intent:** It establishes the final, random hold duration. By adding minHold, you enforce a minimum holding time and, most importantly, prevent the duration from ever being zero, which would cause a "divide by zero" error in the outer modulo operation.
6. `frame % (minHold + ...)`
   - **What it does:** The primary **"Stacked Modulo"** operation. The current `frame` is divided by the final, clamped random hold duration, and the remainder is taken.
   - **Observable Outcome:** A sawtooth wave that ramps from 0 up to the hold duration minus one, then resets to 0.
   - **Intent:** This generates the core dynamic that resets or "jumps" when the frame count exceeds the calculated hold period.
7. `frame - (...)`
   - **What it does:** Subtracts the entire ramping value from the current `frame`.
   - **Observable Outcome:** A value that remains constant for the duration determined in step 5, and then jumps to a new constant value.
   - **Intent:** This final step **locks the value**, creating the explicit "hold" state. The subtraction cancels out the frame's increment, resulting in a stable output until the modulo operation triggers a jump.

**Summary of the "Randomised Move-and-Hold" Behavior**
   - **Hold:** For a random number of frames, the expression outputs a constant, unchanging integer. This duration is controlled by `minHold` and `maxHold` parameters, guaranteeing the hold period falls within a specific, use-defined range. This is the "hold" phase, which creates the illusion of a system that is deliberately pausing or waiting.
   - **Jump:** Once the current frame count surpasses the randomly generated hold duration, the `frame % (duration)` operation resets. This causes a sudden, discontinuous "jump" in the final output value.
   - **Reseed:** The seed for the random hold duration is itself updated every 10 frames. This ensures that the system doesn't fall into a simple, repeating loop and that the lengths of the "hold" periods feel unpredictable and organic.

In essence, the expression uses nested, deterministic cycles to create a larger, seemingly random behavior without ever storing information from one frame to the next. By incorporating `minHold` and `maxHold`, it provides direct control over the rhythm of this behavior, perfectly embodying the FPS-R philosophy of generating structured, stateless unpredictability.

### A Defined Function
Here is a function defined C that goes beyond the compact single-line code. It abstracts the expression into a function with parameters that can be tweaked and controlled. This should be portable across languages and platforms.
```c
// A simple, portable pseudo-random number generator that takes an integer seed.
// Different languages have different rand() implementations, so using a custom
// one like this ensures identical results on any platform.
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large number can be any arbitrary large float.
    return fract(sin(float(seed) * 12.9898) * 43758.5453);
}

/**
 * Frame-Persistent Stateless Randomisation (Stacked Modulo)
 *
 * @param frame The current frame or time input.
 * @param minHold The minimum duration for a value to hold.
 * @param maxHold The maximum duration for a value to hold.
 * @param reseedInterval The fixed interval at which a new duration is calculated.
 * @param offset An offset to create unique random sequences.
 * @return A float value between 0.0 and 1.0 that holds for a random duration.
 */
float fpsr_sm(int frame, int minHold, int maxHold, int reseedInterval, int offset) {
    // 1. Calculate the random hold duration.
    // We use our portable_rand() function and standard math to replace fit01().
    float rand_for_duration = portable_rand(offset + frame - (frame % reseedInterval));
    int holdDuration = floor(minHold + rand_for_duration * (maxHold - minHold));

    // Ensure holdDuration is at least 1 to prevent division by zero.
    if (holdDuration < 1) {
        holdDuration = 1;
    }

    // 2. Generate the stable integer "state" for the hold period.
    int held_integer_state = (offset + frame) - ((offset + frame) % holdDuration);

    // 3. Use the stable integer state as a seed for the final random value.
    // This is the key two-step process, now fully encapsulated.
    float final_held_value = portable_rand(held_integer_state);

    return final_held_value;
}

// Parameters
int minHoldFrames = 16; // probable minimum held period
int maxHoldFrames = 24; // maximum held period before cycling
int reseedFrames = 10; // inner mod cycle timing
int offset = 23; // offsets the outer frame

// Call the FPSR function
float randVal = 
    FPSR(int(@Frame), minHoldFrames, maxHoldFrames, reseedFrames, offset);
```