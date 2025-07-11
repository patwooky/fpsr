# FPS-R Technical Documentation
##### This documentation is still in development. While every update strives to be accurate, there will be parts that are incomplete or inaccurate. 

# Table of Contents

- [The Purpose of This Document](#the-purpose-of-this-document)
  - [A Word of Thanks](#a-word-of-thanks)
  - [⚖️ Attribution Note](#️-attribution-note)
  - [👥 Who This Document Is For](#-who-this-document-is-for)
- [Algorithmic Detail, Code Structure, Usage Notes](#algorithmic-detail-code-structure-usage-notes)
  - [🧾 Code Snippets Provided in this Repository](#-code-snippets-provided-in-this-repository)
- [🔩 How FPS-R Works](#-how-fps-r-works)
- [⚙️ Features of FPS-R](#️-features-of-fps-r)
  - [🧳 Stateless](#-stateless)
  - [🧩 Deterministic](#-deterministic)
- [🌀 How It Works: Stacked Modulo (SM)](#-how-it-works-stacked-modulo-sm)
  - [Core Mechanism](#core-mechanism)
  - [Behavior](#behavior)
- [✴ How It Works: Quantised Switching (QS)](#-how-it-works-quantised-switching-qs)
  - [Core Mechanism](#core-mechanism-1)
  - [Behavior](#behavior-1)
- [Combining FPS-R Algorithms](#combining-fps-r-algorithms)
  - [FPS-R Algorithms Working Together](#fps-r-algorithms-working-together)
  - [FPS-R Algorithms Working with Other Algorithms](#fps-r-algorithms-working-with-other-algorithms)
- [Show Me the Code!](#show-me-the-code)
- [Stacked Modulo (SM) - Mathematical Model](#stacked-modulo-sm---mathematical-model)
- [Stacked Modulo (SM) - Code](#stacked-modulo-sm---code)
  - [One-Line Compact](#one-line-compact)
  - [🧩 Component Breakdown](#-component-breakdown)
  - [Stacked Modulo - A Defined Function](#stacked-modulo---a-defined-function)
  - [Behavior of the Stacked Modulo Function](#behavior-of-the-stacked-modulo-function)
- [Quantised Switching (QS) - Mathematical Model](#quantised-switching-qs---mathematical-model)
- [Quantised Switching (QS) - Code](#quantised-switching-qs---code)

---
# The Purpose of This Document
This document explains and explores the algorithms that constitute **Frame-Persistent Stateless Randomisation (FPS-R)**. It breaks down the mechanics of its modulation logic—revealing how structure, randomness, and phrasing combine to form behavior that feels intentional, yet requires no memory.

### A Word of Thanks
Thank you for taking the time to explore these notes on the algorithm and its inner workings. Whether you’re here out of curiosity, critique, or collaboration—welcome.

#### ⚖️ Attribution Note 
FPS-R is released under the MIT License, which means you're free to use it in commercial, private, and artistic projects without restriction.

In many contexts—SaaS platforms, embedded systems, or creative outputs like VFX shots, motion design, and procedural soundscapes—the code never ships. In those cases, attribution isn’t legally required.

But if FPS-R helped phrase the blink in your commercial, the irregular glitch in your shader, or the drift in your kinetic sculpture—it would mean a lot to be credited. Even a quiet mention in your documentation or acknowledgments helps others discover the grammar behind your motion.

> The code may stay invisible. But if the phrasing speaks? Give it a name.

### 👥 Who This Document Is For
This document is for anyone who wants to see what lives behind the phrasing.

🧑‍💻 Developers will find structured breakdowns of how randomness, modularity, and statelessness combine—ready for implementation across timelines, shaders, or signal streams.

🎨 Designers and motion artists who have used FPS-R instinctively can now peek behind the curtain—understanding what shaped the gestures they've phrased.

🧠 Thinkers, tinkerers, and curious system-builders are welcome too. If you’ve ever wondered how behavior can breathe without memory—this is your invitation to explore.

> There’s no pre-requisite here—just an interest in what modulation can do when left to its own logic.

Come as you are. Stay as long as you like. The phrasing engine is always running.

### Algorithmic Detail, Code Structure, Usage Notes
In this document, we will unpack the FPS-R methods by deconstructing their code into modular components. Each section examines how a fragment contributes to the resulting phrasing—allowing you to understand, modulate, and compose behavior with precision.

### 🧾 Code Snippets Provided in this Repository
The code examples in this repository are **platform-conscious**, not platform-specific. Each snippet has been deliberately shaped for **clarity of intent**, avoiding language-dependent operators (like ternaries), environment-specific idioms, or dependency-bound functions. Where expressive quirks exist (e.g. modulus, quantisation rhythms), they are surfaced with **plain logic and comments that explain the phrasing**, not just the math.

These implementations prioritize **readability, reproducibility, and minimal refactoring cost** across most C-family and expression-bound languages—including C++, Java, JavaScript, GLSL, HLSL, MEL, and Houdini VEX.

While some language-specific adjustment may still be necessary—such as:
- Array declarations ([] vs {} syntax)
- Math operations (floor, mod, frac) depending on standard library access
- Type casting and rounding functions

…the core logic is designed to port cleanly, and has been tested for deterministic phrasing integrity across platforms.

> These snippets are not drop-in libraries—they're reference phrasing kernels. Use them to reconstruct modulation logic in your language of choice, knowing that the behavior should survive the translation.

## 🔩 How FPS-R Works

At its core, FPS-R operates through **coordinate-indexed determinism**. Whether evaluated over time (e.g., `$F`, `@Frame`) or space (`@P`, `uv`, `x`), it applies arithmetic logic—such as `mod()` cycles, `floor()` binning, and seeded `rand()` functions—to produce values that appear to hold, snap, or jump across defined intervals.

Despite its expressive output, FPS-R is strictly **stateless**. Each evaluation is contextually local: it derives its result solely from the current coordinate, without referencing previous frames or adjacent spatial samples.

This yields a surprising property: **discontinuities feel intentional**, and regions of apparent memory emerge—without any simulation or history tracking. It's a sleight of hand through math:  
**perceived temporal coherence from purely evaluative logic.**

---

## ⚙️ Features of FPS-R

Understanding how FPS-R achieves its behavior requires a closer look at what we mean by "stateless" and "deterministic":

### FPS-R Seems to Remember
Due to how the algorithms are formulated and constructed, FPS-R algorithms behave like they remember. This means that a random value is held for a number of frames before "moving on" to the next random value. Before FPS-R, this behaviour is usually achieved through stateful methods. If the result of FPS-R is shown to a programmer or technical artist for the first time, he or she would probably believe this to be coming from a stateful technique that involves remembering that random number from the previous frame and holding it the value with a counter until the next reseed.

This is the foundation of what we've come to call:
> **🪞 Memoryless Mimicry**
> A simulation remembers so it can anticipate. FPS-R forgets, but still manages to feel like it remembers.

### 🧳 FPS-R is Stateless

Stateful systems rely on memory—they accumulate, simulate, or propagate values over time. In contrast, **FPS-R is stateless**: it evaluates in-place, without any dependency on past or future frames.

A typical FPS-R expression evaluates a jump or hold using only the current frame index and seeded math functions. No buffers. No temporal recursion. Just present-tense logic that _feels_ retrospective.

This quality makes FPS-R not only performant, but also **portable and resilient**. Its behavior can be trusted in multi-threaded, parallel, or distributed contexts without simulation overhead.

### 🧩 FPS-R is Deterministic

With the same inputs—frame number, seed parameters, and method—FPS-R always produces the same output. This deterministic footprint enables:

- 🧪 Reproducible behavior across simulations, tests, or procedural evaluations
- 🛠️ Reliable debugging and tuning—behavior is traceable and consistent
- 🎛️ Composable structure—multiple FPS-R layers can interact without uncertainty
- 📈 Cross-domain applicability—whether in robotics, interaction design, motion synthesis, or data-driven generative tools, repeatability ensures trust
- 🧠 Expressive layering: deterministic scaffolds enable deliberate rhythm clashes, controlled glitches, and predictable emergent timing behaviors between the signal systems

### FPS-R Plays Well with Others
#### Playing Well with Everybody
As a modulation framework that adds to the behavior of larger systems, it can create its deterministic move-and-hold behaviour in both stateful and stateless environments.

#### Playing Better with Stateless Operators
The true power of FPS-R’s stateless determinism emerges when paired with other stateless systems. In such a case, **the final result will be equally stateless**.

> The section “**Combining FPS-R Algorithms**” explores how these principles can be layered, stacked, and merged with other systems to unlock deeper phrasing behavior.

---

## 🌀 How It Works: Stacked Modulo (SM)
SM uses **layered modulus operations** combined with shifting `rand()` seeds to create output that seems to "hold" values across multiple frames or spatial coordinates. The result: a pattern of persistent values interrupted by unexpected jumps.

### Core Mechanism
The Stacked Modulo (SM) method generates its unique "move-and-hold" rhythm through a nested, procedural process. Instead of using fixed holding zones, it creates a variable-length rhythm where the duration of each hold is itself determined by a nested FPS-R pattern.
1. **Procedural Hold Duration**  
   At the core of the function, a simple, high-frequency FPS-R pattern generates a random value. This value is then used to calculate an adaptive hold duration, H, that falls between a defined `minHold` and `maxHold`. This `H` value itself holds steady for a short period before being re-randomized.
2. **Variable-Length Modulo Cycle**
   This procedurally generated hold duration, `H`, is then used as the modulus in the main timing operation (`frame % H`). This creates a primary rhythmic cycle whose length is not constant but changes dynamically over time.

3. **Phase-Shifted Seeding**
   The result of this variable-length modulo is used to create a phase-shifted offset. This offset is subtracted from the main `frame` counter to produce a final, complex seed. It is this constantly shifting, rhythmically inconsistent seed that is fed into the final `rand()` function to produce the output.

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

### Behaviour
- Acts like a **switcher**: swaps logic branches, colour palettes, or procedural regimes  
- (Time) Ideal for glitch logic, state machines, or recursive style variation
- (Space) Ideal for creating procedural geometrical structures.

### Advanced Forms and Features
The form of the supplied QS function is only one of the possible forms it can take on. QS approaches stateless randomness differently—by layering continuous signal streams. When quantising and randomising stages are removed, it unlocks richly varied and often surprising behaviours. 

These features can also serve as debugging or visualising tools to reveal the characteristics of the streams of signals underneath to observe how it works before turning them off again for more controlled sculpting.
- Can bypass `rand()` entirely to create *structured switching* revealing the quantised signal streams—stepped but coherent—behind the `rand()` operation.
- Can bypass quantisation or posterisation entirely to reveal the original coherent continuous streams that feed the switcher. This will then become a very regular switching jump between 2 continuous signals.
- Can swap out the default sine wave streams for any signal generating function (noise, look-up array) to achieve completely different behaviours (in the temporal domain), and different aesthetic looks (in the spatial domain).

> QS is not about rhythm—it's about **selection**. And it works beautifully when combined with SM to modulate *when* changes happen, and *what* they reveal.

---
## Combining FPS-R Algorithms
### FPS-R Algorithms Working Together
FPS-R methods can be stacked, nested, or intertwined to unlock even richer phrasing behaviors.
- A Quantised Switching (QS) output can be fed into one or both frame inputs of a Stacked Modulo (SM) instance—overlaying unpredictable switching on top of rhythmic structure.
- Algorithms can also be self-nesting: QS driving QS, SM shaping SM—allowing phrasing to modulate itself recursively.

> Even when layered, the resulting motion remains deterministic, stateless, and reproducible. The phrasing may feel unpredictable—but it will always remember how it moved.

### FPS-R Algorithms Working with Other Algorithms
FPS-R plays well with others. As long as the external algorithms are **stateless and deterministic**, their outputs can be safely embedded or layered into FPS-R phrasing systems without breaking reproducibility.

Below is a non-exhaustive list of common stateless functions and procedural sources:

| Category | Stateless Function Examples |
| :--- | :--- |
| **Mathematical & Trigonometric** | `sin`, `cos`, `pow`, `sqrt`, `floor`, `ceil`, `round`, `min`, `max`, `clamp`, `abs`, `mod`, `frac`, `lerp`, `smoothstep` |
| **Procedural Noise & Patterns** | `Perlin Noise`, `Simplex Noise`, `Worley Noise (Voronoi)`, `Value Noise`, `Fractal Brownian Motion (fBm)` |
| **Hashing & Pseudo-Random** | `Seeded rand(seed)`, various hash functions that convert coordinates or seeds into repeatable numbers |
| **Geometric & Coordinate-Based**| `dot`, `cross`, `normalize`, `distance`, `length`, `SDFs (Signed Distance Functions)`, `Texture Lookups` |
| **Logic & Data Manipulation** | `step`, `mix`, `Bitwise Operations (AND, OR, XOR)` |

> ⚠️ FPS-R can coexist with non-deterministic or stateful sources, but the resulting phrasing will not be traceable or reproducible. That may be an intentional choice—but it's a choice worth naming.

---
## Show Me the Code!
Implementation of the algorithms in a variety of software and environments. 

[Code Implementations](../code/): (a directory) Here is a high level directory in this repository that contains the code implemented in several languages and platforms.

### C
[**Code in C**](../code/c/fpsr_algorithms.c): FPS-R SM and QS in portable C code that would run with minimal modifications in many c-style languages. 

### Python
[**Code in Python**](../code/python/fpsr_algorithms.py): FPS-R SM and QS in a Python `.py` file.

### Jupyter Notebook
[**Code in Jupyter Notebook**](../code/python/fpsr_algorithms.ipynb): FPS-R SM and FPS-R QS Python code in notebook cells, in a visually pleasant layout. For the most intuitive and hands-on exploration, the Jupyter Notebook provides interactively scrollable graphs. This is the recommended way to visually understand the characteristics and "fingerprint" of each algorithm's output.
<img src="../readme/images/jpynotebookFpsrSmScroll.gif" alt="FPS-R-SM Timeline Graph Preview" width="350" height="150">
FPS-R: Stacked Modulo Timeline Graph Preview
<img src="../readme/images/jpynotebookFpsrQsScroll.gif" alt="FPS-R-QS Timeline Graph Preview" width="350" height="150">
FPS-R: Quantised Switching Timeline Graph Preview
> Note: Jupyter notebooks render only as static content on GitHub's web viewer. Interactive scrolling graphs for SM and QS will not show up. If you want to play around with the parameters and drive a different resulting curve, and inspect the scrolling graphs, please feel free to download the notebook and execute it on your local machine runnning Jupyter notebook on a Python 3.x kernal with the relevant dependencies (`Pandas` and `Matplotlib`). 

To access the read-only notebook with the interactive scrolling graphs, you can: [explore the interactive timeline in a Jupyter notebook on `nbviewer`](https://nbviewer.org/github/patwooky/FPSR_Algorithm/blob/main/resources/code/python/fpsr_algorithms.ipynb)
The interactive scrolling graphs are the last 2 cells at the end of the notebook.

### Houdini
[**Houdini `.hip` File**](../code/houdini/h_fpsr_code_v001_01.hip): This is a Houdini project file that has a geometry node. In it there are two `point wrangle` nodes that provide `FPS-R: SM` and and `FPS-R: QS`. Both will produce a FPS-R signal to drive the y-axis of the position of a box.
<img src="../code/houdini/h_fpsr_code_v001_01.gif" alt="'hip' file" width="134" height="157">

---

## Stacked Modulo (SM) - Mathematical Model
The output is a function of a composite seed, which is the sum of multiple layered rhythm functions.
<!-- latex markdown -->
$$
Seed(t) = \sum_{i=1}^{n} \left\lfloor \frac{t}{P_i} + O_i \right\rfloor
$$
$$
f(t)_{\text{SM}} = \text{rand}\left(Seed(t)\right)
$$
**$f(t)$** is the final output of FPS-R:SM.
Where $P\_i$ is the period and $O\_i$ is the phase offset for the $i$-th rhythm layer.

## Stacked Modulo (SM) - Code
### One-Line Compact
This is Stacked Modulo in a nutshell—a simple single-line that tells the whole story. 
```c 
frame - (23 + frame % (minHold + floor(rand(23 + frame - (frame % 10)) * (maxHold - minHold))))
```
At the heart, FPS-R:SM is a temporal modulation function, where the output adjusts the current frame value in a structured-random way. Let’s unpack it inside-out:

#### 🧩 Component Breakdown
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

### Stacked Modulo - A Defined Function
Here is a function defined in C that goes beyond the compact single-line code. It abstracts the expression into a function with parameters that can be tweaked and controlled. This should be portable across languages and platforms.
```c
/**
 * @file fpsr_algorithms.c
 * @brief Portable C implementation of FPS-R: Stacked Modulo (SM) algorithm
 * @details This file contains the stateless, frame-persistent randomization Stacked Modulo algorithm.
 * It uses a custom portable_rand() function to ensure deterministic and
 * consistent results across any platform.
 */

#include <math.h> // For sin() and floor()
#include <stdio.h> // For NULL

/**
 * A simple, portable pseudo-random number generator.
 * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large prime numbers are used to create a chaotic, unpredictable result.
    // The frac() part (or fmod(x, 1.0)) ensures the result is in the [0, 1) range.
    float result = sin((float)seed * 12.9898) * 43758.5453;
    return result - floor(result);
}


/******************************************************************************/
/* FPS-R: Stacked Modulo (SM)                            */
/******************************************************************************/

/**
 * @brief Generates a persistent random value that holds for a calculated duration.
 * @details This function uses a two-step process. First, it determines a random
 * "hold duration". Second, it generates a stable integer for that duration,
 * which is then used as a seed to produce the final, held random value.
 *
 * @param frame The current frame or time input.
 * @param minHold The minimum duration (in frames) for a value to hold.
 * @param maxHold The maximum duration (in frames) for a value to hold.
 * @param reseedInterval The fixed interval at which a new hold duration is calculated.
 * @param seedInner An offset for the random duration calculation to create unique sequences.
 * @param seedOuter An offset for the final value calculation to create unique sequences.
 * @return A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
float fpsr_sm(
    int frame, int minHold, int maxHold,
    int reseedInterval, int seedInner, int seedOuter)
{
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    float rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval));
    int holdDuration = (int)floor(minHold + rand_for_duration * (maxHold - minHold));

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // This value is constant for the entire duration of the hold.
    int held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value ---
    // Because the seed is stable, the final value is also stable.
    float final_held_value = portable_rand(held_integer_state);

    return final_held_value;
}
```

#### A Sample call to the FPS-R:SM function
```c
// Parameters
int frame = 103; // provides the current frame
int minHoldFrames = 16; // probable minimum held period
int maxHoldFrames = 24; // maximum held period before cycling
int reseedFrames = 9; // inner mod cycle timing
int offsetInner = -41; // offsets the inner frame
int offsetOuter = 23; // offsets the outer frame

// Call the FPSR function
float randVal = 
    fpsr_sm(
        int(frame), minHoldFrames, maxHoldFrames, 
        reseedFrames, offsetInner, offsetOuter);
float randVal_previous = 
    fpsr_sm(
        int(frame-1), minHoldFrames, maxHoldFrames, 
        reseedFrames, offsetInner, offsetOuter);
int changed = 0;
if (randVal != randVal_previous) {
    changed = 1; // value has changed from the previous frame
}
```

### Behaviour of the Stacked Modulo Function
The expressive range of the Stacked Modulo (SM) method is controlled by the selection of its core timing parameters. The relationship between cycle lengths and their interaction dictates the character of the output signal.

**High-Frequency Modulation (Twitch & Impulse)**
When the modulo spans (P_i in the formula) are set to small values, the individual rhythm layers complete their cycles at a high frequency. This causes the composite Seed(t) to change rapidly and frequently. The resulting output is a high-frequency signal with short hold durations, producing a behavior that feels twitchy, alert, or like stochastic noise.

**Low-Frequency Modulation (Hesitation & Deliberation)**
Conversely, using large values for the modulo spans results in long cycle lengths. The Seed(t) remains stable for extended periods, only changing when one of the slow-moving layers completes its long cycle. This creates significant temporal stability, where a single random value is held for a long duration before jumping. This behavior is perceived as hesitation, deliberation, or a state-like persistence.

**Rhythmic Interference (Emergent Logic)**
The true complexity emerges from layering multiple modulo functions with non-harmonious periods (e.g., using prime numbers like 13, 31, 97). The cycles of these layers go in and out of phase at irregular intervals. A "jump" in the final output is triggered whenever any of the layers completes its cycle, creating a complex interference pattern. This composite rhythm produces state-like transitions that are not explicitly programmed, mimicking the emergent logic of a complex state machine without storing any state.

---
## Quantised Switching (QS) - Mathematical Model
The output is determined by a selector function choosing from a set of source functions.
<!-- latex markdown -->
$$
Selector(t) = \left\lfloor \frac{t}{P_s} \right\rfloor \pmod{N}
$$
$$
f(t)_{\text{QS}} = \text{Source}_{Selector(t)}(t)
$$
Where $P\_s$ is the period of the selector and N is the number of available sources.

## Quantised Switching (QS) - Code
Here is a function defined in C with parameters that can be tweaked and controlled. This should be portable across languages and platforms.

```c
/**
 * @file fpsr_algorithms.c
 * @brief Portable C implementation of FPS-R: Quantised Switching (QS) algorithm
 * @details This file contains the stateless, frame-persistent randomization Quantised Switching algorithm.
 * It uses a custom portable_rand() function to ensure deterministic and
 * consistent results across any platform.
 */

#include <math.h> // For sin() and floor()
#include <stdio.h> // For NULL

/**
 * A simple, portable pseudo-random number generator.
 * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large prime numbers are used to create a chaotic, unpredictable result.
    // The frac() part (or fmod(x, 1.0)) ensures the result is in the [0, 1) range.
    float result = sin((float)seed * 12.9898) * 43758.5453;
    return result - floor(result);
}

/******************************************************************************/
/* FPS-R: Quantised Switching (QS)                         */
/******************************************************************************/

/**
 * @brief Generates a flickering, quantised value by switching between two sine wave streams.
 * @details This function creates two separate, quantised sine waves and switches
 * between them at a fixed interval to create complex, glitch-like patterns.
 *
 * @param frame The current frame or time input.
 * @param baseWaveFreq The base frequency for the modulation wave of stream 1.
 * @param stream2FreqMult A multiplier for the second stream's frequency. If < 0, a default is used.
 * @param quantLevelsMinMax An array of two integers for the min and max quantisation levels.
 * @param streamsOffset An array of two integers to offset the frame for each stream.
 * @param streamSwitchDur The number of frames after which the streams switch. If < 1, a default is derived.
 * @param stream1QuantDur The duration for stream 1's quantisation switch. If < 1, a default is derived.
 * @param stream2QuantDur The duration for stream 2's quantisation switch. If < 1, a default is derived.
 * @return A pseudo-random float value between 0.0 and 1.0.
 */
float fpsr_qs(
    int frame, float baseWaveFreq, float stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2],
    int streamSwitchDur, int stream1QuantDur, int stream2QuantDur)
{
    // --- 1. Set default durations if not provided ---
    // This pattern allows for optional parameters in a portable C-style.
    if (streamSwitchDur < 1) {
        streamSwitchDur = (int)floor((1.0 / baseWaveFreq) * 0.76);
    }
    if (stream1QuantDur < 1) {
        stream1QuantDur = (int)floor((1.0 / baseWaveFreq) * 1.2);
    }
    if (stream2QuantDur < 1) {
        stream2QuantDur = (int)floor((1.0 / baseWaveFreq) * 0.9);
    }
    // Ensure durations are at least 1 frame to prevent division by zero.
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate quantisation levels for each stream ---
    // The quantisation level itself switches halfway through its own duration cycle.
    int s1_quant_level;
    if ((streamsOffset[0] + frame) % stream1QuantDur < stream1QuantDur / 2) {
        s1_quant_level = quantLevelsMinMax[0];
    } else {
        s1_quant_level = quantLevelsMinMax[1];
    }

    int s2_quant_level;
    // Magic numbers are used to create more variation in the second stream's character.
    // Change these to affect a different look.
    const float STREAM2_QUANT_RATIO_MIN = 1.24;
    const float STREAM2_QUANT_RATIO_MAX = 0.66;
    if ((streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur / 2) {
        s2_quant_level = (int)floor(quantLevelsMinMax[0] * STREAM2_QUANT_RATIO_MIN);
    } else {
        s2_quant_level = (int)floor(quantLevelsMinMax[1] * STREAM2_QUANT_RATIO_MAX);
    }
    // Ensure quantisation levels are at least 1.
    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }


    // --- 3. Generate the two quantised sine wave streams ---
    if (stream2FreqMult < 0) { stream2FreqMult = 3.7; } // Default multiplier.

    float stream1 = floor(sin((float)(streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / s1_quant_level;
    float stream2 = floor(sin((float)(streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / s2_quant_level;

    // --- 4. Switch between the two streams ---
    float active_stream_val;
    if ((frame % streamSwitchDur) < streamSwitchDur / 2) {
        active_stream_val = stream1;
    } else {
        active_stream_val = stream2;
    }

    // --- 5. Hash the final output to create a random-looking value ---
    // The stepped sine wave output is converted to a large integer and used
    // as a seed to produce the final, held random value.
    return portable_rand((int)(active_stream_val * 100000.0));
}
```

