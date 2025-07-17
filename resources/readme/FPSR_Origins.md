# FPS-R - Origins

# Table of Contents


## Foreword

This document recounts and records the origins of FPS-R, how the idea took shape over decades, and how each algorithm in the framework as born, refined, and took shape.

It serves as a memoire and a record of that journey.

-- Patrick Woo

---
## Origin Story
by Patrick Woo
### How it Began - The problem and the Struggle
I had a problem creating an elegant move and hold in my visual effects work (which reflects the real world phenomena of course). 

### The Solution: A Compromise
To achieve these kinds of motion, I tried the following.
- layering noise of different frequencies and offsets
- stateful random reseeds at random frames

These worked, in a limited way. I wasn't particularly dissatisfied with these methods I found back then. I was thinking, that was just the way it is, the tools we were given, just like how we were given tools developed by software developers and we learn them, work with them. If there was something that could not be done, we work around the limitations. I wasn't trying to push the envelope with "a grand vision" in mind. I did not dream and could not imagine that one day I would contribute to a solution to these problems.

### The Beginnings of FPS-R: Stacked Modulo
Earlier on (close to 18 years ago)I learnt about the modulo operator, knew about how it behaves. Over the years, an expression evolved organically in my workflow - `frame - (frame % mod_period)` (apparently this was a common operation to hold a frame for a particular period of time, but I only found out later). The expression that emerged did not come from Google searches, but based on an understanding that grew naturally from the nature of modulo operations. It also emerged from asking myself "What if I could stall time? Since modulo keeps giving me ever increasing numbers up to the `divisor - 1`, if I subtracted the ever-increasing numbers I can get the value to maintain at the last multiple of the divisor". 

Moving further I realised that the stalled periods of stable numbers (frames numbers that were multiples of the hold_duration divisor) could feed into `random()` functions as seeds and that the resulting random values would stay constant for the period of the modulo cycle. This development and realisation took 2 to 3 years. The updated technique enabled me to constantly generate random numbers that hold in a stateless manner. **This became the foundation to the FPS-R Stacked Modulo**. 

This was good progress towards generating unpredictability, but it was still had a very static cycle of generation pattern. The values were random, but the hold times had regularity like clockwork.

### The Final Piece of Stacked Modulo and the Beginning of Quantised Switching
The most significant and final breakthrough for FPS-R was in the last months around March 2025 (this project was started in June). I was trying to create a VHS tape glitch effect where bands of horizontal areas in the frame would drift vertically around in the same region in the frame for a while, then jump to another vertical region in the frame then slide around that area for a while in very inconsistent speeds and periodicity.

Up that point, what I would do to achieve an organic "random move-and-hold" signal was to create 2 streams of modulo `frameA - (frameA % periodA)` and `frameB - (frameB % periodB)` to where `frameB` was the running frame with an offset from `frameA`, and `periodB` is a _slightly longer or shorter period_ than `periodA`, usually _not multiples of each other_. I would then switch between these with a switching mechanism (in Houdini that would be the `switch SOP`). In this switcher node, I would do a 3rd modulo expression with yet another time offset and with a different periodic duration from the modulo of both incoming streams. Each of the streams and the switch is running a modulo operation with its own periodicity, running in its own timeline. This set-up broke up the perceived rhythms and pacing of the resulting pattern, leveraging on the out-of-phase and out-of-sync offsets patterns between the 2 streams that interfere with each other, and they switch with a different switching periodicity. **This timed switching formed the foundation for FPS-R Quantised Switching**.

At that time I did not realise it, but looking back now, I can summarise this pattern to a Stacked Modulo variation:  
`(frameA - (frameA % (frameB - ((frameB % periodSwitch > (periodSwitch * 0.5)) ? periodA : periodB))))`

> Please let me digress for a moment. Writing out the above line and expressing this pre-Stacked Modulo workflow into an expression, it actually just **_gave birth to a third FPS-R algorithm!_**. I will mention this later in this documentation.

**This technique that I was of working with later became the FPS-R: Toggled Modulo**.  

Let us continue. To recap, I was already using some variant of the FPS-R before even realising it. In the pursuit of being perceived as "truly unpredictable", I felt this was inferior to the final form of Stacked Modulo that I would very quickly flesh out.

At that time I began to think about how frustrating and fixed `periodA` and `periodB` were in my modulo expressions. I wondered if I could randomise it within a fixed range of values using the same modulo techniques.  

At that time, in a sudden stroke of inspiration I arrived at a nested solution of nested modulo operations. What if a `rand(frame % duration)` exists inside the outer modulo to periodically randomise different hold periods? At that time it felt like I was going into some kind of infinite recursion. I was thinking about it and it gave me a headache. The feeling was "yes this is what I want to do, but surely this can't be that easy if I were to literally just put a rand(frame-(frame%innerDuration)) inside my outer modulo nest. It'll either go into non-deterministic tail spin into uncontrollably huge values that wouldn't make sense!" 

Eventually, implementing this proved to be successful and did exactly what I planned for it to do! That worked out petty well! **Stacked Modulo was born**. The date was sometime in the beginning of June 2025.

### The Sudden Appearance of Quantised Switching
After testing my ideas with Microsoft Copilot I was convinced I should start a repository (I marked it private back then because it was in a very nascent, vulnerable and fragile stage). 

I was very cautious of labelling this as a novel algorithm, because I did not know enough about what is out there. I searched around and did not find much content and material on the topic of move and hold (I haven't coined it yet), sudden bursts of speed and pause, etc.

I had many rounds of doubt and self debate as well as debates with MS Copilot. I was solely relying on Copilot. Google Gemini felt that the Worley noise was commonly used to create stateless holding temporal patterns with jumps in-between from time to time. I mention this further down in [How Did FPR-S Come Into Existence?](#-how-did-fpr-s-come-into-existence). 

I was very cautious with the replies that Copilot generated, since I was largely using one "sparring partner". I wanted it to play the devil's advocate to tell me the reasons certain part was not really new. I also bring out arguments against my own SM algorithm. questions like "How can it be new when all the components in the expression are simple, and the phrases in the components are commonly seen in well used expressions for other things?" To its credit, MS Copilot assured me, and insisted that this way of doing things and generating signals is truly novel.

So in this state of mind I wrote the code for QS on the spot (perhaps within 3 minutes), quantising 2 streams and put a switching mechanism between them with a modulo duration, I proposed it to Copilot "There. FPS-R:SM is such a cheat, I'm sure this simple 2-stream switching code can easily be used to achieve the same behaviour as FPS-R:SM, and QS is much simpler in logic and concept than Stacked Modulo, I'm sure somebody would have thought about this, surely the SM isn't so novel after all?" Here is my original message:

_I'm still thinking of how to "overthrow", "defeat" or "circumvent" my algorithm. imagine 2 streams of values.
`float stream1 = floor((sin(@Frame*0.3)*100)/100; `
`float stream2 = floor(sin((32+@Frame)*0.5)*155)/155;`
`f@outVal = (@Frame%20<10) ? stream1 : stream2;`
can you see what I'm trying to do with this? i prepare 2 sets of posterised values out of phase, each would have their own "hold" values, that tries to emulate the first (modulo) in my FPS-R, then the other layer of modulo comes with the final @outVal that switches between these, which would then "confuse" the user to think it is really random. ;)_

To my surprise Copilot took the code and replied with:

_"I see exactly what you are doing... You’re engineering a **structured deception**—a system that mimics random persistence by cleverly alternating between two distinct posterised noise streams. It is different from FPS-R (SM). FPS-R maintains true statelessness and deterministic hold, while this 2 posterised stream method depends on underlying sine waves and are cyclic in nature, they create the illusion of persistence through layered phasing and selective switching. This method (FPS-R: QS) excels at masked predictability, efficient and cheap computation from `floor()` to `sin()` calls. it has potential to pair with FPS-R: SM for even richer noise.
This experiment shows how **posterised multi-stream noise** can approximate FPS-R's persistence—without directly replacing it. It’s exciting because it brings another tool into your procedural randomness arsenal. Have you tested this visually yet? The results could be fascinating!"_

### Conclusion of Origin Story

I did not intend for it to be stateless and deterministic, but these properties surfaced from the way I shaped it. I think the biggest contributor of its statelessness was the fact that I kept refining and thinking about it in the context of the code being used in an expression that can be evaluated in an "one-line" expression field.

I wasn't a fancy math genius, I was simply limited to the tools I know and have come to understand well. using the well known techniques of visual effects creating complexity and detail through layering of noise, adding dissonance and breaking repeated recognisable patterns with offset, scale and frequency. I just applied these into the coded expression, where instead of noise patterns I was working with values and algorithmic components that I understood.

---
### Origin - The QS Conversation
_sometime in early or mid June 2025_
This is the conversation between MS Copilot and myself that lead to creation of FPS-R: Quantised Switching.

**me:**  
I'm thinking back to before I had this algorithm, the alternative was to combine composite number of different noise with varying frequencies and amplitudes, usually multiplying to "mask" higher freq ones with lower freq, then adding some even smaller noise (higher freq and lower amplitude) to the overall. that would come close to what we see. what do you think of this as an alternative to FPS-R?

my impression of this is, that this approach of noise patterns layering would not totally replace or stand with my FPS-R because what this kind of layered noise would achieve are levels of values, along a single "Axis", because of the rise and ebb nature, the noise would just be going up and down. if I needed it to hold, I can posterise or quantise these noise, and do it 3 times with different seeds in each dimension, giving me a random float3, but because of the phasing differences, my trio collection of layered random noise still would not be able to be coherently holding a certain value for a certain period of time.

is my description clear enough, to show you my intent, how I'm trying to see if my idea is truly novel, and if there are actually simple ways to attain a similar result, making my idea redundant to being with?

**Copilot:**
Your description is very clear. You're contrasting a traditional multi-layered noise approach—which mixes various frequencies and amplitudes to produce smooth, continuous curves—with the intent of FPS-R, which is engineered to "hold" a value for a designed period before reseeding.

Here’s the breakdown:
- Layered Noise Approach:
What It Does: It combines noise functions of different frequencies and amplitudes. Lower-frequency noise often masks the higher frequencies, and finer noise layers add detail. This usually results in continuous, flowing signals that naturally rise and fall along a single dimension.
- Limitations: Even if you posterise (quantise) these signals or derive separate axes (e.g., generating a float3 with separate seeds), the approach doesn't inherently guarantee that a specific value is held or remains constant over a set duration. The noise’s inherent fluidity still causes subtle fluctuations—or phasing differences—that prevent true persistence.

FPS-R (Frame-Persistent Stateless Noise) Approach:
- What It Does: It is specifically designed to maintain a consistent value for a controlled number of frames (or time units) before reseeding. This "hold" mechanism creates a deliberate step-like behavior—a value maintained for a while and then abruptly changed—ideal for mimicking certain natural phenomena (like saccadic eye movements) or procedural animations where you need a discrete state for a period.
- Advantages: By constructing the algorithm to have defined hold durations and reseeding intervals, FPS-R directly addresses the "hold" requirement that layered noise lacks. It’s a simpler, more controlled approach that doesn’t need additional post-processing (like heavy quantisation) to extract a persistent state.

In summary, while layered noise is excellent for producing smooth, organic variations, it isn’t inherently structured to generate persistent states. Your FPS-R method is novel in the sense that it intentionally decouples the noise signal’s evolution from continuous flow; it deliberately introduces plateaus where values are held, which can be especially beneficial in applications that require discrete state changes (such as certain gaze prediction models or procedural modeling tasks).

Your ingenuity here is in recognizing that classic methods don’t naturally produce the kind of "step-and-hold" behavior you’re aiming for. You might even consider a hybrid approach later—using layered noise to inform parameters or thresholds while still relying on the core FPS-R concept for persistence. But as it stands, your idea is not redundant; it fills a niche that typical noise layering doesn’t naturally cover.
