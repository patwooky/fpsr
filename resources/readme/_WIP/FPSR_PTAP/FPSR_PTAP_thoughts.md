# FPS-R Origins, Journal and Reflections
MIT License—[see LICENSE](../../LICENSE) for details.  
Copyright (c) 2025 Woo Ker Yang (Patrick Woo) patrickwoo.1976@gmail.com
If you reference or adapt this framework, please credit Patrick Woo and this repository.
**This documentation is still in development.** 
While every update strives to be more accurate, there will be parts that are incomplete or inaccurate. 

# Introduction
This is the development documentation for a novel protocol with a phrasing capability, through FPS-R (Frame-Persistent Stateless Randomisation) framework.

## Phrase-Timing Authentication Protocol (P-TAP)
### Inception of Phrase-Timing Authentication Protocol (P-TAP)
_21 Aug 2025 Thursday_
I was having lunch with my wife and I thought about how FPS-R can contribute to cybersecurity, networking and authentication.

I searched for the well known 30 second and 60 second Google Authenticator code refresh cycle. I came up with an idea of a phrasing system that would use FPS-R to create deliberate random holds on the cycle durations, perhaps between 30 to 90.

### Initial Concept of P-TAP
_22 Aug 2025 Thursday_
Here is the write-up of the initial concept.
#### Practical Application: Enhancing Authenticator Security with Phrased Timing
This section outlines a novel multi-factor authentication protocol that leverages FPS-R to enhance security. It serves as a more secure, dynamic alternative to the current standard of Time-Based One-Time Passwords (TOTP), which rely on a fixed, predictable 30-second refresh interval. 

**The "Secret Handshake" Protocol**
This new protocol introduces a "secret handshake" that establishes an unpredictable, phrased rhythm for when new authentication codes become valid, making timing-based attacks significantly more difficult.

**The Protocol in Context**
This protocol is designed as a Multi-Factor Authentication (MFA) system. It provides a secure second factor that a user or machine provides after a primary authentication method (like a password) has been successful. The core innovation is replacing the fixed time interval of TOTP with a secret, deterministically generated, and unpredictable one.

**Phase 1: The One-Time Setup**
This foundational step happens only once when a user enrolls a new device.
1. **Secure Connection**: The entire process occurs over a secure, encrypted channel (like HTTPS).
2. **Master Secret Generation**: The server uses a Cryptographically Secure Pseudo-Random Number Generator (CSPRNG) to create a unique and unpredictable master secret key. This key is the root of trust for the enrolled device.
3. **Sharing the Secret**: The server shares this master secret with the client (e.g., a mobile authenticator app), typically via a QR code.

At the end of this step, both the server and the client securely store the exact same master secret key.

**Phase 2: Login and Re-synchronization (The "Secret Handshake")**
1. This dynamic process occurs each time a user or machine needs to authenticate.
Initial Authentication: The user or machine provides their primary credential (e.g., password).
2. The Handshake: Upon successful primary authentication, the server generates a synchronization tuple containing the "playbook" for the upcoming session and sends it securely to the client.
3. Independent Generation: For a set number of cycles, both the client and server follow the playbook independently, without further communication. They use the shared FPS-R parameters to deterministically calculate the same sequence of unpredictable refresh timings and the same 6-digit authentication codes (generated via a standard HMAC algorithm seeded by FPS-R).

The synchronisation tuple would look like this:
```c
// Define a struct for the synchronization packet
typedef struct {
    int start_time;
    int cycle_time_min;
    int cycle_time_max;
    int number_of_cycles;
    int playbook_idx;
    int algorithm_idx;
    int fpsr_params_idx;
} FpsrSyncPacket;
```
- `start_time`: A Unix timestamp (seconds since epoch) to anchor the sequence in real-world time and help detect clock drift.
- `fpsr_algorithm_index`: The integer number to indicate the FPS-R algorithm used, ensuring both sides use the same engine.
- `cycle_time_min` & `cycle_time_max`: An array that maps FPS-R's normalized 0-1 output to a specific range of real-world hold durations (e.g., a `randVal` of 0.5 might map to a 42-second refresh interval).
- `number_of_cycles`: An integer defining how many refresh cycles both sides will independently generate before they must perform a new handshake.
- `playbook_idx`: Selects a specific LUT playbook index from a preloaded list of playbooks
- `algorithm_idx` & `fpsr_params_idx`: A single integer that points to a specific, pre-shared set of FPS-R parameters (e.g., algorithm type, seeds). This avoids transmitting verbose settings, adding another layer of security.

A crucial failsafe is built into the protocol: if any generated hold duration would exceed a maximum secure lifetime (e.g., 90 seconds), both client and server are programmed to reduce the hold duration by first attempting to reduce the `number_of_cycles`, failing which, both will discard the current frame and advance to the next valid frame in the FPS-R sequence. These deterministic logic are applied on both client and server ensure they are secure and will always remain in sync.

**Mechanics of the Exchange**
Upon a successful login periodic refresh, the server and client perform the secret handshake, securely exchanging the synchronisation tuple. From that point on, both systems operate independently, using the `playbook_idx`, `fpsr_algorithm_index` and `fpsr_params_idx` to select from a shared secret list of "playbooks" (e.g., `[0,0,1,2]`), FPS-R algorithms and related parameters.

Starting from the agreed-upon `fpsr_index`, both systems step through the playbook in a repeating loop to execute the required `number_of_cycles`. The length of the playbook and the number of required cycles are decoupled:
* If the playbook contains more cycles than required, any remaining tasks in the playbook are discarded once the `number_of_cycles` is met.
* If the playbook contains fewer cycles than required, the systems will loop back to the beginning of the playbook and continue until the `number_of_cycles` is met, discarding the remaining tasks in the playbook.

When the `number_of_cycles` cycles has been reached, the client and server goes for another round of secret handshake. 

The is system ensures that the server and client can perfectly mirror each other's timing and values. This creates a complex, phrased synchronisation rhythm that is unknown to outside observers and cannot be replicated. 

### Update and Naming of PTAP
_22 Aug 2025 Thursday_
Named the protocol
#### Considerations: 
##### Time Representation of a Frame in FPS-R
**One Step Per Cycle**
I wanted 1 step in the playbook to represent 1 cycle. If the current cycle is 49 seconds, the playbook step increment to the next steps after 49 seconds.

**Challenges:**
1. **Expressive Phrasing**. There are not enough steps for FPS-R to move. 
    - Acceptable range for cycles are 30s-90s. Any shorter, the code will jump before humans can comfortably look-up and input the code. Wait too long to refresh, there will be a very real probability that the valid code can be discovered and exploited. 
      - if the current generated cycle duration is set to 30s, FPS-R can only hold the same values for a maximum of 3 cycles (3 x 30s = 90s) before the upper limit of held code duration is reached, and code will need to refresh.
      - if the current generated cycle duration is set to 45s, FPS-R can only hold the same values for a maximum of 2 cycles (2 x 45s = 90s) before hitting the max upper limit.
      - any cycle duration value larger than 45s can only be held for a single cycle. 
      - this constrains and ties down FPS-R from doing its natural phrasing. we can set FPS-R to a `minHold`, `maxHold` or `reseedFrames` to 1, 3 and 2 respectively for some interesting variance, but the "runway" is short, there is no room for it to express its natural tendency to hold and phrase.
    - when FPS-R is constrained to 1- or 2-frame holds, it acts like a regular rand(), no difference. (which is not strictly true, I'll explain more later)

Gemini got the impression and idea that 1 playbook index increment can be 1 second in system time. This makes FPS-R phrase better.
**Advantages**:
1. Gemini would be able to "run free" to express its phrasing.
**Challenges**:
1. Absolute control over held durations
    - The held durations in FPS-R cannot be guaranteed. `minHold` in FPS-R SM is not a promise, it is a "probabilistic suggestion" (I have to repeat that FPS-R is deteministic).
      - For SM and TM, this unpredictability has something to do with the different frame offsets in each layer of modulo operations and the switching/reseeding mechanism. they all operate at different points on offset  timelines, when a jump happens it lands in the middle of the timeline of another modulo cycle. coupled with the moving parts there is no way to predict or ensure `minHold` frames is observed.
    - This can cause premature jumps before the `minHold` period is up. if this happens within the min 30s bracket, and the user is entering the digits and FPS-R decides to jump, the digits become invalid part-way through. This creates bad experiences for the user.
    - It is possible to arrive at parameters that would result in correctly behaving phrasing _most of the time_. But given the reasons mentioned, there will always be a chance that the held durations will jump half-way.
    - we can create stateful ways to suppress it but it will make the logic more complex than it has to be. 

When I came up with the idea of the playbook, I have already considered this. However, explaining it to Gemini again made the reasons clearer. Gemini agrees with my assessment.



##### Power Loss and Re-Sync
When there's a power-loss and the session is still going on, the protocol needs to get back into step.

**Power Loss and Restart Occurances** 
How often do restarts or loss of power occur across potential devices that will use PTAP?

**Mechanism of Playbook Playback Run-Up**


#### Redefining the Playbook
**Initial Playbook**
Initially, the playbook format was this:
(e.g., [0,0,1,2], [1,0,0,2,0,0,0,2,0,1,0,0], etc). Where: 
  - `0` - drop frame
  - `1` - Generate Auth Code
  - `2` - Generate Cycle Duration

**Mechanics:**
Only elements in the array that are 1 and 2 are significant. They are the actionable tasks. Drop frames are "skipped frames" that serve to advance the `start_frame`.

**An Example**
Take [0, 0, 1, 2], with a start frame of 150
- the first operation is at index 2 which has the value of 1. This is a generate Auth Code task. 
  - so I'll run `generate_auth_code(frame)`
  - where `frame` is the seed, which is `start_frame + index + 1`, giving us `103`.
  - so `generate_auth_code(103)`
- the second operation is at index 3 that has the vlaue of 2. This is a generate Cycle Duration task.
  - so I'll run `float randVal = fpsr_sm(frame)` that gives a normalised value between `0` and `1`. Say randVal is 0.7.
  - map randVal to `cycle_time_min` and `cycle_time_max`. Say both are 30 and 60 respectively. 
  - so this cycle duration is `(cycle_time_max - cycle_time_min) * 0.7` which is `floor(30s * 0.7)` giving us `21s`.

**Combining Tasks 1 and 2**

#### Can FPS-R be Replaced by `rand()`?
In the Playbook ecosystem, can `rand()`, a pure random value generator replace FPS-R?
Here are some pros and cons:
- **preserved**: but the random number calculated for each cycle would still be deterministic.
- **preserved**: the playbook concept can still be used to drop or skip frames, the tasks can still be carried out.
- **lost**: we lose the layer of phrasing, of being able to hold certain durations. `rand()` on its own is very eager to change to a new value with every change of the seed value. Lightning does not strike twice, it is practically improbable that `rand` generate the same value twice, and practically impossible that `rand` will generate the same value thrice. Hence with `rand` we have almost no chance of a duration holding across cycles.
- **lost**: the FPS-R algorithms to use and switch across secret handshakes, resulting in one less layer of obfuscation
- **lost**: FPS-R specific parameters that can add another layer of obfuscated behind integers pointing to which preset it is using.

I argued to bring FPS-R back again, even if the practically allowed hold cycles are 3 cycles at most.

##### The Argument for FPS-R in P-TAP
As mentioned above `rand()` give extremely low probability (almost never) of a holding duration values across cycles.
With FPS-R, it is possible. It has a phrasing that wants the values to stay for a while. Its purpose is to **intentionally create correlation** between consecutive frames. It takes a random value and purposefully holds it, reducing the volatility to create a rhythmic phrase. It represents **low-entropy, phrased randomness**.

In "Rock, Paper, Scissors", if I keep giving scissors my opponent will be caught off guard expecting me to keep varying between rock, paper and scissors like how other people generally play. 
With `rand`, every cycle duration will be different. That in itself is a "tell" , for the enemy to spot.
FPS-R breaks this meta-pattern. An occasional, unexpected hold for two or three cycles is the "scissors, scissors, scissors" move that defies the expected pattern. It introduces a **higher level of entropy** into the behavior of the protocol itself, making it much harder to profile.

I mentioned that FPS-R limiting FPS-R's natural rhythm by "caging" it in with short parameters like `minHold` = 1, `maxHold` = 3 and `reseed_frame` = 2 in Stacked Modulo, it will start to act like `rand`, and there would be no difference. But here is the difference:

This configuration gives us the best of both worlds:

Most of the time, FPS-R will behave just like `rand()`, producing a new value for each cycle and respecting the natural flow of the playbook.

Every now and then, when the "stars align" in the algorithm when the drifting phase come into alignment, it will produce a short hold. This is the unexpected event that strengthens the protocol.

Finally, we still get to obfuscate all the FPS-R parameters and the algorithm type behind integers adding another 2 layers.

Each additional layer will add multiplicative growth to the number of combinations available that a malicious attacker has to go through to figure out the code. This effectively reduces the chance of him stubling upon or converging upon the correct code by brute force or chance.