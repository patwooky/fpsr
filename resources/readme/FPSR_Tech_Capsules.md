# Capsules Documentation
##### This documentation is still in development. While every update strives to be accurate, there will be parts that are incomplete or inaccurate.



## Table of Contents
- [✍️ FPS-R Capsule Contribution Guide](#-fps-r-capsule-contribution-guide)
- [🎯 What Is a FPS-R Capsule?](#-what-is-a-fps-r-capsule)
- [A Typical 📦 `.cap.json` Format](#a-typical--capjson-format)
  - [FPS-R: Stacked Modulo Capsule Format](#fps-r-stacked-modulo-capsule-format)
    - [🛠 Minimal Fields](#-minimal-fields)
  - [FPS-R: Quantised Switching Capsule Format](#fps-r-quantised-switching-capsule-format)
- [🌱 Naming Guidelines (Optional, Suggested)](#-naming-guidelines-optional-suggested)
- [🧪 How to Observe & Curate a Capsule](#-how-to-observe--curate-a-capsule)
- [🌱 The Spirit of Sharing](#-the-spirit-of-sharing)
- [🌱 The Current State of the Capsules Technology](#-the-current-state-of-the-capsules-technology)
- [🤝 Attribution & Community](#-attribution--community)

---

## ✍️ FPS-R Capsule Contribution Guide
Creating and Sharing Behavioral Phrases in FPS-R

## 🎯 What Is a FPS-R Capsule?
A **FPS-R capsule** is a mechanism that **stores** and **captures** FPS-R's performances of structured and deterministic unpredictability. 

A FPS-R capsule's settings is **reusable**, and a capsule's performance is **repeatable**. These settings and performances shows how FPS-R's behavior unfolds over time using only a seed, input signal, and configuration—without memory. Capsules can either be:

### Storage
**Reusable parameter settings:** to replicate the state of FPS-R at the point of capture.

### Capture
**Captured Performances:** curated windows of time that showcase distinctive phrasing


---
Here is the format of a capsule file.

### A Typical 📦 `.cap.json` Format
#### FPS-R: Stacked Modulo Capsule Format
```json
{
    "metadata": {
        "name": "glitch-drift",
        "author": "patrick",
        "URL": "author's website",
        "created": "2025-07-03",
        "description": "A rhythmic modulation that begins with erratic jumps and softens into a calm decay.",
        "tags": ["SM", "behavioral", "modulation", "glitch", "drift"],
        "platforms": ["py3.10-ARMv8"],
    },
    "performance_clip": {
        "preview_trace": [0.02, 0.31, 0.78, 0.55, 0.11, 0.07],  
        "clip_time": [start_frame, end_frame] // [] if no capture window
    }, 
    "engine": "sm", // "sm" stacked modulo or "qs" for quantised_switching
    "config": {
        "minHoldFrames": 16,
        "maxHoldFrames": 24,
        "reseedFrames": 9,
        "offsetOuter": 23,
        "offsetInner": -41
    }
}
```

##### 🛠 Minimal Fields


| **Field**        | **Required?** | **Example**                        | **Notes**                                                |
|------------------|---------------|-------------------------------------|----------------------------------------------------------|
| `name`           | ✅            | `"pulse–vanish"`                   | Descriptive phrase capturing behavioral modulation       |
| `author`         | ✅            | `"patrick"`                        | Creator attribution                                      |
| `URL`            | ✅            | `"2025-07-04"`                     | Creator's Website                                        |
| `created`        | ✅            | `"2025-07-04"`                     | ISO date format                                          |
| `description`    | ⬜            | `"Soft flicker that fades out"`    | Human-readable overview of phrasing                      |
| `tags`           | ⬜            | `["glitch", "SM", "decay"]`        | Used for filtering, grouping, browsing                   |
| `platforms`      | ⬜            | `["py3.10-ARMv8"]`                 | Platforms where the phrasing has been visually confirmed |
| `preview_trace`  | ⬜            | `[0.12, 0.45, ...]`                | Output values over time (1:1 mapping to `clip_time`)     |
| `preview_path`   | ⬜ (Legacy)   | `"previews/xyz.gif"`               | Optional if `preview_trace` is present                   |
| `settings.type`  | ✅            | `0`                                | `0 = SM`, `1 = QS`                                       |
| `settings.seed`  | ✅            | `12345`                            | Controls modulation sequence                             |
| `settings.inner_mod_dur` | ✅    | `40`                               | For SM: sub-phrase length                                |
| `settings.outer_mod_dur` | ✅    | `50`                               | For SM: phrase cycle length                              |
| `settings.clip_time` | ⬜       | `[120, 125]`                        | The start and end frames of the observed performance. Omit or empty array `[ ]` indicates no performance, capsule becomes just a setting           |

---
#### FPS-R: Quantised Switching Capsule Format
```json
{
    "metadata": {
        "name": "glitch-drift",
        "author": "patrick",
        "URL": "author's website",
        "created": "2025-07-03",
        "description": "A rhythmic modulation that begins with erratic jumps and softens into a calm decay.",
        "tags": ["SM", "behavioral", "modulation", "glitch", "drift"],
        "platforms": ["py3.10-ARMv8"],
    },
    "performance_clip": {
        "preview_trace": [0.02, 0.31, 0.78, 0.55, 0.11, 0.07],  
        "clip_time": [start_frame, end_frame] // [] if no capture window
    }, 
    "engine": "qs", // "sm" stacked modulo or "qs" for quantised_switching
    "config": {
        "baseHoldFreq": 0.035,
        "quantLevelsMinMax": [4, 10],
        },
        "timing": {
            "switchFactor": 0.4,
            "stream1QuantFactor": 0.9,
            "stream2QuantFactor": 0.65
            "streamSwitchDur": 24 // Duration for switching streams in frames
        },
        "stream1": {
            "offset": 0,
            "generator": "sine",
            "params": {
                "freqFactor": 1.0,
                "phaseOffset": 0.0
            }
        },
        "stream2": {
            "offset": 5,
            "generator": "sine",
            "params": {
                "freqFactor": 5.2,
                "phaseOffset": 90.0
            }
        }
    }
}
```


---
## 🌱 Naming Guidelines (Optional, Suggested)
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
## 🧪 How to Observe & Curate a Capsule
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
## 🌱 The Spirit of Sharing
FPS-R capsules are meant to be shared—not just as assets, but as expressive fragments of understanding. Each one captures a phrasing, a modulation, a moment that felt intentional. Sharing them openly means building a collective language of behavior, where anyone can contribute, remix, or reuse phrasing as a creative gesture.

This isn’t just data. It’s a living vocabulary. With it we can form complete sentences, and passages with these performance "clips".

The goal isn’t control—it’s resonance. As the library grows, so does our collective ability to describe movement, signal intent, and craft synthetic motion with clarity and emotion. Capsules are our shared phrases. The more we share, the more fluent we become.

---
### 🌱 The Current State of the Capsules Technology
The implementation of FPS-R Capsules is **still in the conception stage**. The vision is clear and the use-cases for FPS-R Capsules are vast.

As the technology matures and its use becomes wide-spread, the need of capturing performances and sharing them will become increasingly helpful and important. 

As the framework takes off, more work will be put into refining and implementing the capsule technology.

---
## 🤝 Attribution & Community
FPS-R is an open framework for expressive modulation. If you’ve used its capsule data or principles in your own creative work, a small attribution—linking back to this repository—is deeply appreciated.

More importantly: if you’ve created your own phrasing, named a new behaviors, or refined a modulation idea—we invite you to contribute it back to the growing capsule library.

This project thrives not just on usage, but on conversation. Let’s teach the world to speak in phrasing.