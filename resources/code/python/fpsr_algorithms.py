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
    shift %= _CHUNK_BITS
    return _to_uint64((value << shift) | (value >> (_CHUNK_BITS - shift)))

def _circular_right_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular right shift (rotate right)."""
    shift %= _CHUNK_BITS
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

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the raw bitstream(s) for the entire block ---
    num_chunks = (block_size + (_CHUNK_BITS - 1)) // _CHUNK_BITS
    raw_streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
        chunks = [_splitmix64(_to_uint64(stream_seed + j)) for j in range(num_chunks)]
        raw_streams.append(chunks)

    # --- Step 3: Apply Intra-Stream Transformations ---
    transformed_streams = []
    unary_op = intra_op.lower()
    
    dynamic_ops = ["lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic"]

    if unary_op in dynamic_ops:
        for i in range(0, streams_number // 2):
            data_stream = raw_streams[i * 2]
            controller_stream = raw_streams[i * 2 + 1]
            
            max_bits_for_shift = max(1, math.ceil(math.log2(_CHUNK_BITS)) if _CHUNK_BITS > 1 else 1)
            bit_mask_size = max(1, min(max_bits_for_shift, dynamic_shift_bits))
            bit_mask = (1 << bit_mask_size) - 1
            
            transformed_chunks = []
            for j in range(num_chunks):
                data_chunk = data_stream[j]
                controller_chunk = controller_stream[j]
                dynamic_shift = (controller_chunk & bit_mask) % _CHUNK_BITS
                
                if unary_op == "lshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk << dynamic_shift))
                elif unary_op == "rshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk >> dynamic_shift))
                elif unary_op == "rotl_dynamic":
                    transformed_chunks.append(_circular_left_shift(data_chunk, dynamic_shift))
                elif unary_op == "rotr_dynamic":
                    transformed_chunks.append(_circular_right_shift(data_chunk, dynamic_shift))
            transformed_streams.append(transformed_chunks)
        
        if streams_number % 2 != 0:
            transformed_streams.append(raw_streams[-1])

    else: # Apply static operations
        for stream_chunks in raw_streams:
            if unary_op == "not":
                transformed_chunks = [_to_uint64(~chunk) for chunk in stream_chunks]
            elif unary_op == "lshift":
                transformed_chunks = [_to_uint64(chunk << static_shift_amount) for chunk in stream_chunks]
            elif unary_op == "rshift":
                transformed_chunks = [_to_uint64(chunk >> static_shift_amount) for chunk in stream_chunks]
            elif unary_op == "rotl":
                transformed_chunks = [_circular_left_shift(chunk, static_shift_amount) for chunk in stream_chunks]
            elif unary_op == "rotr":
                transformed_chunks = [_circular_right_shift(chunk, static_shift_amount) for chunk in stream_chunks]
            else: # "none"
                transformed_chunks = stream_chunks
            transformed_streams.append(transformed_chunks)
        
    # --- Step 4: Combine Streams with Inter-Stream Operation ---
    if len(transformed_streams) > 1:
        op_map = { "xor": (lambda a, b: a ^ b), "or": (lambda a, b: a | b), "and": (lambda a, b: a & b) }
        chosen_op = op_map.get(inter_op.lower(), lambda a, b: a ^ b)
        
        combined_chunks = []
        for chunk_idx in range(num_chunks):
            chunks_to_combine = [stream[chunk_idx] for stream in transformed_streams]
            combined_chunk = functools.reduce(chosen_op, chunks_to_combine)
            combined_chunks.append(combined_chunk)
        final_chunks = combined_chunks
    elif transformed_streams:
        final_chunks = transformed_streams[0]
    else:
        final_chunks = [0] * num_chunks

    # --- Step 5: Decode the final bitstream ---
    def get_bit(n):
        if not (0 <= n < block_size): return 0
        chunk_index, bit_index = n // _CHUNK_BITS, n % _CHUNK_BITS
        return (final_chunks[chunk_index] >> bit_index) & 1

    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0
    
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 6: Generate the final random value from the last bit-flip position ---
    final_seed = _to_uint64(outer_anchor) + _to_uint64(last_flip_pos) + _to_uint64(value_seed_offset)
    
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
    # algorithms: 0 - sm, 1 - tm, 2 - qs, 3 - bd
    algo = 3  # Change this value to 0, 1, 2, or 3 to test different algorithms
    algo_name = ["SM", "TM", "QS", "BD"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103, 100]  # starting frames for each algorithm
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

        elif algo == 3:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:BD function
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
        
        # Mirror C printf formatting and two-step print for the suffix
        print(f"Frame {frame}: randVal {randVal:.6f}, randVal_previous {randVal_previous:.6f}, changed {changed} ", end="")
        print("(jumped)" if changed else "")