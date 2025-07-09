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

### Stacked Modulo - A Defined Function
Here is a function defined C that goes beyond the compact single-line code. It abstracts the expression into a function with parameters that can be tweaked and controlled. This should be portable across languages and platforms.
```c
// Frame-Persistent Stateless Randomisation (Stacked Modulo)

// A simple, portable pseudo-random number generator that takes an integer seed.
// Different languages have different rand() implementations, so using a custom
// one like this ensures identical results on any platform.
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large number can be any arbitrary large float.
    return frac(sin(float(seed) * 12.9898) * 43758.5453);
}

/**
    Frame-Persistent Stateless Randomisation (Stacked Modulo)
    int frame The current frame or time input.
    int minHold The minimum duration for a value to hold.
    int maxHold The maximum duration for a value to hold.
    int reseedInterval The fixed interval at which a new duration is calculated.
    int offsetInner An inner offset to create unique random sequences.
    int offsetOuter An outer offset to create unique random sequences.
    returns A float value between 0.0 and 1.0 that holds for a random duration.
**/
float fpsr_sm(
    int frame, int minHold, int maxHold, 
    int reseedInterval, int offsetInner, int offsetOuter) 
{
    if (reseedInterval < 1) {
        reseedInterval = 1; // at least 1 to prevent division by zero
    }
    // 1. Calculate the random hold duration.
    // We use our portable_rand() function and standard math to replace fit01().
    float rand_for_duration = portable_rand(offsetInner + frame - (frame % reseedInterval));
    int holdDuration = floor(minHold + rand_for_duration * (maxHold - minHold));

    if (holdDuration < 1) {
        holdDuration = 1; // at least 1 to prevent division by zero
    }

    // 2. Generate the stable integer "state" for the hold period.
    int held_integer_state = (offsetOuter + frame) - ((offsetOuter + frame) % holdDuration);

    // 3. Use the stable integer state as a seed for the final random value.
    // This is the key two-step process, now fully encapsulated.
    float final_held_value = portable_rand(held_integer_state);

    return final_held_value;
}

// Parameters
int minHoldFrames = 16; // probable minimum held period
int maxHoldFrames = 24; // maximum held period before cycling
int reseedFrames = 9; // inner mod cycle timing
int offsetInner = -41; // offsets the inner frame
int offsetOuter = 23; // offsets the outer frame

// Call the FPSR function
float randVal = 
    fpsr_sm(
        int(@Frame), minHoldFrames, maxHoldFrames, 
        reseedFrames, offsetInner, offsetOuter);
float randVal_previous = 
    fpsr_sm(
        int(@Frame-1), minHoldFrames, maxHoldFrames, 
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

```c
// FPS-R Quantised Switching (QS)

// A simple, portable pseudo-random number generator that takes an integer seed.
// Different languages have different rand() implementations, so using a custom
// one like this ensures identical results on any platform.
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large number can be any arbitrary large float.
    return frac(sin(float(seed) * 12.9898) * 43758.5453);
}

// FPS-R Quantised Switching (QS)
/*
  returns a normalised 0 to 1 pseudo-random value based on the current frame, 
  base hold frequency, and quantisation levels.
  
  float frame: The current frame number, which is used to generate a pseudo-random value.
  float baseWaveFreq: The base frequency for the modulation wave of stream 1.
  float stream2freqMult: A multiplier for the second stream's frequency, which can be adjusted to create different rhythms.
  int quantLevelsMinMax: An array of two integers for the min and max numbers of quantisation levels for the two streams.
  streamSwitchDur: The number of frames after which the streams switch.
  streamsOffset: An array of two integers that offsets the frame used in quantisation and sine wave for each stream.
  stream1QuantDur: The duration for the first stream's quantisation switch cycle in frames.
  stream2QuantDur: The duration for the second stream's quantisation switch cycle in frames.
*/
float fpsr_qs(int frame, 
    float baseWaveFreq, float stream2freqMult=-1, 
    int quantLevelsMinMax=[1, 10], 
    int streamsOffset=[0, 0], int streamSwitchDur=-1, 
    int stream1QuantDur=-1, int stream2QuantDur=-1)
     
{
    if (streamSwitchDur < 1) {
        // If streamSwitchDur is not provided, derive from baseWaveFreq.
        // default: 76% of inverse of baseWaveFreq.
        streamSwitchDur = floor(1.0 / baseWaveFreq * 0.76);
        if (streamSwitchDur < 1) {
            streamSwitchDur = 1; // Ensure it's at least 1 frame.
        }
    }
    if (stream1QuantDur < 1) {
        // If stream1QuantDur is not provided, derive from baseWaveFreq.
        // default: 120% of inverse of baseWaveFreq.
        stream1QuantDur = floor((1.0 / baseWaveFreq) * 1.2);
        if (stream1QuantDur < 1) {
            stream1QuantDur = 1; // Ensure it's at least 1 frame.
        }
    }
    if (stream2QuantDur < 1) {
        // If stream2QuantDur is not provided, derive from baseWaveFreq.
        // default: 90% of inverse of baseWaveFreq.
        stream2QuantDur = floor((1.0 / baseWaveFreq) * 0.9);
        if (stream2QuantDur < 1) {
            stream2QuantDur = 1; // Ensure it's at least 1 frame.
        }
    }
    
    // Calculate quantised values for the two streams based on their respective durations.
    float s1quantised = 0.0;
    if (int(streamsOffset[0] + frame) % stream1QuantDur < (stream1QuantDur * 0.5)) {
        s1quantised = quantLevelsMinMax[0]; // Use the minimum quantisation level for the first half of the duration.
    } else {
        s1quantised = quantLevelsMinMax[1]; // Use the maximum quantisation level for the second half of the duration.
    }
    s1quantised = floor(s1quantised);
    
    // stream2QuantRatio Min and Max are multipliers for 
    // the quantisation levels for stream 2.
    // These are magic numbers to drive stream 2's to quantise to different levels.
    // These can be adjusted to create different quantisation effects.
    float stream2QuantRatioMin = 1.24;
    float stream2QuantRatioMax = 0.66;
    float s2quantised = 0.0;
    if (int(streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur * 0.5) {
        // Use the minimum quantisation level for the first half of the duration.
        s2quantised = floor(quantLevelsMinMax[0] * stream2QuantRatioMin);
    } else {
        // Use the maximum quantisation level for the second half of the duration.
        s2quantised = floor((quantLevelsMinMax[1] + 0.999) * stream2QuantRatioMax);
    }
    s2quantised = floor(s2quantised);

    // Generate two streams based on sine functions, quantised to the respective levels.
    // A multiplier for the second stream's frequency will create a different rhythm.
    if (stream2freqMult < 0) {
        // If stream2FreqMult is not provided, provide a magic number.
        // default: 3.7 times the baseWaveFreq.
        stream2FreqMult = 3.7; // This multiplier can be adjusted to change the rhythm of the 
    }
    float stream1 = floor(sin(int(streamsOffset[0] + frame)/24 * baseWaveFreq) * s1quantised) / s1quantised;
    float stream2 = floor(sin(int(streamsOffset[1] + frame)/24 * baseWaveFreq * stream2FreqMult) * s2quantised) / s2quantised;
    float outVal = 0.0;
    if ((frame % streamSwitchDur) < streamSwitchDur * 0.5) {
        outVal = stream1; // Use the first stream for the first half of the switch duration.
    } else {
        outVal = stream2; // Use the second stream for the second half of the switch duration.
    }
    outVal = outVal * 2.0 - 1.0; // Convert to range [-1, 1]
    return portable_rand(int(outVal * 100000.0)); // Scale to a larger integer for better distribution.
}

float baseWaveFreq = 0.012; // Base frequency for the modulation wave of stream 1
float stream2freqMult = 3.1; // Multiplier for the second stream's frequency
int quantLevelsMinMax[2] = {12, 22}; // Min, Max quantisation levels for the two streams
int streamsOffset[2] = {0, 76}; // Offset for the two streams
int streamSwitchDur = 24; // Duration for switching streams in frames
int stream1QuantDur = 16; // Duration for the first stream's quantisation switch cycle in frames
int stream2QuantDur = 20; // Duration for the second stream's quantisation switch cycle in frames

float randVal = fpsr_qs(
    frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur);

// another call to fpsr_qs for the previous frame
float randVal_previous = fpsr_qs(
    int(@Frame - 1), baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur);

int changed = 0; // Variable to track if the value has changed
if (randVal != randVal_previous) {
    changed = 1; // Mark as changed if the value has changed from the previous frame
} 
```