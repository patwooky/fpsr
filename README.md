
# 🎲 FPS-R Algorithm: Frame-Persistent Stateless Randomization

_Stateless unpredictability with a structured soul._

## What is FPS-R?

**FPS-R** (Frame-Persistent Stateless Randomization) is a lightweight algorithmic pattern designed to simulate *persistent yet stateless randomness* across animation frames—ideal for procedural motion, glitch aesthetics, and analog artifact emulation. Inspired by natural behaviors like eye saccades, VHS drift, and procedural noise, FPS-R produces unpredictability without relying on previous-frame memory.

It’s **repeatable, tunable, and frame-specific**, making it a powerful tool for Houdini, GLSL, and embedded environments alike.

---

## ✨ Key Features

- Frame-anchored **repeatability** with no frame-to-frame state  
- Adjustable **“chaos profiles”** with modular curve shaping  
- Compatible with real-time systems, stateless shaders, and simulation loops  
- Ideal for creating **“alive” but non-random-looking behavior**

---

## 🧪 Use Cases

- **Procedural animation systems**: motion patterns that feel human or organic  
- **Analog artifact emulation**: including drifting VHS snow, chroma tear, and pop flicker  
- **Embedded systems**: when you want deterministic behavior without heavy buffers

---

## 🔩 How It Works

At its core, FPS-R uses stacked modulo logic and structured offsetting to yield consistent, frame-specific entropy. Think of it as noise that forgets the past—but never acts out of character.

> Imagine your random number generator read its own diary yesterday, then tore the page out and wrote today's entry in the same chaotic tone.

---

## 🚧 Current Status

FPS-R is currently under active development. Expect:
- Refactoring of utility functions  
- GLSL and Houdini-friendly versions  
- More sample use cases  
- Parameterized presets for common “chaos curves”

This repo is **temporarily private** while core logic is stabilized, cleaned up, and packaged for creative deployment.

---

## 🤝 Contributions

Got thoughts on structured unpredictability, glitch theory, or embedded-friendly chaos? Contributions, critiques, and collaborative ideas are welcome once this project returns to public.
