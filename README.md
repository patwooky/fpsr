# 🎲 FPS-R Algorithm: Frame-Persistent Stateless Randomisation  
### _Stateless unpredictability with a structured soul._

## What is FPS-R?

### Introduction  
[README in Chinese - 中文简体](README-CH.md)

**FPS-R** stands for `Frame-Persistent Stateless Randomisation` (or `静态律动算法` in Chinese). It is a lightweight algorithmic pattern for simulating persistent yet stateless randomness across a continuous timeline—ideal for procedural motion, glitch aesthetics, synthesised organic behaviour and analogue artefact emulation. 

Inspired by natural behaviours such as eye saccades, animal foraging, human hesitation, predatory stalking, and structured noise, FPS-R produces unpredictability without relying on previous-frame memory. It mirrors the rhythm of an explorer's torch as he explores a darkened cave—pausing, twitching, drifting with intent—or the room-clearing manoeuvre of a soldier’s rifle: moving and holding from one strategic point to another, every new holding position disjointed from what came before.

It is **repeatable**, **tunable**, and **frame-specific**, making it a versatile tool for time-based content creation platforms (Houdini, Maya, 3D Studio Max, Nuke, Adobe After Effects), GLSL, P5.js, game engines (Unity, Unreal) and embedded environments. Thanks to its simplicity and efficiency, FPS-R can also be implemented in any 3D platform that supports expressions or scripting.

### 🎭 Motion Philosophy  
**FPS-R simulates the unpredictability of hesitation—yet within the same framework, it can just as easily express instinct.**  
When its temporal holds are short, it expresses *impulse*: quick changes that feel twitchy, clipped, or alert.  
When they lengthen, it evokes *hesitation*: thoughtful pauses, lingering states, the illusion of deliberation.  
It doesn’t switch modes—it sculpts time.  
The values may be random, but *when* they appear is guided by the frame count itself—producing a rhythm that feels intentional, even without memory.  
What emerges is not chaos, but *temporal behaviour shaped by structure*.

> 🧠 Teaching Note: “No-sim is the best sim.”  
> In teaching Houdini—which is famed for simulation—I often remind students that building a procedural system without frame-to-frame dependencies is often superior to relying on complex simulations.  
> The most elegant systems allow each frame to stand alone—yet somehow feel like they remember the past.

---

## ✨ Key Features

- Frame-anchored **repeatability** without storing state  
- Tunable **chaos profiles** with modular curve sculpting  
- Compatible with stateless shaders, simulation loops, and real-time systems  
- Ideal for generating behaviour that feels **alive**, not algorithmic

---

## 🎞 Sample Uses in Animation

![Single Eyeball Look Straight Ahead with Saccades](resources/readme/images/h_fpsr_01_v002_02.gif)  
*Eye saccades or darting behaviour in Houdini – Single Eyeball Look Straight Ahead with Saccades*

![Double Eyeballs Look at a Moving Box with Layered FPS-R Saccades](resources/readme/images/h_fpsr_01_v002.gif)  
*Saccades layered on top of object tracking in Houdini – Double Eyeballs Look at a Moving Box with Layered FPS-R Saccades*

---

## 💡 Why Do I Need Another Random Stream Generator?

### 🧬 The Nature of “Held” Randomness

In both natural and artificial systems, randomness is rarely pure noise—it often lingers, persists, or evolves in a way that feels intentional. Whether it’s the shimmer of dappled light, the jitter of analogue machinery, or the twitchy searching of eye saccades, we encounter random behaviours that hold their shape or drift over time. Yet despite how ubiquitous this phenomenon is, most content creation tools are ill-equipped to simulate, emulate and represent it in an intuitively straightforward and lightweight manner. Replicating this structured unpredictability often requires labour-intensive custom coding or non-intuitive workarounds.

> ✒️ Metaphor Note: *The FPS-R system is like a public pen at the post office.*  
> People come and go: one picks it up, another leaves it askew, sometimes no one touches it for hours.  
> Each interaction changes its orientation, position, or status—but never with memory.  
> Yet from a time-lapse sped-up video review, it appears to dance with intention.  

<br>
<p align="center">
  <img src="https://img.shields.io/badge/algorithmic-chaos%20with%20poise-blueviolet?style=flat-square" alt="Algorithmic Chaos with Poise">
  <img src="https://img.shields.io/badge/stateless-beauty-lightgrey?style=flat-square" alt="Stateless Beauty">
  <img src="https://img.shields.io/badge/memoryless-not%20mindless-9cf?style=flat-square" alt="Memoryless, Not Mindless">
</p>


### 🧱 Limitations of Conventional Techniques

The most common approaches tend to fall into two camps:

- **Worley-like noise functions**: These typically rely on spatially seeded feature points—often distributed via jittered grids or hash-based schemes—and compute distances between each sample and its surrounding points. While powerful for generating cellular textures, standard Worley noise tends to use evenly spaced seed distributions, leading to Voronoi-like cells with relatively uniform size and brightness. This results in predictable distance falloffs and similar displacement amplitudes across the field, which limits its expressiveness in simulating more erratic or organic behaviours. Additionally, it requires multiple computation steps—lookup tables, distance evaluations, sorting—which can be costly, especially when combining higher-order Fn layers, applying input warping, or layering across octaves. These techniques increase complexity and make Worley noise more difficult to visualise or intuitively control compared to simpler stateless methods. 

- **Previous-frame dependent logic**: This method can emulate “held” randomness by passing state from frame to frame. However, it introduces complexity and tightly couples your logic to platforms that support such state sharing (e.g., Houdini). Furthermore, it often cannot be implemented in lightweight contexts like expression fields or shader snippets, limiting its portability.

### ⚙️ Why FPS-R Is Different

FPS-R introduces a lightweight and highly portable solution to this long-standing problem. At its core is the **Stacked Modulo (SM)** method, which generates structured, frame-evolving randomness without relying on state or lookup tables.

Key advantages include:

- ✅ **Truly Stateless**: FPS-R does not rely on previous-frame data or persistent memory buffers  
- 🧠 **Compact & Readable**: The SM expression can be written in a single line using `$F` in Houdini expressions or `@Frame` in VEX  
- 🛠️ **Platform Agnostic**: Works seamlessly in any environment that exposes frame-based context  
- ⚡ **Performance Friendly**: No costly distance functions, hash lookups, or scatter generation

---

## 🧬 Flavours of FPS-R

### 🌀 Stacked Modulo (SM) or 叠模机制  
The original FPS-R method. Uses layered modulus operations and shifting offsets to produce coherent but unpredictable transitions.  
- Feels like *memory without memory*  
- Shaped by frequency, amplitude, and phase  
- Lightweight and highly composable

**SM Features:**

- Adjustable upper bounds for how long values are held  
- Uses `rand()` and `mod()` functions on current frame  
- Works in one-liner form in many toolkits

---

### ✴ Quantised Switching (QS) or 量跃机制  
A deterministic pseudo-random index selector for flickering, logic switching, and glitch-like transitions.

**QS Features:**

- Supports custom value banks  
- Can bypass randomness for structured switching  
- Quantisation optional for smooth interpolation

---

## 🧪 Use Cases

- Procedural animation and rigging  
- Analogue artefact and glitch emulation  
- Embedded systems and microcontrollers  
- Gaze simulation and behavioural psychology  
- Crowd logic and non-repeating state machines  
- Games and XR  
- Stateless runtime logic

---

## 🔩 How It Works

It’s all about frame-indexed logic: modulos, ramps, and deterministic `rand()` expressions give the illusion of continuity—with no memory attached.

---

## 🚧 Current Status

FPS-R is under active development and currently private during cleanup. Planned improvements:

- Modular utilities  
- Plug-and-play GLSL and Houdini expressions  
- Ready-made presets and chaos profiles

---

## 🤝 Contributions

If you're into procedural chaos, analogue aesthetics, or the poetry of entropy—your thoughts are welcome once it returns to public life.