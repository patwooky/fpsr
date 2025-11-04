# SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

'''
file: fpsr_algorithms.py
brief: Python implementation of FPS-R algorithms: 
    Stacked Modulo (SM), Toggled Modulo (TM), Quantised Switching (QS), and Bitwise Decode (BD).
details: 
    FPS-R (Frame-Persistent Stateless Randomisation) is a set of algorithms that
    generate frame-persistent and stateless random values. 
    This file contains four stateless, frame-persistent randomization algorithms.
    It uses a custom portable_rand() function to ensure deterministic and consistent results across any platform.
'''

import math
import functools
# [PORT UPDATE] Import 'struct' module. This is needed for fpsr_qs to perform
# a bit-for-bit cast from a float (double) to a 64-bit integer,
# matching the C version's 'memcpy' logic.
import struct

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

# Bit-width used for chunked bit operations. It must remain 64 for deterministic
# compatibility with SplitMix64 and the 64-bit masking below.
# This value is not meant to be changed. DO NOT MODIFY.
_CHUNK_BITS = 64

# 64-bit mask for emulating uint64_t wraparound exactly like C.
_UINT64_MASK = (1 << _CHUNK_BITS) - 1

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
    # [PORT UPDATE] C-version passes a uint64_t. We emulate this by masking
    # the input seed *first* before passing to splitmix64.
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

# --- Bitwise Rotation Helpers ---

def _circular_left_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular left shift (rotate left)."""
    # [PORT UPDATE] Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value << shift) | (value >> (_CHUNK_BITS - shift)))

def _circular_right_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular right shift (rotate right)."""
    # [PORT UPDATE] Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value >> shift) | (value << (_CHUNK_BITS - shift)))


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
               If False, the raw integer state value (as a float).
    """
    # --- 1. Calculate the random hold duration ---
    if reseedInterval < 1:
        reseedInterval = 1  # Prevent division by zero.

    # Use floor-based modulo to match C helper and Python semantics for negatives.
    # [PORT UPDATE] Cast seed to int to ensure portable_rand_u64 receives an int
    reseed_anchor = int(seedInner + frame) - i64_floor_mod(int(frame), int(reseedInterval))

    # Deterministic PRNG over 64-bit integer seed; result is double in [0,1).
    rand_for_duration = portable_rand_u64(reseed_anchor)

    # Compute duration with double intermediates then floor to int, mirroring C.
    holdDuration = math.floor(float(minHold) + rand_for_duration * float(maxHold - minHold))

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # Align down using floor-mod semantics for negative inputs to ensure parity.
    held_integer_state = i64_align_down(int(seedOuter + frame), int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # [PORT UPDATE] Match the canonical C implementation.
        # The C version directly uses the 64-bit 'held_integer_state' as the
        # seed. The previous Python version multiplied this by 100,000.
        # This update removes that multiplication to align with the C reference.
        # portable_rand_u64 will handle casting to uint64 internally.
        fpsr_output = portable_rand_u64(held_integer_state)
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
               If False, the raw integer state value (as a float).
    """
    # --- 1. Determine the hold duration by toggling between two periods ---
    if periodSwitch < 1:
        periodSwitch = 1  # Prevent division by zero.

    # The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    inner_clock_frame = int(seedInner + frame)
    
    # Use floor-based modulo for cross-language consistency with the C helper.
    r = i64_floor_mod(inner_clock_frame, int(periodSwitch))

    # Toggle threshold at exactly half the period using integer math (no FP rounding).
    holdDuration = periodA if (2 * r) < periodSwitch else periodB

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    outer_clock_frame = int(seedOuter + frame)
    held_integer_state = i64_align_down(outer_clock_frame, int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # [PORT UPDATE] Match the canonical C implementation.
        # The C version directly uses the 64-bit 'held_integer_state' as the
        # seed. The previous Python version multiplied this by 100,000.
        # This update removes that multiplication to align with the C reference.
        # portable_rand_u64 will handle casting to uint64 internally.
        fpsr_output = portable_rand_u64(held_integer_state)
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
    streamSwitchDur = max(int(streamSwitchDur), 1)
    stream1QuantDur = max(int(stream1QuantDur), 1)
    stream2QuantDur = max(int(stream2QuantDur), 1)

    # --- 2. Calculate random quantisation levels for each stream ---
    quant_min = int(quantLevelsMinMax[0])
    quant_max = int(quantLevelsMinMax[1])
    quant_range = quant_max - quant_min + 1

    # --- Stream 1 Quant Level ---
    s1_quant_seed_aligned = i64_align_down(int(quantOffsets[0] + frame), stream1QuantDur)
    s1_rand_for_quant = portable_rand_u64(s1_quant_seed_aligned)
    s1_quant_level = quant_min + math.floor(s1_rand_for_quant * float(quant_range))

    # --- Stream 2 Quant Level ---
    s2_quant_seed_aligned = i64_align_down(int(quantOffsets[1] + frame), stream2QuantDur)
    s2_rand_for_quant = portable_rand_u64(s2_quant_seed_aligned)
    s2_quant_level = quant_min + math.floor(s2_rand_for_quant * float(quant_range))

    s1_quant_level = max(s1_quant_level, 1)
    s2_quant_level = max(s2_quant_level, 1)

    # --- 3. Generate the two quantised sine wave streams ---
    if stream2FreqMult < 0: stream2FreqMult = 3.7

    # Deterministic double math: sin() -> [-1,1], map to [0,1], quantise via floor.
    angle1 = (float(streamsOffset[0]) + float(frame)) * float(baseWaveFreq)
    angle2 = (float(streamsOffset[1]) + float(frame)) * float(baseWaveFreq) * float(stream2FreqMult)
    
    stream1 = math.floor((math.sin(angle1) * 0.5 + 0.5) * float(s1_quant_level)) / float(s1_quant_level)
    stream2 = math.floor((math.sin(angle2) * 0.5 + 0.5) * float(s2_quant_level)) / float(s2_quant_level)

    # --- 4. Switch between the two streams ---
    # Use floor-mod and an integer half-threshold (2*r < period) to match C.
    r = i64_floor_mod(int(frame), streamSwitchDur)
    active_stream_val = stream1 if (2 * r) < streamSwitchDur else stream2

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        # [PORT UPDATE] Match the canonical C implementation.
        # The C version performs a 'memcpy' of the double 'active_stream_val'
        # into a uint64_t to use its raw bits as a seed.
        # The Python equivalent is struct.pack/unpack, which does the same
        # bit-for-bit cast from a float (C double) to an unsigned 64-bit int.
        # The old Python code (math.floor(active_stream_val * 100000.0))
        # was a different hashing logic.
        try:
            seed_bytes = struct.pack('d', active_stream_val) # 'd' = C double (8 bytes)
            seed_u64 = struct.unpack('Q', seed_bytes)[0] # 'Q' = C unsigned long long (8 bytes / uint64_t)
            fpsr_output = portable_rand_u64(seed_u64)
        except (struct.error, OverflowError):
             # Fallback in case of a highly unusual float value
            fpsr_output = portable_rand_u64(int(active_stream_val * 1e9))
    else:
        # If finalRandSwitch is false, return the active stream value directly.
        # The quantised streams `stream1` and `stream2` (and thus `active_stream_val`)
        # are already in the correct [0.0, 1.0] range.
        fpsr_output = active_stream_val
        
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

"""
------------------------------
FPS-R: Bitwise Decode (BD)
------------------------------
"""

def fpsr_bd(
    frame: int,
    block_size: int,
    streams_number: int = 1,
    streams_offset: int = 0,
    intra_op: str = "none",
    dynamic_shift_bits: int = 6,
    static_shift_amount: int = 1,
    inter_op: str = "xor",
    value_seed_offset: int = 0
):
    """
    Generates a phrased random value by decoding a deterministically generated bitstream.

    This algorithm is stateless. For any given frame, it calculates its state by:
    1. Finding the start of its macro-block (`outer_anchor`).
    2. Generating one or more raw bitstreams for the block.
    3. Applying transformations (intra-stream op) to each stream.
    4. Combining the transformed streams (inter-stream op).
    5. Decoding the final bitstream to produce phrased holds and jumps based on bit-flips.

    Args:
        frame (int): The current frame or time input.
        block_size (int): The size of the macro-rhythm in frames. Must be > 0.
        streams_number (int): The number of parallel bitstreams to generate.
        streams_offset (int): The frame offset between each parallel stream's seed.
        intra_op (str): The unary (intra-stream) operation.
                        Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
                        Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
        dynamic_shift_bits (int): For dynamic ops, the number of controller bits to determine shift amount.
        static_shift_amount (int): For static ops, the fixed number of bits to shift/rotate.
        inter_op (str): The binary (inter-stream) operation to combine streams ("xor", "or", "and").
        value_seed_offset (int): An additional seed offset for the final value calculation.
    Returns:
        float: A deterministic, phrased pseudo-random float between 0.0 and 1.0.
    """
    if block_size <= 0:
        block_size = 1
    if streams_number < 1:
        streams_number = 1

    # [PORT UPDATE] Match C's 'sanitized_static_shift'.
    # This ensures static shift amounts are always in the valid [0, 63] range
    # by masking, preventing undefined/inconsistent behavior for large shifts.
    sanitized_static_shift = static_shift_amount & (_CHUNK_BITS - 1)

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the raw bitstream(s) for the entire block ---
    num_chunks = (block_size + (_CHUNK_BITS - 1)) // _CHUNK_BITS
    raw_streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
        # [PORT UPDATE] Ensure seed components are int for uint64 emulation
        chunks = [_splitmix64(_to_uint64(int(stream_seed) + j)) for j in range(num_chunks)]
        raw_streams.append(chunks)

    # --- Step 3: Apply Intra-Stream Transformations ---
    transformed_streams = []
    unary_op = intra_op.lower()
    
    dynamic_ops = ["lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic"]
    is_dynamic = unary_op in dynamic_ops

    if is_dynamic:
        num_transformed_streams = (streams_number // 2) + (streams_number % 2)
        for i in range(0, streams_number // 2):
            data_stream = raw_streams[i * 2]
            controller_stream = raw_streams[i * 2 + 1]
            
            # [PORT UPDATE] Match C's logic for max_bits_for_shift
            # C: int max_bits_for_shift = 6; (for CHUNK_BITS=64)
            max_bits_for_shift = 6
            bit_mask_size = max(1, min(max_bits_for_shift, dynamic_shift_bits))
            bit_mask = (1 << bit_mask_size) - 1
            
            transformed_chunks = []
            for j in range(num_chunks):
                data_chunk = data_stream[j]
                controller_chunk = controller_stream[j]
                # [PORT UPDATE] Match C's logic: dynamic_shift is just the masked value
                dynamic_shift = (controller_chunk & bit_mask)
                # The modulo is applied during the shift/rotate call
                
                if unary_op == "lshift_dynamic":
                    # [PORT UPDATE] Apply modulo inside shift call to match C
                    transformed_chunks.append(_to_uint64(data_chunk << (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rshift_dynamic":
                    # [PORT UPDATE] Apply modulo inside shift call to match C
                    transformed_chunks.append(_to_uint64(data_chunk >> (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rotl_dynamic":
                    # [PORT UPDATE] Pass raw dynamic_shift to helper, which will modulo
                    transformed_chunks.append(_circular_left_shift(data_chunk, dynamic_shift))
                elif unary_op == "rotr_dynamic":
                    # [PORT UPDATE] Pass raw dynamic_shift to helper, which will modulo
                    transformed_chunks.append(_circular_right_shift(data_chunk, dynamic_shift))
            transformed_streams.append(transformed_chunks)
        
        # [PORT UPDATE] Match C's logic for handling odd number of streams
        if streams_number % 2 != 0:
            transformed_streams.append(raw_streams[-1]) # Copy last stream as-is

    else: # Apply static operations
        num_transformed_streams = streams_number
        for stream_chunks in raw_streams:
            if unary_op == "not":
                transformed_chunks = [_to_uint64(~chunk) for chunk in stream_chunks]
            elif unary_op == "lshift":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_to_uint64(chunk << sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rshift":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_to_uint64(chunk >> sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotl":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_circular_left_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotr":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_circular_right_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            else: # "none"
                transformed_chunks = list(stream_chunks) # "none", copy the stream
            transformed_streams.append(transformed_chunks)
        
    # --- Step 4: Combine Streams with Inter-Stream Operation ---
    # [PORT UPDATE] Match C's inter-op logic exactly.
    if num_transformed_streams > 0:
        # Start with a copy of the first transformed stream's chunks
        final_chunks = list(transformed_streams[0])
        
        op_map = { "xor": (lambda a, b: a ^ b), "or": (lambda a, b: a | b), "and": (lambda a, b: a & b) }
        chosen_op = op_map.get(inter_op.lower(), lambda a, b: a ^ b) # Default to xor

        # Loop from the *second* stream onwards
        for i in range(1, num_transformed_streams):
            for j in range(num_chunks):
                final_chunks[j] = chosen_op(final_chunks[j], transformed_streams[i][j])
                # Emulate uint64 wraparound
                final_chunks[j] = _to_uint64(final_chunks[j])
    else:
        # No streams, result is all zeros
        final_chunks = [0] * num_chunks


    # --- Step 5: Decode the final bitstream ---
    def get_bit(n):
        # Helper to get a specific bit, matching C's logic
        if not (0 <= n < block_size): return 0
        chunk_index, bit_index = n // _CHUNK_BITS, n % _CHUNK_BITS
        if chunk_index >= num_chunks: return 0
        return (final_chunks[chunk_index] >> bit_index) & 1

    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0
    
    # Scan backwards from current position to find the last bit-flip
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 6: Generate the final random value from the last bit-flip position ---
    # [PORT UPDATE] C logic relies on uint64 wraparound for the final seed addition.
    # We emulate this by summing the components as standard Python ints,
    # and portable_rand_u64 will handle the final _to_uint64 mask.
    final_seed = int(outer_anchor) + int(last_flip_pos) + int(value_seed_offset)
    
    return portable_rand_u64(final_seed)

# Sample code to call the FPS-R:BD function
# Parameters
frame = 100  # Current frame number
p_block_size = 64 # Size of the macro-rhythm in frames
p_streams_number = 2 # Number of parallel bitstreams to generate
p_streams_offset = 10 # Frame offset between each parallel stream's seed
# Intra-stream operation
# Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
# Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
p_intra_op = "rotl_dynamic" 
p_dynamic_shift_bits = 6 # For dynamic ops, number of controller bits to determine shift amount
p_static_shift_amount = 1 # For static ops, fixed number of bits to shift/rotate
p_inter_op = "xor" # Binary (inter-stream) operation to combine streams
p_value_seed_offset = 78901 # Additional seed offset for the final value calculation

# # Call the FPS-R:BD function
# randVal = fpsr_bd(
#     frame=frame,
#     block_size=p_block_size,
#     streams_number=p_streams_number,
#     streams_offset=p_streams_offset,
#     intra_op=p_intra_op,
#     dynamic_shift_bits=p_dynamic_shift_bits,
#     static_shift_amount=p_static_shift_amount,
#     inter_op=p_inter_op,
#     value_seed_offset=p_value_seed_offset
# )
# # Another call to fpsr_bd for the previous frame
# randVal_previous = fpsr_bd(
#     frame=frame - 1,
#     block_size=p_block_size,
#     streams_number=p_streams_number,
#     streams_offset=p_streams_offset,
#     intra_op=p_intra_op,
#     dynamic_shift_bits=p_dynamic_shift_bits,
#     static_shift_amount=p_static_shift_amount,
#     inter_op=p_inter_op,
#     value_seed_offset=p_value_seed_offset
# )
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Bitwise Decode (BD) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}')

# end of fpsr_bd sample


# /******************************************************************************/
# /* Main function to demonstrate usage of FPS-R algorithms                     */
# /******************************************************************************/
if __name__ == "__main__":
    # [PORT UPDATE] This entire test block has been updated to use the
    # exact same parameters and loop logic as the C 'main' function
    # for 1-to-1 comparison and verification of the port.
    
    # algorithms: 0 - sm, 1 - tm, 2 - qs, 3 - bd
    algo = 3  # Change this value to 0, 1, 2, or 3 to test different algorithms
    algo_name = ["SM", "TM", "QS", "BD"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103, 100]  # starting frames for each algorithm
    num_frames = 30  # run a loop of x frames to demonstrate changes
    
    # create main for loop to demonstrate changes
    for loop_frame in range(num_frames):
        # [PORT UPDATE] Use the same frame logic as C for ALL algorithms
        frame = loop_frame + start_frames[algo]  # starting frame for the selected algorithm
        randVal = 0.0  # variable to hold the random value output
        randVal_previous = 0.0  # variable to hold the previous frame's random value
        changed = 0  # Variable to track if the value has changed

        if algo == 0:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:SM function
            # [PORT UPDATE] Parameters now match the C main() function
            # --------------------------------------------------------------------------
            # Parameters
            minHoldFrames = 10  # probable minimum held period
            maxHoldFrames = 13  # maximum held period before cycling
            reseedFrames = 5    # inner mod cycle timing
            offsetInner = -34   # offsets the inner frame
            offsetOuter = 22    # offsets the outer frame
            finalRandSwitch = 1 # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:SM function        
            # call to fpsr_sm for the current frame
            randVal = float(fpsr_sm(
                int(frame), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            # another call to fpsr_sm for the previous frame
            randVal_previous = float(fpsr_sm(
                int(frame - 1), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        elif algo == 1:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:TM function
            # [PORT UPDATE] Parameters now match the C main() function
            # --------------------------------------------------------------------------
            # Parameters
            period_A = 10         # The first hold duration
            period_B = 16         # The second hold duration
            periodSwitch = 9      # The toggle happens every 30 frames
            offset_inner = 4      # offsets the inner (toggle) clock
            offset_outer = 0      # offsets the outer (hold) clock
            final_rand_switch = 1 # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:TM function
            # call to fpsr_tm for the current frame
            randVal = float(fpsr_tm(
                int(frame), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            # another call to fpsr_tm for the previous frame
            randVal_previous = float(fpsr_tm(
                int(frame - 1), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        elif algo == 2:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:QS function
            # [PORT UPDATE] Parameters now match the C main() function
            # --------------------------------------------------------------------------
            # Parameters
            baseWaveFreq = 0.012 # Base frequency for the modulation wave of stream 1
            stream2freqMult = 3.1 # Multiplier for the second stream's frequency
            quantLevelsMinMax = [4, 12] # Min, Max quantisation levels for the two streams
            streamsOffset = [0, 72] # Offset for the two streams
            quantOffsets = [9, 81] # Offset for the random quantisation selection
            streamSwitchDur = 11 # Duration for switching streams in frames
            stream1QuantDur = 13 # Duration for the first stream's quantisation switch cycle in frames
            stream2QuantDur = 20 # Duration for the second stream's quantisation switch cycle in frames
            finalRandSwitch = 1 # 1 to apply the final randomisation step, 0 to skip it
            
            # call to fpsr_qs for the current frame
            randVal = float(fpsr_qs(
                int(frame), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), bool(finalRandSwitch)))
            # another call to fpsr_qs for the previous frame
            randVal_previous = float(fpsr_qs(
                int(frame - 1), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), bool(finalRandSwitch)))
            changed = 0  # Variable to track if the value has changed
            if randVal != randVal_previous:
                changed = 1  # Mark as changed if the value has changed from the previous frame

        elif algo == 3:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:BD function
            # [PORT UPDATE] Parameters already matched C, no change needed here.
            # --------------------------------------------------------------------------
            # Parameters
            p_block_size = 64 # Size of the macro-rhythm in frames
            p_streams_number = 2 # Number of parallel bitstreams to generate
            p_streams_offset = 10 # Frame offset between each parallel stream's seed
            # Intra-stream operation
            # Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
            # Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
            p_intra_op = "rotl_dynamic"
            p_dynamic_shift_bits = 6 # For dynamic ops, number of controller bits to determine shift amount
            p_static_shift_amount = 1 # For static ops, fixed number of bits to shift/rotate
            p_inter_op = "xor" # Binary (inter-stream) operation to combine streams
            p_value_seed_offset = 78901 # Additional seed offset for the final value calculation

            randVal = fpsr_bd(
                frame=frame, block_size=p_block_size, streams_number=p_streams_number,
                streams_offset=p_streams_offset, intra_op=p_intra_op,
                dynamic_shift_bits=p_dynamic_shift_bits, static_shift_amount=p_static_shift_amount,
                inter_op=p_inter_op, value_seed_offset=p_value_seed_offset)
            
            randVal_previous = fpsr_bd(
                frame=frame - 1, block_size=p_block_size, streams_number=p_streams_number,
                streams_offset=p_streams_offset, intra_op=p_intra_op,
                dynamic_shift_bits=p_dynamic_shift_bits, static_shift_amount=p_static_shift_amount,
                inter_op=p_inter_op, value_seed_offset=p_value_seed_offset)

            if randVal != randVal_previous: changed = 1
        
        # [PORT UPDATE] Mirror C printf formatting (%.6f) and two-step print
        print(f"Frame {frame}: randVal {randVal:.6f}, randVal_previous {randVal_previous:.6f}, changed {changed} ", end="")
        print("(jumped)" if changed else "")
