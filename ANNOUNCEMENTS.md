# FPS-R Announcements

This file contains major announcements and important updates about the project.



---
### 11 November 2025
[v 3.0.0]

#### 🎉 Major Release v3.0.0 on GitHub

We are thrilled to announce the release of **FPS-R v3.0.0** - a landmark update that represents the full maturation of the Frame-Persistent Stateless Randomisation framework from its initial v1.0.0 release.

**Release Link**: [Release v3.0.0 on GitHub](https://github.com/patwooky/fpsr/releases/tag/v3.0.0)  
**Commit hash**: [`64b6fa1`](https://github.com/patwooky/fpsr/commit/64b6fa11286a130ee15fa9b2f302e01b0f2df729)  
**License**: Apache 2.0 License

#### ✨ What's New in v3.0.0

**1. The Bitwise Decode (BD) Algorithm - A Fourth Member of the FPS-R Family**  
The most expressive FPS-R algorithm yet! BD uses bit-flipping mechanics to generate complex, non-repeating patterns with exceptional combinatory possibilities. Perfect for applications requiring highly dynamic and unpredictable procedural generation.

**2. Wrapper Versions with Rich Analytical Capabilities**  
Both C and Python now include comprehensive wrapper functions that provide deep insights into FPS-R output:
- Track value changes with `has_changed`
- Monitor hold progression with `hold_progress`
- Access temporal anchors with `last_changed_frame` and `next_changed_frame`
- Hierarchical Phrased Quantisation (HPQ) for sophisticated time-scaling

**3. Interactive HTML/JavaScript Visualizer**  
Experience all four FPS-R algorithms in action through our new online visualizer! Play with parameters in real-time and see how each algorithm behaves.  
**[Try it now on GitHub Pages](https://patwooky.github.io/fpsr/resources/code/html_javascript/fpsr_demo.html)**

**4. Comprehensive Profile Output Analysis**  
New data analysis tools reveal the unique "fingerprint" of each FPS-R algorithm:
- Interactive Jupyter notebook for hands-on exploration
- Visual graphs showing output patterns, hold durations, and distributions
- Performance benchmarks and timing comparisons
- All integrated into the main README for easy reference

#### 🔧 Key Technical Improvements
- **Cross-Platform Determinism**: Thread-safe sine lookup tables ensure bit-for-bit identical outputs between C and Python
- **Performance Optimizations**: Flexible LOD system and baked sine curves for efficient computation
- **Enhanced Robustness**: Comprehensive overflow checks and deterministic initialization

#### 📖 Documentation
This release includes extensive documentation updates with visual examples, performance metrics, and detailed explanations of all new features. The README now features embedded graphs showing the output characteristics of each algorithm.

[Read the full Changelog](./CHANGELOG.md#300---2025-11-11) for complete details.

---
### 20 Oct 2025
[v 2.0.4]
**Breaking Change**
Changed the SM and TM output. In previous versions, the final stage where the seed was multiplied by a very large number (`100000ULL`) before being passed to `portable_rand_u64()`. This number is now removed, and this final number would be directly passed to `portable_rand_u64()`. This improves the theoretical purity of the hashing step.

---
### 19 Oct 2025
[v 2.0.3]
.An exciting addition of a **_fourth_** algorithm to the FPS-R framework, the **FPS-R: Bitwise Decode**.
Updated Development Journal on the origin. [Read Here](./resources/readme/FPSR_Origins_Journal_Reflections.md#a-fourth-algorithm---deeper-into-the-bits-bitwise-decode)

---
### 22 Sep 2025
[v 2.0.3]
C and Python implementations are now deterministically bit-for-bit, producing exactly the same output as each other. **This change breaks output consistency with previous versions.** 
[Read the the Changelog](./CHANGELOG.md#203---2025-09-22) for more detail

---

### 4 Sep 2025
[v 1.0.1]
Made some changes to FPS-R QS stream outputs. This will affect QS outputs and break output consistency with earlier versions. Pervious versions send the output from both sine wave streams (-1 to 1) to `portable_rand()`. I have normalised thsee to (0 to 1).
[Read the the Changelog](./CHANGELOG.md#101---2025-09-04) for more detail

---
### 14 August 2025

#### > New Release on GitHub
Released [v1.0.0] on GitHub. 
Link: [Release v1.0.0 on Github](https://github.com/patwooky/FPSR_Algorithm/releases/tag/v1.0.0)
Commit hash: [`d512644`](https://github.com/patwooky/FPSR_Algorithm/commit/d512644e19c3c8f8ad5600f5294ef38cd10417c0)
Released under the MIT License.

[Read the the Changelog](./CHANGELOG.md#100---2025-08-14) for more detail

#### > New `ANNOUNCEMENTS` Document
I added the this announcements document in anticipation of any major changes in the codes, repos and documents.

#### > Consolidated Documents: `Journal`, `Thoughts` and `Origins`
I have consolidated the following documents into a new one.

Removed: 
`FPSR_Thoughts.md`, `FPSR_Journal.md`, `FPSR_Origins.md`

New document: 
`FPSR_Origins_Journal_Reflections.md`

This change will be reflected in `README.md` as well.

---