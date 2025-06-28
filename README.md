# 🎲 FPS-R Algorithm: Frame-Persistent Stateless Randomization
### _Stateless unpredictability with a structured soul._

## What is FPS-R?

### Short Introduction 
[Introduction in Chinese, 中文简单介绍](README-CH.md)

**FPS-R** (`Frame-Persistent Stateless Randomization` or `静态律动算法` in Chinese), is a lightweight algorithmic pattern for simulating *persistent yet stateless randomness* across a continuous timeline — ideal for procedural motion, glitch aesthetics, and synthesised analog artifact emulation. Inspired by natural behaviors like eye saccades, animal foraging, human hesitation, predatory stalking, and structured noise, FPS-R generates unpredictability without relying on previous-frame memory.

It’s **repeatable**, **tunable**, and **frame-specific**, making it a versatile tool for time-based content creation platforms (Houdini, Maya, 3D Studio Max, Nuke, Adobe After Effects), GLSL, P5.js, game engines (Unity, Unreal) and embedded environments. Thanks to its simplicity and efficiency, FPS-R can also be implemented in any 3D platform that supports expressions or scripting—such as Maya, Nuke, or Adobe After Effects.

## 🎭 Motion Philosophy 
**FPS-R simulates the unpredictability of hesitation—yet within the same framework, it can just as easily express instinct.**  
When its temporal holds are short, it expresses *impulse*: quick changes that feel twitchy, clipped, or alert.  
When they lengthen, it evokes *hesitation*: thoughtful pauses, lingering states, the illusion of deliberation.  
It doesn’t switch modes—it sculpts time.  
The values may be random, but *when* they appear is guided by the frame count itself—producing a rhythm that feels intentional, even without memory.  
What emerges is not chaos, but *temporal behavior shaped by structure*.

> 🧠 Teaching Note: “No-sim is the best sim.”<br>
> In teaching Houdini—which is famed for simulation—I often remind students that building a procedural system without frame-to-frame dependencies is often superior to relying on complex simulations.<br> 
> The most elegant systems allow each frame to stand alone—yet somehow feel like they remember the past.

---
## ✨ Key Features

- Frame-anchored **repeatability** without storing state  
- Tunable **chaos profiles** with modular curve sculpting  
- Compatible with stateless shaders, simulation loops, and real-time systems  
- Ideal for generating behavior that feels **alive**, not algorithmic

---
## 🎞 Sample Uses in Animation

![Single Eyeball Look Straight Ahead with Saccades](resources/readme/images/h_fpsr_01_v002_02.gif)  
*Eye saccades or darting behavior in Houdini - Single Eyeball Look Straight Ahead with Saccades*

![Double Eyeballs Look at a Moving Box with Layered FPS-R Saccades](resources/readme/images/h_fpsr_01_v002.gif)  
*Saccades layered on top of object tracking in Houdini - Double Eyeballs Look at a Moving Box with Layered FPS-R Saccades*

---

## 💡 Why Do I Need Another Random Stream Generator?

### 🧬 The Nature of "Held" Randomness

In both natural and artificial systems, randomness is rarely pure noise—it often lingers, persists, or evolves in a way that feels intentional. Whether it’s the shimmer of dappled light, the jitter of analog machinery, or the twitchy searching of eye saccades, we encounter random behaviors that hold their shape or drift over time. Yet despite how ubiquitous this phenomenon is, most content creation tools are ill-equipped to simulate, emulate and represent it in an intuitively straightforward and lightweight manner. Replicating this structured unpredictability often requires laborious custom coding or non-intuitive workarounds.

### 🧱 Limitations of Conventional Techniques

The most common approaches tend to fall into two camps:

- **Worley-like noise functions**: These typically use seeded scatter points driven by hash tables, requiring lookups and multiple distance calculations per sample. While powerful, they require multiple steps and are thus more complex and computationally expensive — especially when layered across octaves—and difficult to visualize or control intuitively.

- **Previous-frame dependent logic**: This method can emulate “held” randomness by passing state from frame to frame. However, it introduces complexity and tightly couples your logic to platforms that support such state sharing (e.g., Houdini). Furthermore, it often cannot be implemented in lightweight contexts like expression fields or shader snippets, limiting its portability.

### ⚙️ Why FPS-R Is Different

FPS-R (Frame-Persistent Stateless Randomization) introduces a lightweight and highly portable solution to this long-standing problem. At its core is the **Stacked Modulo (SM)** method, which generates structured, frame-evolving randomness without relying on state or lookup tables.

Key advantages include:

- ✅ **Truly Stateless**: FPS-R does not rely on previous-frame data or persistent memory buffers.
- 🧠 **Compact & Readable**: The SM expression can be written in a single line using `$F` in Houdini expressions or `@Frame` in VEX.
- 🛠️ **Platform Agnostic**: Works seamlessly in any environment that exposes frame-based context, even those without advanced memory-sharing capabilities.
- ⚡ **Performance Friendly**: No costly distance functions, hash lookups, or scatter generation—making it ideal for stacking, layering, or real-time use.


---
## 🧬 Flavours of FPS-R

### 🌀 Stacked Modulo (SM) or (叠模机制 in Chinese)
The original FPS-R method. **Stacked Modulo** uses layered modulus operations and shifting offsets to produce coherent but unpredictable transitions in output values and frame-held durations. This approach simulates analog drift or organic irregularities with surprising expressiveness.
- Feels like *memory without memory*  
- Shaped by frequency, amplitude, and phase control  
- Lightweight and highly composable

**SM Features:**
- Adjustable upper bounds for how long values are held between regenerations  
- Uses native `rand()` or platform-specific random functions to preserve entropy
- A very compact 1-line version is able to achieve stateless random-hold as long as these features are available:
  - a random function (ie, `rand()`)
  - the modulo function (ie, `mod()` or `%`)
  - the current frame as an integer

---

### ✴ Quantised Switching (QS) or (量跃机制 in Chinese)
**Quantised Switching** selects discrete values from user-defined inputs using deterministic pseudo-random indexing. It’s excellent for triggering state changes, discrete flicker events, or cascading logic.

This method offers broad flexibility and configuration.
- Great for **glitch jumps**, **channel flickering**, or **logic switching**  
- Stateless but consistently frame-coherent  
- Supports frequency shaping and value distribution

**QS Features:**
- Accepts any input: procedural noise, parametric curves, or lookup splines  
- Optional quantization bypass for continuous, ordered output  
- Optional randomness bypass for fully structured transitions using raw inputs

Both methods are interoperable—layer them to blend abrupt shifts with smooth drift for richer behavior.

---

## 🧪 Use Cases

- **Procedural animation systems**: Generate structured motion that avoids mechanical repetition and supports layered rhythmic control  
- **Analog artifact emulation**: Recreate chroma drift, signal pop, scanline jitter, and other temporally responsive glitch aesthetics  
- **Embedded systems & microcontrollers**: Deliver time-varying behaviors in memory-constrained environments (e.g., ESP32 visualizations, LED pacing)  
- **Attention shift modeling**: Simulate gaze changes, hesitation, and organic redirection with psychological plausibility  
- **Crowd systems and agent states**: Drive behavioral state shifts across populations, enabling emergent motion patterns like scattering, regrouping, or swarming  
- **Games and XR environments**: Add non-repetitive, lifelike behaviors to characters, props, or environments without full simulation overhead  
- **Low-cost, no-simulation logic building**: Construct believable time-based transitions in platforms lacking memory-sharing or state-machine logic


---

## 🔩 How It Works

FPS-R leverages frame-indexed logic—often stacked modulos, shaped ramps, and deterministic randomness—to generate entropy without internal state. It outputs motion and behavior that evolves believably without memory.

> _Imagine your random number generator read its diary from yesterday, tore out the page, and wrote today’s in the same erratic tone._

---

## 🚧 Current Status

FPS-R is under active development. Planned updates include:
- Modular utility refactors  
- Ready-to-deploy Houdini and GLSL implementations  
- More creative samples and behaviors  
- Preset “chaos curves” and transition archetypes

This repository is **currently private** during code cleanup and finalization.

---

## 🤝 Contributions

Intrigued by deterministic chaos, analog glitch modeling, or entropy as narrative? Contributions, suggestions, and entropy philosophies are welcome once the repo returns to public life.
