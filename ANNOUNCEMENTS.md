# Announcements

This file contains major announcements and important updates about the project.


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