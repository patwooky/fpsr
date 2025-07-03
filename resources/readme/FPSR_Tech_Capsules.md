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
  "author": "your_name",
  "created": "2025-07-04",
  "description": "A rhythmic modulation that begins with erratic jumps and softens into a calm decay.",
  "tags": ["SM", "behavioral", "modulation", "glitch", "drift"],
  "preview_path": "previews/glitch-drift_120-183.gif",
  "settings": {
    "type": 0,                    // 0 = SM, 1 = QS
    "seed": 12345,
    "inner_mod_dur": 40,
    "outer_mod_dur": 50,
    "clip_time": [120, 183]       // Optional: omit or [] if no capture window
  }
}

```
### 🛠 Minimal Fields


| **Field**       | **Required?** | **Example**                        | **Notes**                                      |
|-----------------|---------------|-------------------------------------|------------------------------------------------|
| `name`          | ✅            | `"pulse-flicker"`                  | Use behavioral naming, like gestures           |
| `type`          | ✅            | `0` (SM) or `1` (QS)               | Set in `settings.type`                         |
| `seed`          | ✅            | `12345`                            | Determines deterministic phrasing              |
| `mod_*`         | ✅            | `inner_mod_dur`, `outer_mod_dur`  | Or QS equivalents                              |
| `clip_time`     | ⬜            | `[120, 183]`                       | Empty or omitted = preset only                 |
| `description`   | ⬜            | `"Begins glitchy, resolves slow"` | For human readability                          |
| `tags`          | ⬜            | `["drift", "SM", "rhythmic"]`     | Search, filters, category grouping             |
| `preview_path`  | ⬜            | `"previews/name_120-183.gif"`     | Thumbnail, canvas export, or link              |


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