# Capsules Documentation

## Table of Contents



---

## ✍️ Capsule Contribution Guide
Creating and Sharing Behavioral Phrases in FPS-R

## 🎯 What Is a Capsule?
A **capsule** is a deterministic modulation unit built on FPS-R. It defines how a behavior unfolds over time using only a seed, input signal, and configuration—without memory. Capsules can either be:

- **Presets:** reusable parameter sets with no defined clip time
- **Captured Performances:** curated modulation windows that showcase distinctive phrasing

### 📦 `.capsule.json` Format
```json
{
  "name": "glitch-drift",
  "author": "patrick",
  "created": "2025-07-03",
  "description": "A rhythmic modulation that begins with erratic jumps and softens into a calm decay.",
  "tags": ["SM", "behavioral", "modulation", "glitch", "drift"],
  "platforms": ["py3.10-ARMv8"],
  "preview_trace": [0.02, 0.31, 0.78, 0.55, 0.11, 0.07],  
  "settings": {
    "type": 0,              // 0 = SM, 1 = QS                  
    "seed": 12345,
    "inner_mod_dur": 40,
    "outer_mod_dur": 50,
    "clip_time": [120, 125] // Optional: omit or [] if no capture window
  }
}




```
### 🛠 Minimal Fields


| **Field**        | **Required?** | **Example**                        | **Notes**                                                |
|------------------|---------------|-------------------------------------|----------------------------------------------------------|
| `name`           | ✅            | `"pulse–vanish"`                   | Descriptive phrase capturing behavioral modulation       |
| `author`         | ✅            | `"patrick"`                        | Creator attribution                                      |
| `created`        | ✅            | `"2025-07-04"`                     | ISO date format                                          |
| `description`    | ⬜            | `"Soft flicker that fades out"`    | Human-readable overview of phrasing                     |
| `tags`           | ⬜            | `["glitch", "SM", "decay"]`        | Used for filtering, grouping, browsing                   |
| `platforms`      | ⬜            | `["py3.10-ARMv8"]`                 | Platforms where the phrasing has been visually confirmed |
| `preview_trace`  | ⬜            | `[0.12, 0.45, ...]`                | Output values over time (1:1 mapping to `clip_time`)     |
| `preview_path`   | ⬜ (Legacy)   | `"previews/xyz.gif"`              | Optional if `preview_trace` is present                   |
| `settings.type`  | ✅            | `0`                                | `0 = SM`, `1 = QS`                                       |
| `settings.seed`  | ✅            | `12345`                            | Controls modulation sequence                             |
| `settings.inner_mod_dur` | ✅    | `40`                               | For SM: sub-phrase length                                |
| `settings.outer_mod_dur` | ✅    | `50`                               | For SM: phrase cycle length                              |
| `settings.clip_time` | ⬜       | `[120, 125]`                       | Omit or empty array to indicate a preset (no phrasing)   |


---
### 🧪 How to Observe & Curate a Capsule
1. **Start from a preset –** define core parameters, mod durations, and seed
2. **Run it through your FPS-R system –** play over timeline or field
3. **Watch for phrasing –** does a recognizable behavior emerge over N frames?
4. **If so, record:**
- Start frame = beginning of phrased gesture
- End frame = last frame where phrasing holds
- Optional: annotate or preview export
5. **Name it** based on its contour (e.g., linger-jump, wobble-fade)
6. **Wrap into** `.capsule.json` as a documented behavior
---
### 🌱 Naming Guidelines (Optional, Suggested)
Capsules represent captured performances—they’re behavioral phrases, not just functions. Name them like gestures, not parameters.
- Use flowing phrase structures, like [adjective] verb – [adjective] verb – [optional more phrases]  
Examples:
    - calm hold – huge jump
    - glitch – stutter3x – hold – final calm
    - tight pulse – slip – vanish
- Speak to the **feel**, not the implementation. Avoid overly technical terms—favour motion, energy, rhythm.
- Compound phrases are welcome. Don’t limit length unless for UI constraints. Let clarity and cadence guide you.
- Optional: use timing hints like `stutter3x`, `long hold`, or mood tags like `soft`, `erratic`, `final`.
- Group by behavioral tags: "`glitch`", "`sustain`", "`hesitation`", "`burst`", "`spiral`"—not tech specs like `mod_inner_dur`.
- Think like a movement director, not a math parser. What would you call this if it were a dance or a gesture in dialogue?

---

## 🌱 The Spirit of Sharing
FPS-R capsules are meant to be shared—not just as assets, but as expressive fragments of understanding. Each one captures a phrasing, a modulation, a moment that felt intentional. Sharing them openly means building a collective language of behavior, where anyone can contribute, remix, or reuse phrasing as a creative gesture.

This isn’t just data. It’s a living vocabulary.

The goal isn’t control—it’s resonance. As the library grows, so does our collective ability to describe movement, signal intent, and craft synthetic motion with clarity and emotion. Capsules are our shared phrases. The more we share, the more fluent we become.

---
## 🤝 Attribution & Community
FPS-R is an open framework for expressive modulation. If you’ve used its capsule data or principles in your own creative work, a small attribution—linking back to this repository—is deeply appreciated.

More importantly: if you’ve created your own phrasing, named a new behavior, or refined a modulation idea—we invite you to contribute it back to the growing capsule library.

This project thrives not just on usage, but on conversation. Let’s teach the world to speak in phrasing.