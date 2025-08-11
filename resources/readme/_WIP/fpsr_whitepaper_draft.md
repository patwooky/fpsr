# FPS-R: A Stateless Deterministic Framework for Generating Unpredictable Move-and-Hold Phrasing Signals

> **Work in Progress**  
> This document is an active draft and may contain incomplete sections, placeholder data, and provisional terminology.  
> Feedback, suggestions, and contributions are welcome. The FPS-R codebase is open-sourced under the MIT licence.

**Author:** *Patrick Woo*  
**Affiliation:** *Independent Researcher / Creative Technologist / Technical Artist / VFX Specialist*  
**Contact:** *[Your email or website]*  
**Version:** Draft 0.1 — *[Date]*  
**Licence:** MIT (same as code repository)  

---

## Abstract
*Short, one-paragraph summary of my core contribution and why it matters.*  
Example:  
> In this paper I introduce FPS-R, a minimal and composable framework for generating deterministic, stateless, frame-persistent “move-and-hold” phrasing signals across time and space. FPS-R produces sequences that are repeatable and auditable whilst remaining unpredictable in timing, enabling new control paradigms for distributed systems, procedural generation, simulation, and creative applications. I describe its architecture, compare it to related algorithms such as Worley noise, Perlin noise, and EA Seed, and demonstrate its portability, reproducibility, and computational efficiency.

---

## 1. Introduction
- **The problem**: Existing randomisation or procedural noise methods produce either fully continuous variation (Perlin, simplex) or predictable discrete changes (modulo-based, cell noise). Many systems require *unpredictable but reproducible* changes that “hold” for irregular intervals.  
- **The opportunity**: In distributed, auditable, or cross-platform contexts, *stateless determinism* provides synchronisation without network state and enables verifiable playback (“capsules”).  
- **My contribution**: FPS-R is a *grammar* of three simple operators (SM, TM, QS) that can be layered to produce complex hold–jump phrasing patterns, spatial or temporal, whilst remaining deterministic and stateless.

---

## 2. Background and Related Work
### 2.1 Procedural Noise
Briefly describe Perlin noise and Worley noise, what they are suited for, and their limitations for hold/jump phrasing.

### 2.2 Stateless Random Allocation
Summarise EA Seed-like algorithms — focus on their goal (non-colliding index allocation) and why FPS-R solves a different problem.

### 2.3 Temporal Modulation and Pattern Generators
Mention common methods (LFOs, mod+hash, pseudo-random time-stepping) and why they fall short for multi-domain phrasing needs.
  
*<Insert comparison table between Perlin, Worley, EA Seed, FPS-R, listing: primary domain (spatial/temporal), determinism, statelessness, predictability of hold duration, composability, computational cost>*

---

## 3. FPS-R Architecture
### 3.1 Core Properties
Stateless, deterministic, reproducible across platforms; frame/time as the only incrementing input; composable; portable across languages and runtime environments.

### 3.2 The Three Operators
- **SM (Step Modulator)**: controls discrete step changes at irregular intervals.  
- **TM (Time Modulator)**: scales and offsets input frame/time to vary pacing.  
- **QS (Quantised Sine)**: generates periodic patterns with quantised holds.  
*(Add diagrams or mini-waveforms here)*

### 3.3 Capsules
A capsule is a parameter set that defines a reproducible signal segment, allowing exact replay across devices and platforms.  
While I have not yet implemented a working capsule framework, my intention is to develop a minimal reference implementation in future work to validate portability.  
*<Insert schematic showing capture/replay flow here>*

### 3.4 Composability and Nesting
Describe how SM/TM/QS outputs can feed each other or modulate other systems.

---

## 4. Implementation
### 4.1 Pseudocode Overview
*<Insert short pseudocode for SM, TM, QS>*

### 4.2 Language Ports
Mention existing ports (Python, GLSL, VEX, C).  
*<Insert code snippet examples in 2–3 languages>*

### 4.3 Complexity and Performance
Big-O analysis and rough CPU timings.  
*<Insert profiling table comparing FPS-R to Perlin/Worley for equivalent resolution>*

---

## 5. Experimental Results
### 5.1 Reproducibility Tests
Run identical parameter sets on multiple platforms (Google Colab Python, Replit JS, local C++ build, GLSL shader) and verify identical output sequences.  
*<Insert table of checksums / hash matches>*

### 5.2 Hold Duration Distribution
Measure and plot the statistical distribution of hold lengths for various parameter settings.  
*<Insert histogram plots here>*

### 5.3 Comparative Behaviour
Show side-by-side timelines for:  
- Modulo random selection (predictable beat)  
- Perlin quantised to bands  
- Worley-based holds  
- FPS-R SM/TM/QS  
*<Insert comparison graphs here>*

### 5.4 Domain-Specific Demonstrations
1. Cybersecurity: unpredictable defender attention switching.  
2. Distributed systems: reproducible sequence across nodes.  
3. Creative/VFX: particle or camera motion phrased with FPS-R.

---

## 6. Discussion
- **Advantages**: portable, low-cost, composable, reproducible, deterministic, stateless.  
- **Limitations**: underlying randomness is still pseudo-random; security applications require proper integration with cryptographic systems; no built-in semantic meaning (needs higher-level system context).  
- **Opportunities for Extension**: multi-dimensional capsules, adaptive parameter mutation, integration with AI agents for behavioural modulation.

---

## 7. Conclusion
Summarise:  
> FPS-R offers a minimal yet expressive language for deterministic, stateless phrasing, enabling reproducible unpredictability across domains. Its three-operator design and composability allow for creative and technical control beyond what existing procedural noise or allocation methods provide.

---

## Acknowledgements
*Optional — name collaborators, tools, inspirations.*

---

## References
*(Add references for Perlin noise papers, Worley, EA Seed docs, any relevant procedural/randomness literature)*

---

## Appendix A: Additional Examples
- Code snippets, more graphs, capsule library examples.
