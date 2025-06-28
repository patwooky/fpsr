# 🎲 FPS-R Algorithm: Frame-Persistent Stateless Randomization

_Stateless unpredictability with a structured soul._

## What is FPS-R?

**FPS-R** (`Frame-Persistent Stateless Randomization`) is a lightweight algorithmic pattern for simulating *persistent yet stateless randomness* across animation frames—ideal for procedural motion, glitch aesthetics, and analog artifact emulation. Inspired by natural behaviors like eye saccades, VHS drift, and structured noise, FPS-R generates unpredictability without relying on previous-frame memory.

It’s **repeatable**, **tunable**, and **frame-specific**, making it a versatile tool for Houdini, GLSL, P5.js, and embedded environments. Thanks to its simplicity and efficiency, FPS-R can also be implemented in any 3D platform that supports expressions or scripting—such as Maya, Nuke, or Adobe After Effects.

---

## 🎞 Sample Uses in Animation

![FPS-R Algorithm Diagram](resources/readme/images/h_fpsr_01_v002_02.gif)  
*Eye saccades or darting behavior in Houdini*

![FPS-R Algorithm Diagram](resources/readme/images/h_fpsr_01_v002.gif)  
*Saccades layered on top of object tracking in Houdini*

---

## ✨ Key Features

- Frame-anchored **repeatability** without storing state  
- Tunable **chaos profiles** with modular curve sculpting  
- Compatible with stateless shaders, simulation loops, and real-time systems  
- Ideal for generating behavior that feels **alive**, not algorithmic

---

## 🧬 Flavours of FPS-R

### 🌀 Stacked Modulo (SM)

The original FPS-R method. **Stacked Modulo** uses layered modulus operations and shifting offsets to produce coherent but unpredictable transitions in output values and frame-held durations. This approach simulates analog drift or organic irregularities with surprising expressiveness.

- Feels like *memory without memory*  
- Shaped by frequency, amplitude, and phase control  
- Lightweight and highly composable

**SM Features:**
- Adjustable upper bounds for how long values are held between regenerations  
- Uses native `rand()` or platform-specific random functions to preserve entropy

---

### ✴ Quantised Switching (QS)

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

- **Procedural animation systems**: structured motion that resists mechanical repetition  
- **Analog artifact emulation**: including chroma roll, signal pop, and drift  
- **Embedded systems**: when temporal variation is needed but memory is limited

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
