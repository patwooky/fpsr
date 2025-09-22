# SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

'''
file: fpsr_algorithms.py
brief: Python implementation of FPS-R algorithms: 
    Stacked Modulo (SM), Toggled Modulo (TM) and Quantised Switching (QS).
details: 
    FPS-R (Frame-Persistent Stateless Randomisation) is a set of three algorithms that
    generate frame-persistent and stateless random values. 
    This file contains three stateless, frame-persistent randomization algorithms.
    It uses a custom portable_rand() function to ensure deterministic and consistent results across any platform.
'''

import math

# -----------------------------------------------------------------------------
# Deterministic helpers and PRNG matching the C reference implementation
# -----------------------------------------------------------------------------
# Why these helpers?
# - Python's % and // already use floor semantics for negatives, which we mirror in C.
#   We still define explicit helpers so both languages call the same logical steps.
# - All frame/seed/duration math remains in the integer domain (Python int is arbitrary
#   precision). Where the C code relies on uint64_t wraparound, we emulate it with masks.
# - All fractional math that converts to/from integers uses Python's float (IEEE-754
#   double) and math.floor to match C 'double' behavior, ensuring bit-for-bit parity.

# 64-bit mask for emulating uint64_t wraparound exactly like C.
_UINT64_MASK = (1 << 64) - 1

def _to_uint64(x: int) -> int:
    """Cast any Python int to an emulated uint64_t by masking to 64 bits.
    This reproduces C's well-defined unsigned wraparound and is essential for
    deterministic PRNG behavior across languages and platforms.
    """
    return x & _UINT64_MASK

# Floor-based modulo that matches Python's a % m for negative a (m>0).
# We expose it explicitly to mirror the C helper and document the determinism intent.
def i64_floor_mod(a: int, m: int) -> int:
    """Return a modulo m using floor semantics, identical to Python's % for m>0.
    C's % truncates toward zero, which diverges for negative a; by always using
    floor-mod here (and in C), we guarantee alignment logic matches exactly.
    """
    # Assumes m > 0 by contract.
    return a % m

# Align down to the nearest multiple of m using floor-based modulo.
# This mirrors C's i64_align_down and Python's 'a - (a % m)' even when a < 0.
def i64_align_down(a: int, m: int) -> int:
    """Align a down to a multiple of m using floor-mod semantics.
    Using this helper wherever alignment is needed ensures C/Python parity.
    """
    return a - i64_floor_mod(a, m)

# SplitMix64: portable 64-bit mixer with well-defined unsigned wraparound.
# Each arithmetic step is masked to uint64 to exactly mirror C's uint64_t behavior.
def _splitmix64(x: int) -> int:
    x = _to_uint64(x + 0x9E3779B97F4A7C15)
    x = _to_uint64((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9)
    x = _to_uint64((x ^ (x >> 27)) * 0x94D049BB133111EB)
    x = _to_uint64(x ^ (x >> 31))
    return x

# Portable, deterministic PRNG that returns a double in [0,1).
# Uses the top 53 bits of the 64-bit output to match IEEE-754 double mantissa size.
# Implemented identically in C and Python to yield bit-for-bit identical floats.
def portable_rand_u64(seed: int) -> float:
    r = _splitmix64(_to_uint64(seed))
    return float((r >> 11)) * (1.0 / 9007199254740992.0)  # 2^53

# Back-compat wrapper with the old name/signature.
# Prefer passing integer seeds; if a float is provided (legacy), we floor it to
# remove ambiguity and to align with C's explicit use of floor() before casts.
def portable_rand(seed):
    """
    A simple, portable pseudo-random number generator.
    Generates a deterministic float between 0.0 and 1.0 from an integer-like seed.
    This wrapper forwards to a 64-bit deterministic PRNG that mirrors the C code
    (SplitMix64 + top-53-bit mapping), ensuring cross-language bit-for-bit parity.

    Args:
        seed (int|float): An integer-like seed. Floats are floored for determinism.

    Returns:
        float: A pseudo-random float between 0.0 and 1.0.
    """
    if isinstance(seed, float):
        seed = math.floor(seed)
    else:
        seed = int(seed)
    return portable_rand_u64(seed)


"""
--------------------------
FPS-R: Stacked Modulo (SM)
--------------------------
"""

def fpsr_sm(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch=True):
    """
    Produces a pseudo-random value that persists across multiple frames, held for a calculated duration.
    The hold timing varies over time, driven by deterministic interference between reseeded modular rhythms.
    This method mimics structured hesitation and twitch-like behavior—creating motion that feels deliberate without relying on state or memory.

    Args:
        frame (int): The current frame or time input.
        minHold (int): The minimum duration (in frames) for a value to hold.
        maxHold (int): The maximum duration (in frames) for a value to hold.
        reseedInterval (int): The fixed interval at which a new hold duration is calculated.
        seedInner (int): An offset for the random duration calculation to create unique sequences.
        seedOuter (int): An offset for the final value calculation to create unique sequences.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw integer state value.
    """
    # --- 1. Calculate the random hold duration ---
    if reseedInterval < 1:
        reseedInterval = 1  # Prevent division by zero.

    # Use floor-based modulo to match C helper and Python semantics for negatives.
    reseed_anchor = (seedInner + frame) - i64_floor_mod(frame, reseedInterval)

    # Deterministic PRNG over 64-bit integer seed; result is double in [0,1).
    rand_for_duration = portable_rand_u64(reseed_anchor)

    # Compute duration with double intermediates then floor to int, mirroring C.
    holdDuration = math.floor(float(minHold) + rand_for_duration * float(maxHold - minHold))

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # Align down using floor-mod semantics for negative inputs to ensure parity.
    held_integer_state = i64_align_down((seedOuter + frame), holdDuration)

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # Keep seed math in 64-bit integer space; emulate uint64 wraparound.
        seed_u64 = _to_uint64(held_integer_state) * 100000
        seed_u64 &= _UINT64_MASK
        fpsr_output = portable_rand_u64(seed_u64)
    else:
        # Return the raw integer state as a float (matches C's cast).
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# Sample code to call the function
# Parameters
frame = 100  # Replace with the current frame value
minHoldFrames = 16  # probable minimum held period
maxHoldFrames = 24  # maximum held period before cycling
reseedFrames = 9    # inner mod cycle timing
offsetInner = -41   # offsets the inner frame
offsetOuter = 23    # offsets the outer frame
use_final_random = True # Set to False to bypass final randomization

# # Call the FPS-R:SM function
# randVal = fpsr_sm(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random)
# # Another call to fpsr_sm for the previous frame
# randVal_previous = fpsr_sm(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random)
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Stacked Modulo (SM) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}\n')

# end of fpsr_sm function


"""
---------------------------
FPS-R: Toggled Modulo (TM)
---------------------------
"""

def fpsr_tm(frame, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch=True):
    """
    Generates a persistent value that holds for a rhythmically toggled duration.
    This function uses a deterministic switch to toggle the hold duration
    between two fixed periods. This creates a predictable, rhythmic, or mechanical
    "move-and-hold" pattern, as opposed to the organic randomness of SM.

    Args:
        frame (int): The current frame or time input.
        periodA (int): The first hold duration (in frames).
        periodB (int): The second hold duration (in frames).
        periodSwitch (int): The fixed interval at which the hold duration is toggled.
        seedInner (int): An offset for the toggle clock to de-sync it from the main clock.
        seedOuter (int): An offset for the main clock to create unique output sequences.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw integer state value.
    """
    # --- 1. Determine the hold duration by toggling between two periods ---
    if periodSwitch < 1:
        periodSwitch = 1  # Prevent division by zero.

    # The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    inner_clock_frame = seedInner + frame
    
    # Use floor-based modulo for cross-language consistency with the C helper.
    r = i64_floor_mod(inner_clock_frame, periodSwitch)

    # Toggle threshold at exactly half the period using integer math (no FP rounding).
    holdDuration = periodA if (2 * r) < periodSwitch else periodB

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    outer_clock_frame = seedOuter + frame
    held_integer_state = i64_align_down(outer_clock_frame, holdDuration)

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # Deterministic seeding in the uint64 domain with wraparound, mirroring C.
        seed_u64 = (_to_uint64(held_integer_state) * 100000) & _UINT64_MASK
        fpsr_output = portable_rand_u64(seed_u64)
    else:
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# Sample code to call the FPS-R:TM function
# Parameters
frame = 100  # Replace with the current frame value
period_A = 10  # The first hold duration
period_B = 25  # The second hold duration
switch_duration = 30  # The toggle happens every 30 frames
offset_inner = 15  # offsets the inner (toggle) clock
offset_outer = 0  # offsets the outer (hold) clock
use_final_random = True  # Set to False to bypass final randomization

# # Call the FPS-R:TM function
# randVal = fpsr_tm(frame, period_A, period_B, switch_duration, offset_inner, offset_outer, use_final_random)
# # Another call to fpsr_tm for the previous frame
# randVal_previous = fpsr_tm(frame - 1, period_A, period_B, switch_duration, offset_inner, offset_outer, use_final_random)
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Toggled Modulo (TM) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}\n')

# end of fpsr_tm function



"""
-------------------------------
FPS-R: Quantised Switching (QS)
-------------------------------
"""

def fpsr_qs(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets,
            streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch=True):
    """
    Generates a flickering, quantised value by switching between two sine wave streams.
    For each stream, a new random quantisation level is chosen from within the [min, max] 
    range at a set interval. The function then switches between these two streams to create
    complex, glitch-like patterns.
    
    Args:
        frame (int): The current frame or time input.
        baseWaveFreq (float): The base frequency for the modulation wave of stream 1.
        stream2FreqMult (float): A multiplier for the second stream's frequency.
        quantLevelsMinMax (list[int]): A list of two integers for the min and max quantisation levels.
        streamsOffset (list[int]): A list of two integers to offset the frame for each stream's sine wave.
        quantOffsets (list[int]): A list of two integers to offset the random quantisation selection for each stream.
        streamSwitchDur (int): The number of frames after which the streams switch.
        stream1QuantDur (int): The duration for which stream 1's random quantisation level is held.
        stream2QuantDur (int): The duration for which stream 2's random quantisation level is held.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw stepped signal value, scaled to the [0, 1] range.
    """
    # --- 1. Set default durations if not provided ---
    if streamSwitchDur < 1: streamSwitchDur = math.floor((1.0 / baseWaveFreq) * 0.76)
    if stream1QuantDur < 1: stream1QuantDur = math.floor((1.0 / baseWaveFreq) * 1.2)
    if stream2QuantDur < 1: stream2QuantDur = math.floor((1.0 / baseWaveFreq) * 0.9)
    
    # Ensure durations are at least 1 frame to prevent division by zero.
    streamSwitchDur = max(streamSwitchDur, 1)
    stream1QuantDur = max(stream1QuantDur, 1)
    stream2QuantDur = max(stream2QuantDur, 1)

    # --- 2. Calculate random quantisation levels for each stream ---
    quant_min = quantLevelsMinMax[0]
    quant_max = quantLevelsMinMax[1]
    quant_range = quant_max - quant_min + 1

    # --- Stream 1 Quant Level ---
    s1_quant_seed_aligned = i64_align_down((quantOffsets[0] + frame), stream1QuantDur)
    s1_rand_for_quant = portable_rand_u64(s1_quant_seed_aligned)
    s1_quant_level = quant_min + math.floor(s1_rand_for_quant * float(quant_range))

    # --- Stream 2 Quant Level ---
    s2_quant_seed_aligned = i64_align_down((quantOffsets[1] + frame), stream2QuantDur)
    s2_rand_for_quant = portable_rand_u64(s2_quant_seed_aligned)
    s2_quant_level = quant_min + math.floor(s2_rand_for_quant * float(quant_range))

    s1_quant_level = max(s1_quant_level, 1)
    s2_quant_level = max(s2_quant_level, 1)

    # --- 3. Generate the two quantised sine wave streams ---
    if stream2FreqMult < 0: stream2FreqMult = 3.7

    # Deterministic double math: sin() -> [-1,1], map to [0,1], quantise via floor.
    stream1 = math.floor((math.sin((streamsOffset[0] + frame) * baseWaveFreq) * 0.5 + 0.5) * s1_quant_level) / s1_quant_level
    stream2 = math.floor((math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * 0.5 + 0.5) * s2_quant_level) / s2_quant_level

    # --- 4. Switch between the two streams ---
    # Use floor-mod and an integer half-threshold (2*r < period) to match C.
    r = i64_floor_mod(frame, streamSwitchDur)
    active_stream_val = stream1 if (2 * r) < streamSwitchDur else stream2

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        # Derive a stable integer seed from the double stream using floor(), then hash.
        # This avoids ambiguous float->int casts and reproduces exactly in C.
        hashed_int = math.floor(active_stream_val * 100000.0)
        fpsr_output = portable_rand_u64(_to_uint64(hashed_int))
    else:
        # If finalRandSwitch is false, scale the [0,1] stream value to [0,1] as in C.
        fpsr_output = 0.5 * active_stream_val + 0.5
        
    return fpsr_output

# Sample code to call the FPS-R:QS function
# Parameters
frame = 103  # Current frame number
baseWaveFreq = 0.012  # Base frequency for the modulation wave of stream 1
stream2freqMult = 3.1  # Multiplier for the second stream's frequency
quantLevelsMinMax = [4, 12]  # Min, Max quantisation levels for the two streams
streamsOffset = [0, 76]  # Offset for the two streams' sine waves
quantOffsets = [10, 81] # Offset for the random quantisation selection
streamSwitchDur = 24  # Duration for switching streams in frames
stream1QuantDur = 16  # Duration for the first stream's quantisation switch cycle in frames
stream2QuantDur = 20  # Duration for the second stream's quantisation switch cycle in frames
use_final_random = True # Set to False to bypass final randomization

# # Call the FPS-R:QS function
# randVal = fpsr_qs(
#     frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
#     streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random
# )

# # Another call to fpsr_qs for the previous frame
# randVal_previous = fpsr_qs(
#     frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
#     streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random
# )
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Quantised Switching (QS) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}')

# end of fpsr_qs function

# /******************************************************************************/
# /* Main function to demonstrate usage of FPS-R algorithms                     */
# /******************************************************************************/
if __name__ == "__main__":
    # algorithms: 0 - sm, 1 - tm, 2 - qs
    algo = 0  # Change this value to 0, 1, or 2 to test different algorithms
    algo_name = ["SM", "TM", "QS"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103]  # starting frames for each algorithm
    num_frames = 30  # run a loop of x frames to demonstrate changes
    
    # create main for loop to demonstrate changes
    for loop_frame in range(num_frames):
        frame = loop_frame + start_frames[algo]  # starting frame for the selected algorithm
        randVal = 0.0  # variable to hold the random value output
        randVal_previous = 0.0  # variable to hold the previous frame's random value
        changed = 0  # Variable to track if the value has changed

        if algo == 0:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:SM function
            # --------------------------------------------------------------------------
            # Parameters
            minHoldFrames = 12  # probable minimum held period
            maxHoldFrames = 21  # maximum held period before cycling
            reseedFrames = 7  # inner mod cycle timing
            offsetInner = -41  # offsets the inner frame
            offsetOuter = 23  # offsets the outer frame
            finalRandSwitch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:SM function        
            # call to fpsr_sm for the current frame
            randVal = float(fpsr_sm(
                int(loop_frame), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            # another call to fpsr_sm for the previous frame
            randVal_previous = float(fpsr_sm(
                int(loop_frame - 1), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        elif algo == 1:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:TM function
            # --------------------------------------------------------------------------
            # Parameters
            period_A = 6  # The first hold duration
            period_B = 8  # The second hold duration
            periodSwitch = 10  # The toggle duration between periods A and B in frames
            offset_inner = 15  # offsets the inner (toggle) clock
            offset_outer = 0  # offsets the outer (hold) clock
            final_rand_switch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:TM function
            # call to fpsr_tm for the current frame
            randVal = float(fpsr_tm(
                int(loop_frame), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            # another call to fpsr_tm for the previous frame
            randVal_previous = float(fpsr_tm(
                int(loop_frame - 1), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        elif algo == 2:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:QS function
            # --------------------------------------------------------------------------
            # Parameters
            baseWaveFreq = 0.012  # Base frequency for the modulation wave of stream 1
            stream2freqMult = 3.1  # Multiplier for the second stream's frequency
            quantLevelsMinMax = [4, 12]  # Min, Max quantisation levels for the two streams
            streamsOffset = [0, 76]  # Offset for the two streams
            quantOffsets = [10, 81]  # Offset for the random quantisation selection
            streamSwitchDur = 14  # Duration for switching streams in frames
            stream1QuantDur = 6  # Duration for the first stream's quantisation switch cycle in frames
            stream2QuantDur = 9  # Duration for the second stream's quantisation switch cycle in frames
            finalRandSwitch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # call to fpsr_qs for the current frame
            randVal = float(fpsr_qs(
                int(loop_frame), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), bool(finalRandSwitch)))
            # another call to fpsr_qs for the previous frame
            randVal_previous = float(fpsr_qs(
                int(loop_frame - 1), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), bool(finalRandSwitch)))
            changed = 0  # Variable to track if the value has changed
            if randVal != randVal_previous:
                changed = 1  # Mark as changed if the value has changed from the previous frame
        
        # Mirror C printf formatting and two-step print for the suffix
        print(f"Frame {frame}: randVal {randVal:.6f}, randVal_previous {randVal_previous:.6f}, changed {changed} ", end="")
        print("(jumped)" if changed else "")