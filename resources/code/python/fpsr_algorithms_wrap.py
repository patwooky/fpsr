# SPDX-License-Identifier: MIT — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

'''
file: fpsr_algorithms_wrap.py
brief: Python port of the wrapper-based approach for getting rich metadata
    from the core FPS-R algorithms.
details: 
    This implementation contains the pure, stateless algorithms and wrapper
    functions that perform a robust, two-phase search (exponential probe +
    binary search) to populate the FPSR_Output struct.
    This version includes the "Hierarchical Phrased Quantisation" (HPQ) wrapper
    logic, which implements a "stretch-and-generate" model for time scaling.
'''

import math
import functools
import threading
import struct # Required for bit-for-bit float/int casting (replaces C memcpy)

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
    # C-version passes a uint64_t. We emulate this by masking
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
    # Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value << shift) | (value >> (_CHUNK_BITS - shift)))

def _circular_right_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular right shift (rotate right)."""
    # Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value >> shift) | (value << (_CHUNK_BITS - shift)))

# Helper to get a specific bit from a chunk array
def _get_bit(n: int, block_size: int, chunks: list, num_chunks: int) -> int:
    """
    Helper to get a specific bit from a chunk array, matching C's out-of-bounds logic
    """
    if not (0 <= n < block_size): return 0
    chunk_index = n // _CHUNK_BITS
    bit_index = n % _CHUNK_BITS
    if chunk_index >= num_chunks: return 0
    return (chunks[chunk_index] >> bit_index) & 1

# -----------------------------------------------------------------------------
# Sine Lookup Table (LUT) Implementation
# -----------------------------------------------------------------------------
# This logic mirrors the C reference's thread-safe, one-time initialization
# of sine lookup tables for use in fpsr_qs.

_SINE_LUT_SIZE_100 = 100
_SINE_LUT_SIZE_500 = 500
_SINE_LUT_SIZE_1000 = 1000
_SINE_LUT_SIZE_4096 = 4096

_TWO_PI = 6.28318530718

# Global sine lookup tables (initialized as empty lists)
_sine_lut_100 = []
_sine_lut_500 = []
_sine_lut_1000 = []
_sine_lut_4096 = []

# Thread-safe initialization control (Python equivalent of C's 'init_once')
_sine_luts_initialized = False
_sine_luts_lock = threading.Lock()

def initialize_sine_luts():
    """
    Initializes all global sine lookup tables.
    This function is called exactly once by _init_once_func.
    """
    global _sine_lut_100, _sine_lut_500, _sine_lut_1000, _sine_lut_4096
    
    _sine_lut_100 = [math.sin(i / _SINE_LUT_SIZE_100 * _TWO_PI) for i in range(_SINE_LUT_SIZE_100)]
    _sine_lut_500 = [math.sin(i / _SINE_LUT_SIZE_500 * _TWO_PI) for i in range(_SINE_LUT_SIZE_500)]
    _sine_lut_1000 = [math.sin(i / _SINE_LUT_SIZE_1000 * _TWO_PI) for i in range(_SINE_LUT_SIZE_1000)]
    _sine_lut_4096 = [math.sin(i / _SINE_LUT_SIZE_4096 * _TWO_PI) for i in range(_SINE_LUT_SIZE_4096)]

def _init_once_func():
    """
    Ensures initialize_sine_luts() is called exactly once, in a thread-safe manner.
    """
    global _sine_luts_initialized
    # Double-checked locking pattern for efficiency
    if not _sine_luts_initialized:
        with _sine_luts_lock:
            if not _sine_luts_initialized:
                initialize_sine_luts()
                _sine_luts_initialized = True

def _get_sine_from_lod_lut(phase: float, lut_size: int, lut_array: list) -> float:
    """
    Gets a sine value from a specific LUT with linear interpolation.
    Matches the C reference implementation.
    """
    # 1. Guaranteed thread-safe call
    _init_once_func()

    # 2. Interpolation logic
    # Wrap phase to 0 to 2*PI range
    phase = phase % _TWO_PI
    if phase < 0:
        phase += _TWO_PI  # Ensure positive

    # Map phase to LUT index range
    fractional_index = (phase / _TWO_PI) * lut_size
    
    # Get integer part and fractional part
    index1 = math.floor(fractional_index)
    frac = fractional_index - index1
    
    # Handle wrap-around for index2 (last point wraps to first)
    index1 = int(index1)
    if index1 >= lut_size:
        index1 = 0
    index2 = (index1 + 1) % lut_size

    # Linear interpolation
    return lut_array[index1] * (1.0 - frac) + lut_array[index2] * frac

# -----------------------------------------------------------------------------

# [FIX]: Converting C-style comments to Python
# -----------------------------------------------------------------------------
# FPS-R Output Structure
# -----------------------------------------------------------------------------
# Python class equivalent of the C FPSR_Output struct
class FPSR_Output:
    """
    This class holds the output of the FPS-R algorithms.
    The LOD (Level of Detail) determines the computational overhead and the
    amount of information returned.

    [PORT]: Ported from C FPSR_Output struct documentation.
    
    Different LODs will return different sets of fields:
    - LOD 0: randVal
    - LOD 1: randVal, has_changed
    - LOD 2: randVal, has_changed, hold_progress, last_changed_frame, next_changed_frame,
             randVal_next_changed_frame, randStreams[2], selected_stream (for QS algorithm)
    Note: All fields will be set to 0 if the LOD is not applicable.
    
    Fields:
    - randVal (float): LOD 0, 1, 2. The random value.
    - has_changed (int): LOD 1, 2. 1 if randVal changed from prev frame, else 0.
    - randVal_previous (float): LOD 1, 2. The random value from the previous frame.
    - hold_progress (float): LOD 2. Normalized progress of the hold [0, 1].
    - last_changed_frame (int): LOD 2. The frame when randVal last changed.
    - next_changed_frame (int): LOD 2. The frame when randVal will next change.
    - randVal_next_changed_frame (float): LOD 2. The value at next_changed_frame.
    - randStreams (list[float]): LOD 2. (QS only) Raw values of stream1 and stream2.
    - selected_stream_idx (int): LOD 2. (QS only) 0 for stream1, 1 for stream2.
    """
    def __init__(self):
        self.randVal = 0.0
        self.has_changed = 0
        self.randVal_previous = 0.0
        self.hold_progress = 0.0
        self.last_changed_frame = 0
        self.next_changed_frame = 0
        self.randVal_next_changed_frame = 0.0
        # QS-specific fields
        self.randStreams = [0.0, 0.0]
        self.selected_stream_idx = 0

# [FIX]: Converting C-style comments to Python
# -----------------------------------------------------------------------------
# Pure, Canonical FPS-R Algorithms
# -----------------------------------------------------------------------------
# These functions are the pure, canonical reference implementations. They operate
# on a 64-bit integer timeline for absolute determinism.

# --------------------------
# FPS-R: Stacked Modulo (SM)
# --------------------------
def fpsr_sm_base(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch=True):
    """
    (Pure implementation)
    Generates a persistent random value that holds for a calculated duration.
    This function uses a two-step process. First, it determines a random
    "hold duration". Second, it generates a stable integer for that duration,
    which is then used as a seed to produce the final, held random value.

    [PORT]: Ported from C docstring for fpsr_sm_base.

    Args:
        frame (int): The current frame or time input.
        minHold (int): The minimum duration (in frames) for a value to hold.
        maxHold (int): The maximum duration (in frames) for a value to hold.
        reseedInterval (int): The fixed interval at which a new hold duration is calculated.
        seedInner (int): An offset for the random duration calculation to create unique sequences.
        seedOuter (int): An offset for the final value calculation to create unique sequences.
        finalRandSwitch (bool): A flag that can turn off the final randomisation step.

    Returns:
        float: 
        when finalRandSwitch is 0: 
            randVal will be a whole number representing the currently held frame 
            that remains constant for the hold duration.
        when finalRandSwitch is 1: 
            A float value between 0.0 and 1.0 that remains constant 
            for the held duration.
    """
    # --- 1. Calculate the random hold duration ---
    if reseedInterval < 1:
        reseedInterval = 1  # Prevent division by zero.

    # Use floor-based modulo to match Python for negative frames.
    reseed_anchor = int(seedInner + frame) - i64_floor_mod(int(frame), int(reseedInterval))
    
    # Deterministic PRNG over 64-bit integer seed; result is double in [0,1].
    rand_for_duration = portable_rand_u64(reseed_anchor)
    
    # Compute duration with double intermediates then floor to int64.
    holdDuration = math.floor(float(minHold) + rand_for_duration * float(maxHold - minHold))

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # Align down using floor-mod semantics for negative inputs.
    held_integer_state = i64_align_down(int(seedOuter + frame), int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # The held_integer_state is the unique identifier.
        # Pass it directly to the SplitMix64 hasher.
        fpsr_output = portable_rand_u64(held_integer_state)
    else:
        # Return the raw integer state as a float (matches C's cast).
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# ---------------------------
# FPS-R: Toggled Modulo (TM)
# ---------------------------
def fpsr_tm_base(frame, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch=True):
    """
    (Pure implementation)
    Generates a persistent value that holds for a rhythmically toggled duration.
    This function uses a deterministic switch to toggle the hold duration
    between two fixed periods. This creates a predictable, rhythmic, or mechanical
    "move-and-hold" pattern, as opposed to the organic randomness of SM.

    [PORT]: Ported from C docstring for fpsr_tm_base.

    Args:
        frame (int): The current frame or time input.
        periodA (int): The first hold duration (in frames).
        periodB (int): The second hold duration (in frames).
        periodSwitch (int): The fixed interval at which the hold duration is toggled.
        seedInner (int): An offset for the toggle clock to de-sync it from the main clock.
        seedOuter (int): An offset for the main clock to create unique output sequences.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: 
        when finalRandSwitch is 0: 
            An integer value representing the currently held frame state.
        when finalRandSwitch is 1: 
            A float value between 0.0 and 1.0 that holds for the toggled duration.
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
    # The "outer clock" is offset by seedOuter to create unique output sequences.
    outer_clock_frame = int(seedOuter + frame)
    held_integer_state = i64_align_down(outer_clock_frame, int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # The held_integer_state is the unique identifier.
        # Pass it directly to the SplitMix64 hasher.
        fpsr_output = portable_rand_u64(held_integer_state)
    else:
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# -------------------------------
# FPS-R: Quantised Switching (QS)
# -------------------------------
def fpsr_qs_base(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets,
                 streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch=True, sine_lod_level: int = 4):
    """
    (Pure implementation for wrapper)
    Generates a quantized sine-based persistent random value using two streams.
    This function creates two sine wave streams with configurable frequencies
    and offsets. For each stream, a new random quantisation level is chosen 
    from within the [min, max] range at a set interval, and the output alternates
    between the two streams based on a defined switch duration. The final output can
    optionally be further randomized.

    [PORT]: Ported from C docstring for fpsr_qs_base.

    Args:
        frame (int): The current frame or time input.
        baseWaveFreq (float): The base frequency for the sine waves.
        stream2FreqMult (float): A multiplier for the second stream's frequency.
        quantLevelsMinMax (list[int]): A list [min, max] quantization levels.
        streamsOffset (list[int]): A list [offset1, offset2] for each sine stream.
        quantOffsets (list[int]): A list [q_offset1, q_offset2] for each stream.
        streamSwitchDur (int): The duration (in frames) before switching between streams.
        stream1QuantDur (int): The quantization duration (in frames) for stream 1.
        stream2QuantDur (int): The quantization duration (in frames) for stream 2.
        finalRandSwitch (bool): A flag that can turn off the final randomisation step.
        sine_lod_level (int): Level of detail for sine calculation (0=direct, 1-4=LUTs).

    Returns:
        FPSR_Output: 
        A populated FPSR_Output object containing the randVal, randStreams, 
        and selected_stream_idx. Other LOD fields are not populated by this base function.
    """
    output = FPSR_Output()

    if streamSwitchDur < 1: streamSwitchDur = 1
    if stream1QuantDur < 1: stream1QuantDur = 1
    if stream2QuantDur < 1: stream2QuantDur = 1

    # --- 2. Calculate random quantisation levels for each stream ---
    quant_min = int(quantLevelsMinMax[0])
    quant_max = int(quantLevelsMinMax[1])
    quant_range = quant_max - quant_min + 1
    if quant_range < 1: quant_range = 1

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
    if stream2FreqMult <= 0: stream2FreqMult = 3.7

    angle1 = (float(streamsOffset[0]) + float(frame)) * float(baseWaveFreq)
    angle2 = (float(streamsOffset[1]) + float(frame)) * float(baseWaveFreq) * float(stream2FreqMult)
    
    stream1_raw_sine = 0.0
    stream2_raw_sine = 0.0
    
    if sine_lod_level == 0:
        stream1_raw_sine = math.sin(angle1)
        stream2_raw_sine = math.sin(angle2)
    elif sine_lod_level == 1:
        stream1_raw_sine = _get_sine_from_lod_lut(angle1, _SINE_LUT_SIZE_100, _sine_lut_100)
        stream2_raw_sine = _get_sine_from_lod_lut(angle2, _SINE_LUT_SIZE_100, _sine_lut_100)
    elif sine_lod_level == 2:
        stream1_raw_sine = _get_sine_from_lod_lut(angle1, _SINE_LUT_SIZE_500, _sine_lut_500)
        stream2_raw_sine = _get_sine_from_lod_lut(angle2, _SINE_LUT_SIZE_500, _sine_lut_500)
    elif sine_lod_level == 3:
        stream1_raw_sine = _get_sine_from_lod_lut(angle1, _SINE_LUT_SIZE_1000, _sine_lut_1000)
        stream2_raw_sine = _get_sine_from_lod_lut(angle2, _SINE_LUT_SIZE_1000, _sine_lut_1000)
    else: # Default to 4
        stream1_raw_sine = _get_sine_from_lod_lut(angle1, _SINE_LUT_SIZE_4096, _sine_lut_4096)
        stream2_raw_sine = _get_sine_from_lod_lut(angle2, _SINE_LUT_SIZE_4096, _sine_lut_4096)

    # Map sine from [-1,1] to [0,1] before quantizing
    output.randStreams[0] = math.floor((stream1_raw_sine * 0.5 + 0.5) * float(s1_quant_level)) / float(s1_quant_level)
    output.randStreams[1] = math.floor((stream2_raw_sine * 0.5 + 0.5) * float(s2_quant_level)) / float(s2_quant_level)

    # --- 4. Switch between the two streams ---
    r = i64_floor_mod(int(frame), streamSwitchDur)
    output.selected_stream_idx = 0 if (2 * r) < streamSwitchDur else 1
    active_stream_val = output.randStreams[output.selected_stream_idx]

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        # Match the canonical C implementation's 'FIX'.
        # Scale the quantized value to preserve level information for hashing.
        hashed_int = math.floor(active_stream_val * 1000000.0)
        output.randVal = portable_rand_u64(hashed_int)
    else:
        # Match the canonical C implementation's 'FIX'.
        # Return the active_stream_val directly, which is already in the [0, 1] range.
        output.randVal = active_stream_val
        
    return output

# ------------------------------
# FPS-R: Bitwise Decode (BD)
# ------------------------------
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
    (Pure implementation)
    Generates a phrased random value by decoding a deterministically generated bitstream.
    
    [PORT]: Ported from C docstring for fpsr_bd.

    Args:
        frame (int): The current frame or time input.
        block_size (int): The size of the macro-rhythm in frames. Must be > 0.
        streams_number (int): The number of parallel bitstreams to generate.
        streams_offset (int): The frame offset between each parallel stream's seed.
        intra_op (str): The unary (intra-stream) operation.
            Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
            Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
        dynamic_shift_bits (int): For dynamic ops, the number of controller bits to read
            to determine the shift/rotate amount (1-6 when chunk_bits=64).
        static_shift_amount (int): For static ops, the fixed number of bits to shift/rotate.
        inter_op (str): The binary (inter-stream) operation to combine multiple
            transformed streams. Options: "xor", "or", "and".
        value_seed_offset (int): An additional seed offset for the final value calculation.

    Returns:
        float: A deterministic, phrased pseudo-random double between 0.0 and 1.0.
    """
    if block_size <= 0:
        block_size = 1
    if streams_number < 1:
        streams_number = 1

    # Sanitize static_shift_amount to prevent Undefined Behavior
    sanitized_static_shift = static_shift_amount & (_CHUNK_BITS - 1)

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the raw bitstream(s) for the entire block ---
    num_chunks = (block_size + (_CHUNK_BITS - 1)) // _CHUNK_BITS
    raw_streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
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
            
            max_bits_for_shift = 6 # ceil(log2(64))
            bit_mask_size = max(1, min(max_bits_for_shift, dynamic_shift_bits))
            bit_mask = (1 << bit_mask_size) - 1
            
            transformed_chunks = []
            for j in range(num_chunks):
                data_chunk = data_stream[j]
                controller_chunk = controller_stream[j]
                dynamic_shift = (controller_chunk & bit_mask)
                
                if unary_op == "lshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk << (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk >> (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rotl_dynamic":
                    transformed_chunks.append(_circular_left_shift(data_chunk, dynamic_shift))
                elif unary_op == "rotr_dynamic":
                    transformed_chunks.append(_circular_right_shift(data_chunk, dynamic_shift))
            transformed_streams.append(transformed_chunks)
        
        if streams_number % 2 != 0:
            transformed_streams.append(raw_streams[-1]) # Copy last stream as-is

    else: # Apply static operations
        num_transformed_streams = streams_number
        for stream_chunks in raw_streams:
            if unary_op == "not":
                transformed_chunks = [_to_uint64(~chunk) for chunk in stream_chunks]
            elif unary_op == "lshift":
                transformed_chunks = [_to_uint64(chunk << sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rshift":
                transformed_chunks = [_to_uint64(chunk >> sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotl":
                transformed_chunks = [_circular_left_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotr":
                transformed_chunks = [_circular_right_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            else: # "none"
                transformed_chunks = list(stream_chunks) # "none", copy the stream
            transformed_streams.append(transformed_chunks)
        
    # --- Step 4: Combine Streams with Inter-Stream Operation ---
    if num_transformed_streams > 0:
        final_chunks = list(transformed_streams[0])
        op_map = { "xor": (lambda a, b: a ^ b), "or": (lambda a, b: a | b), "and": (lambda a, b: a & b) }
        chosen_op = op_map.get(inter_op.lower(), lambda a, b: a ^ b) # Default to xor

        for i in range(1, num_transformed_streams):
            for j in range(num_chunks):
                final_chunks[j] = _to_uint64(chosen_op(final_chunks[j], transformed_streams[i][j]))
    else:
        final_chunks = [0] * num_chunks


    # --- Step 5: Decode the final bitstream ---
    # We define get_bit inside fpsr_bd so it has access to
    # final_chunks, num_chunks, and block_size from its closure.
    def get_bit(n):
        # Helper to get a specific bit, matching C's logic
        if not (0 <= n < block_size): return 0
        chunk_index = n // _CHUNK_BITS
        bit_index = n % _CHUNK_BITS
        if chunk_index >= num_chunks: return 0
        return (final_chunks[chunk_index] >> bit_index) & 1

    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0
    
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 6: Generate the final random value from the last bit-flip position ---
    final_seed = int(outer_anchor) + int(last_flip_pos) + int(value_seed_offset)
    
    return portable_rand_u64(final_seed)


# [FIX]: Converting C-style comments to Python
# -----------------------------------------------------------------------------
# High-Level Wrapper Functions with Hierarchical Time
# -----------------------------------------------------------------------------

# NOTE: The 'p_scaled_frame_pos_out' pointer parameter from C is omitted
# in the Python port as it was NULL in the C main() and Python handles
# returns differently.

def fpsr_sm_get_details(
    frame: int, frame_multiplier: float,
    minHold: int, maxHold: int,
    reseedInterval: int, seedInner: int, seedOuter: int, finalRandSwitch: bool,
    lod: int, max_search_frames: int,
    seg_block_length: int
) -> FPSR_Output:
    """
    ---- SM: Stacked Modulo Wrapper with Details ----
    Wrapper for fpsr_sm that returns a detailed FPSR_Output struct.

    [PORT]: Ported from C docstring for fpsr_sm_get_details.

    Args:
        frame (int): The current frame or time input.
        frame_multiplier (float): The time scaling factor.
            < 1.0 = Slow-Motion (Time Stretch)
            = 1.0 = Normal Speed
            > 1.0 = Fast-Motion (Time Compression)
        minHold (int): Algorithm parameter.
        maxHold (int): Algorithm parameter.
        reseedInterval (int): Algorithm parameter.
        seedInner (int): Algorithm parameter.
        seedOuter (int): Algorithm parameter.
        finalRandSwitch (bool): Algorithm parameter.
        lod (int): The level of detail to calculate.
        max_search_frames (int): A safety limit for the backward/forward search.
        seg_block_length (int): The "runway" length for HPQ logic.

    Returns:
        FPSR_Output: A struct with metadata populated based on the LOD.
    """
    out = FPSR_Output()
    
    # --- HPQ Timeline Definitions ---
    # 1. "Application Timeline": The user's `frame` (e.g., 0, 1, 2...).
    # 2. "Content Timeline": The *original* algorithm's timeline.
    # `frame_multiplier` (fm) maps between them:
    # (Application Timeline Frame) * fm = (Content Timeline Frame)
    
    # Sanitize frame_multiplier (now "playback_speed")
    fm = 1.0 if frame_multiplier == 0.0 else frame_multiplier

    # --- (START) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    # --- 1. Find coordinate on "Content Timeline" ---
    # [FIX]: C-style comment converted to Python
    # This calculation now matches the intuitive "playback_speed" convention.
    scaled_frame_position = float(frame) * fm
    master_frame = math.floor(scaled_frame_position)

    # --- 2. Find "Start Line" on "Application Timeline" ---
    # This finds the *first* application frame that maps to this master_frame.
    master_frame_start_app_frame = math.ceil(float(master_frame) / fm)

    # --- 3. Calculate Local Coordinates (all on "Application Timeline") ---
    # How many application frames has it been since this master_frame began?
    app_frames_into_gap = frame - master_frame_start_app_frame
    segment_index = 0
    local_progress_in_segment = 0

    if seg_block_length > 0:
        # Note: Python's // and % handle negatives with floor semantics,
        # which matches the C `i64_floor_mod` and `i64_align_down` logic.
        segment_index = app_frames_into_gap // seg_block_length
        local_progress_in_segment = app_frames_into_gap % seg_block_length
    
    # --- 4. Execute Two-Mode Logic ---
    if segment_index == 0:
        # --- MODE 1: "Tape Varispeed" (Anchor) ---
        # Repeat the value of the `master_frame` from the Content Timeline.
        out.randVal = float(fpsr_sm_base(master_frame, int(minHold), int(maxHold), int(reseedInterval), int(seedInner), int(seedOuter), finalRandSwitch))
    else:
        # --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        gap_seed = _splitmix64(_to_uint64(int(master_frame) + int(segment_index)))
        
        # Call using `local_progress_in_segment` (from Application Timeline)
        # and inject the unique `gap_seed` as 'seedInner'.
        out.randVal = float(fpsr_sm_base(local_progress_in_segment, int(minHold), int(maxHold), int(reseedInterval), int(gap_seed), int(seedOuter), finalRandSwitch))
    # --- (END) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if lod < 1: return out

    # LOD 1: Compare with previous frame to check for change.
    # This call is on the "Application Timeline".
    prev_out = fpsr_sm_get_details(frame - 1, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length)
    out.randVal_previous = prev_out.randVal 
    out.has_changed = 1 if (out.randVal != prev_out.randVal) else 0

    if lod < 2: return out

    # --- LOD 2: MODIFIED Robust Two-Phase Search ---
    # The search logic operates entirely on the "Application Timeline".
    next_val_candidate = 0.0
    step_int = 1

    # --- Backwards Search for last_changed_frame (on Application Timeline) ---
    if out.has_changed:
        out.last_changed_frame = int(frame)
    else:
        # Exponential probe backwards
        bound_low_int = frame
        step_int = 1
        while (frame - step_int > frame - max_search_frames): 
            val_at_probe = fpsr_sm_get_details(frame - step_int, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
            if (val_at_probe != out.randVal):
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2
        
        # Binary search
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1
        while(low_int <= high_int):
            mid_int = low_int + (high_int - low_int) // 2
            mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
            if (mid_val == out.randVal):
                prev_mid_val = fpsr_sm_get_details(mid_int - 1, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
                if (prev_mid_val != out.randVal):
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = int(result_int)

    # --- Forwards Search for next_changed_frame (on Application Timeline) ---
    # Exponential probe forwards
    bound_high_int = frame
    step_int = 1
    while (frame + step_int < frame + max_search_frames): 
        val_at_probe = fpsr_sm_get_details(frame + step_int, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
        if (val_at_probe != out.randVal):
            bound_high_int = frame + step_int
            next_val_candidate = val_at_probe
            break
        bound_high_int = frame + step_int
        step_int *= 2
    
    # Binary search
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames
    while(low_int <= high_int):
        mid_int = low_int + (high_int - low_int) // 2
        mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
        if (mid_val != out.randVal):
            result_int = mid_int
            next_val_candidate = mid_val
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = int(result_int)
    out.randVal_next_changed_frame = float(next_val_candidate)
    
    # --- (START) REPLACEMENT: UPDATED hold_progress Calculation ---
    # This calculation is now performed *purely* on the "Application Timeline"
    hold_duration_app_frames = float(out.next_changed_frame) - float(out.last_changed_frame)
    if (hold_duration_app_frames > 0.0):
        out.hold_progress = float((float(frame) - float(out.last_changed_frame)) / hold_duration_app_frames)
    else:
        out.hold_progress = 0.0
    # --- (END) REPLACEMENT: UPDATED hold_progress Calculation ---
    
    return out


def fpsr_tm_get_details(
    frame: int, frame_multiplier: float,
    periodA: int, periodB: int,
    periodSwitch: int, seedInner: int, seedOuter: int, finalRandSwitch: bool,
    lod: int, max_search_frames: int,
    seg_block_length: int
) -> FPSR_Output:
    """
    ---- TM: Toggle Modulo Wrapper with Details ----
    Wrapper for fpsr_tm that returns a detailed FPSR_Output struct.

    [PORT]: Ported from C docstring for fpsr_tm_get_details.

    Args:
        frame (int): The current frame or time input.
        frame_multiplier (float): The time scaling factor.
            < 1.0 = Slow-Motion (Time Stretch)
            = 1.0 = Normal Speed
            > 1.0 = Fast-Motion (Time Compression)
        periodA (int): Algorithm parameter.
        periodB (int): Algorithm parameter.
        periodSwitch (int): Algorithm parameter.
        seedInner (int): Algorithm parameter.
        seedOuter (int): Algorithm parameter.
        finalRandSwitch (bool): Algorithm parameter.
        lod (int): The level of detail to calculate.
        max_search_frames (int): A safety limit for the backward/forward search.
        seg_block_length (int): The "runway" length for HPQ logic.

    Returns:
        FPSR_Output: A struct with metadata populated based on the LOD.
    """
    out = FPSR_Output()
    
    # Sanitize frame_multiplier
    fm = 1.0 if frame_multiplier == 0.0 else frame_multiplier

    # --- (START) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    scaled_frame_position = float(frame) * fm
    master_frame = math.floor(scaled_frame_position)
    master_frame_start_app_frame = math.ceil(float(master_frame) / fm)

    app_frames_into_gap = frame - master_frame_start_app_frame
    segment_index = 0
    local_progress_in_segment = 0
    
    if seg_block_length > 0:
        segment_index = app_frames_into_gap // seg_block_length
        local_progress_in_segment = app_frames_into_gap % seg_block_length

    if segment_index == 0:
        # --- MODE 1: "Tape Varispeed" (Anchor) ---
        out.randVal = float(fpsr_tm_base(master_frame, int(periodA), int(periodB), int(periodSwitch), int(seedInner), int(seedOuter), finalRandSwitch))
    else:
        # --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        gap_seed = _splitmix64(_to_uint64(int(master_frame) + int(segment_index)))
        # Inject the unique `gap_seed` as 'seedInner'.
        out.randVal = float(fpsr_tm_base(local_progress_in_segment, int(periodA), int(periodB), int(periodSwitch), int(gap_seed), int(seedOuter), finalRandSwitch))
    # --- (END) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if lod < 1: return out

    # LOD 1
    prev_out = fpsr_tm_get_details(frame - 1, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length)
    out.randVal_previous = prev_out.randVal 
    out.has_changed = 1 if (out.randVal != prev_out.randVal) else 0
    
    if lod < 2: return out

    # --- LOD 2: MODIFIED Robust Search (on Application Timeline) ---
    next_val_candidate = 0.0
    step_int = 1

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = int(frame)
    else:
        bound_low_int = frame
        step_int = 1
        while (frame - step_int > frame - max_search_frames):
            val_at_probe = fpsr_tm_get_details(frame - step_int, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
            if (val_at_probe != out.randVal):
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2
        
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1
        while(low_int <= high_int):
            mid_int = low_int + (high_int - low_int) // 2
            mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
            if (mid_val == out.randVal):
                prev_mid_val = fpsr_tm_get_details(mid_int - 1, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
                if (prev_mid_val != out.randVal):
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = int(result_int)

    # --- Forwards search ---
    bound_high_int = frame
    step_int = 1
    while (frame + step_int < frame + max_search_frames):
        val_at_probe = fpsr_tm_get_details(frame + step_int, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
        if (val_at_probe != out.randVal):
            bound_high_int = frame + step_int
            next_val_candidate = val_at_probe
            break
        bound_high_int = frame + step_int
        step_int *= 2
    
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames
    while(low_int <= high_int):
        mid_int = low_int + (high_int - low_int) // 2
        mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length).randVal
        if (mid_val != out.randVal):
            result_int = mid_int
            next_val_candidate = mid_val
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = int(result_int)
    out.randVal_next_changed_frame = float(next_val_candidate)
    
    # --- UPDATED hold_progress Calculation ---
    hold_duration_app_frames = float(out.next_changed_frame) - float(out.last_changed_frame)
    if (hold_duration_app_frames > 0.0):
        out.hold_progress = float((float(frame) - float(out.last_changed_frame)) / hold_duration_app_frames)
    else:
        out.hold_progress = 0.0

    return out


def fpsr_qs_get_details(
    frame: int, frame_multiplier: float,
    baseWaveFreq: float, stream2FreqMult: float,
    quantLevelsMinMax: list, streamsOffset: list, quantOffsets: list,
    streamSwitchDur: int, stream1QuantDur: int, stream2QuantDur: int, finalRandSwitch: bool,
    sine_lod_level: int,
    lod: int, max_search_frames: int,
    seg_block_length: int
) -> FPSR_Output:
    """
    ---- QS: Quantised Switching Wrapper with Details ----
    Wrapper for fpsr_qs that returns a detailed FPSR_Output struct.

    [PORT]: Ported from C docstring for fpsr_qs_get_details.

    Args:
        frame (int): The current frame or time input.
        frame_multiplier (float): The time scaling factor.
            < 1.0 = Slow-Motion (Time Stretch)
            = 1.0 = Normal Speed
            > 1.0 = Fast-Motion (Time Compression)
        baseWaveFreq (float): Algorithm parameter.
        stream2FreqMult (float): Algorithm parameter.
        quantLevelsMinMax (list[int]): Algorithm parameter.
        streamsOffset (list[int]): Algorithm parameter.
        quantOffsets (list[int]): Algorithm parameter.
        streamSwitchDur (int): Algorithm parameter.
        stream1QuantDur (int): Algorithm parameter.
        stream2QuantDur (int): Algorithm parameter.
        finalRandSwitch (bool): Algorithm parameter.
        sine_lod_level (int): Algorithm parameter.
        lod (int): The level of detail to calculate.
        max_search_frames (int): A safety limit for the backward/forward search.
        seg_block_length (int): The "runway" length for HPQ logic.

    Returns:
        FPSR_Output: A struct with metadata populated based on the LOD.
    """
    
    # Sanitize frame_multiplier
    fm = 1.0 if frame_multiplier == 0.0 else frame_multiplier

    # --- (START) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    scaled_frame_position = float(frame) * fm
    master_frame = math.floor(scaled_frame_position)
    master_frame_start_app_frame = math.ceil(float(master_frame) / fm)

    app_frames_into_gap = frame - master_frame_start_app_frame
    segment_index = 0
    local_progress_in_segment = 0
    
    if seg_block_length > 0:
        segment_index = app_frames_into_gap // seg_block_length
        local_progress_in_segment = app_frames_into_gap % seg_block_length

    base_qs_output = None
    if segment_index == 0:
        # --- MODE 1: "Tape Varispeed" (Anchor) ---
        base_qs_output = fpsr_qs_base(master_frame, float(baseWaveFreq), float(stream2FreqMult), quantLevelsMinMax, streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), finalRandSwitch, sine_lod_level)
    else:
        # --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        gap_seed = _splitmix64(_to_uint64(int(master_frame) + int(segment_index)))
        
        # For QS, inject the unique seed into the 'quantOffsets'.
        new_quantOffsets = [
            quantOffsets[0] + int(_to_uint64(gap_seed) & 0xFFFFFFFF),
            quantOffsets[1] + int((_to_uint64(gap_seed) >> 32) & 0xFFFFFFFF)
        ]
        
        # Use `local_progress_in_segment` as the "frame"
        base_qs_output = fpsr_qs_base(local_progress_in_segment, float(baseWaveFreq), float(stream2FreqMult), quantLevelsMinMax, streamsOffset, new_quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), finalRandSwitch, sine_lod_level)
    # --- (END) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    # Copy base results into the main output object
    out = base_qs_output

    if lod < 1: return out

    # LOD 1
    prev_out = fpsr_qs_get_details(frame - 1, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
    out.randVal_previous = prev_out.randVal
    out.has_changed = 1 if (out.randVal != out.randVal_previous) else 0
    
    if lod < 2: return out

    # --- LOD 2: MODIFIED Robust Search (on Application Timeline) ---
    next_val_candidate = 0.0
    step_int = 1

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = int(frame)
    else:
        bound_low_int = frame
        step_int = 1
        while (frame - step_int > frame - max_search_frames): 
            probe_qs_output = fpsr_qs_get_details(frame - step_int, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
            if (probe_qs_output.randVal != out.randVal):
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2
        
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1
        while(low_int <= high_int):
            mid_int = low_int + (high_int - low_int) // 2
            mid_qs_output = fpsr_qs_get_details(mid_int, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
            if (mid_qs_output.randVal == out.randVal):
                mid_minus_step_qs_output = fpsr_qs_get_details(mid_int - 1, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
                if (mid_minus_step_qs_output.randVal != out.randVal):
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = int(result_int)

    # --- Forwards search ---
    bound_high_int = frame
    step_int = 1
    while (frame + step_int < frame + max_search_frames): 
        probe_qs_output = fpsr_qs_get_details(frame + step_int, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
        if (probe_qs_output.randVal != out.randVal):
            bound_high_int = frame + step_int
            next_val_candidate = probe_qs_output.randVal
            break
        bound_high_int = frame + step_int
        step_int *= 2
    
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames
    while(low_int <= high_int):
        mid_int = low_int + (high_int - low_int) // 2
        mid_qs_output = fpsr_qs_get_details(mid_int, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0, seg_block_length)
        if (mid_qs_output.randVal != out.randVal):
            result_int = mid_int
            next_val_candidate = mid_qs_output.randVal
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = int(result_int)
    out.randVal_next_changed_frame = float(next_val_candidate)
    
    # --- UPDATED hold_progress Calculation ---
    hold_duration_app_frames = float(out.next_changed_frame) - float(out.last_changed_frame)
    if (hold_duration_app_frames > 0.0):
        out.hold_progress = float((float(frame) - float(out.last_changed_frame)) / hold_duration_app_frames)
    else:
        out.hold_progress = 0.0

    return out


def fpsr_bd_get_details(
    frame: int, frame_multiplier: float,
    block_size: int,
    streams_number: int,
    streams_offset: int,
    intra_op: str,
    dynamic_shift_bits: int,
    static_shift_amount: int,
    inter_op: str,
    value_seed_offset: int,
    lod: int, max_search_frames: int,
    seg_block_length: int
) -> FPSR_Output:
    """
    ---- BD: Bitwise Decode Wrapper with Details ----
    Wrapper for fpsr_bd that returns a detailed FPSR_Output struct.

    [PORT]: Ported from C docstring for fpsr_bd_get_details.

    Args:
        frame (int): The current frame or time input.
        frame_multiplier (float): The time scaling factor.
            < 1.0 = Slow-Motion (Time Stretch)
            = 1.0 = Normal Speed
            > 1.0 = Fast-Motion (Time Compression)
        block_size (int): Algorithm parameter.
        streams_number (int): Algorithm parameter.
        streams_offset (int): Algorithm parameter.
        intra_op (str): Algorithm parameter.
        dynamic_shift_bits (int): Algorithm parameter.
        static_shift_amount (int): Algorithm parameter.
        inter_op (str): Algorithm parameter.
        value_seed_offset (int): Algorithm parameter.
        lod (int): The level of detail to calculate.
        max_search_frames (int): A safety limit for the backward/forward search.
        seg_block_length (int): The "runway" length for HPQ logic.

    Returns:
        FPSR_Output: A struct with metadata populated based on the LOD.
    """
    out = FPSR_Output()
    
    # Sanitize frame_multiplier
    fm = 1.0 if frame_multiplier == 0.0 else frame_multiplier

    # --- (START) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    scaled_frame_position = float(frame) * fm
    master_frame = math.floor(scaled_frame_position)
    master_frame_start_app_frame = math.ceil(float(master_frame) / fm)

    app_frames_into_gap = frame - master_frame_start_app_frame
    segment_index = 0
    local_progress_in_segment = 0
    
    if seg_block_length > 0:
        segment_index = app_frames_into_gap // seg_block_length
        local_progress_in_segment = app_frames_into_gap % seg_block_length

    if segment_index == 0:
        # --- MODE 1: "Tape Varispeed" (Anchor) ---
        out.randVal = float(fpsr_bd(
            master_frame, int(block_size), streams_number, int(streams_offset),
            intra_op, dynamic_shift_bits, static_shift_amount, inter_op, int(value_seed_offset)
        ))
    else:
        # --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        gap_seed = _splitmix64(_to_uint64(int(master_frame) + int(segment_index)))
        
        # For BD, inject the unique seed as the 'value_seed_offset'.
        out.randVal = float(fpsr_bd(
            local_progress_in_segment, int(block_size), streams_number, int(streams_offset),
            intra_op, dynamic_shift_bits, static_shift_amount, inter_op, int(gap_seed)
        ))
    # --- (END) HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if lod < 1: return out

    # LOD 1
    prev_out = fpsr_bd_get_details(frame - 1, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length)
    out.randVal_previous = prev_out.randVal
    out.has_changed = 1 if (out.randVal != out.randVal_previous) else 0

    if lod < 2: return out

    # --- LOD 2: MODIFIED Robust Search (on Application Timeline) ---
    next_val_candidate = 0.0
    step_int = 1

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = int(frame)
    else:
        bound_low_int = frame
        step_int = 1
        while (frame - step_int > frame - max_search_frames):
            val_at_probe = fpsr_bd_get_details(frame - step_int, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length).randVal
            if (val_at_probe != out.randVal):
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2
        
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1
        while(low_int <= high_int):
            mid_int = low_int + (high_int - low_int) // 2
            mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length).randVal
            if (mid_val == out.randVal):
                prev_mid_val = fpsr_bd_get_details(mid_int - 1, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length).randVal
                if (prev_mid_val != out.randVal):
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = int(result_int)

    # --- Forwards search ---
    bound_high_int = frame
    step_int = 1
    while (frame + step_int < frame + max_search_frames):
        val_at_probe = fpsr_bd_get_details(frame + step_int, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length).randVal
        if (val_at_probe != out.randVal):
            bound_high_int = frame + step_int
            next_val_candidate = val_at_probe
            break
        bound_high_int = frame + step_int
        step_int *= 2
    
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames
    while(low_int <= high_int):
        mid_int = low_int + (high_int - low_int) // 2
        mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length).randVal
        if (mid_val != out.randVal):
            result_int = mid_int
            next_val_candidate = mid_val
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = int(result_int)
    out.randVal_next_changed_frame = float(next_val_candidate)
    
    # --- UPDATED hold_progress Calculation ---
    hold_duration_app_frames = float(out.next_changed_frame) - float(out.last_changed_frame)
    if (hold_duration_app_frames > 0.0):
        out.hold_progress = float((float(frame) - float(out.last_changed_frame)) / hold_duration_app_frames)
    else:
        out.hold_progress = 0.0

    return out


if __name__ == "__main__":
    # Example usage of the FPSR algorithms with detailed output
    
    # Algorithms: 0 - SM, 1 - TM, 2 - QS, 3 - BD
    algo = 3 # Change this value to 0, 1, 2, or 3 to test different algorithms
    algo_name = ["SM", "TM", "QS", "BD"] # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103, 100] # Starting frames for each algorithm
    num_frames = 30 # Run a loop of 30 frames to demonstrate changes
    lod = 2 # Level of detail (0, 1, or 2) for rich output
    
    # 1.0 = normal speed
    # 0.5 = 0.5x speed (slow motion / time stretch)
    # 2.0 = 2.0x speed (fast motion / time compression)
    main_frame_multiplier = 1.0 # Default value representing "Normal Speed"
    
    speed_mode_description = ""
    # [FIX]: C-style comment converted to Python
    # Check if the frame multiplier is less than 1.0, indicating "Slow-Down" mode
    if main_frame_multiplier < 1.0:
        speed_mode_description = "Slow-Down"
    elif main_frame_multiplier > 1.0: # Speed-Up mode
        speed_mode_description = "Speed-Up"
    else:
        speed_mode_description = "Normal Speed"
    print(f"Frame Multiplier: {main_frame_multiplier:.2f} ({speed_mode_description})")
    
    # NEW: HPQ Parameter
    # A value of 5 means "tape varispeed" holds until a 5x stretch
    # (i.e., frame_multiplier <= 0.2), at which point new generative
    # phrases kick in. (5 = 1.0 / 0.2)
    seg_block_length = 5

    for loop_frame in range(num_frames):
        frame = loop_frame + start_frames[algo] # Use int
        frame_multiplier = main_frame_multiplier # Use float
        output = FPSR_Output()
        
        if algo == 0:
            # Parameters for FPS-R:SM
            minHoldFrames = 7      # Minimum hold duration
            maxHoldFrames = 9      # Maximum hold duration
            reseedFrames = 6       # Reseed interval
            offsetInner = -41      # Inner seed offset
            offsetOuter = 23       # Outer seed offset
            finalRandSwitch = True # Final randomisation switch
            max_search_frames = 50 # Safety limit for search

            # Call fpsr_sm_get_details
            output = fpsr_sm_get_details(frame, frame_multiplier, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch, lod, max_search_frames, seg_block_length)
        
        elif algo == 1:
            # Parameters for FPS-R:TM
            periodA = 8            # First hold duration
            periodB = 5            # Second hold duration
            periodSwitch = 6       # Period switch interval
            offsetInner = 15       # Inner seed offset
            offsetOuter = 0        # Outer seed offset
            finalRandSwitch = True # Final randomisation switch
            max_search_frames = 50 # Safety limit for search

            # Call fpsr_tm_get_details
            output = fpsr_tm_get_details(frame, frame_multiplier,
                periodA, periodB, periodSwitch, offsetInner, offsetOuter, 
                finalRandSwitch, lod, max_search_frames, seg_block_length)
        
        elif algo == 2:
            # Parameters for FPS-R:QS
            baseWaveFreq = 0.012    # Base wave frequency for stream 1
            stream2FreqMult = 3.1   # Frequency multiplier for stream 2
            quantLevelsMinMax = [4, 12] # Min and max quantisation levels
            streamsOffset = [0, 76] # Frame offsets for each stream
            quantOffsets = [10, 81] # Quantisation level offsets
            streamSwitchDur = 8        # Duration after which streams switch
            stream1QuantDur = 10       # Duration for stream 1 quantisation hold
            stream2QuantDur = 13       # Duration for stream 2 quantisation hold
            finalRandSwitch = True     # Final randomisation switch
            sine_lod_level = 4         # Sine wave LOD level (0-4)
            max_search_frames = 50     # Safety limit for search

            # Call fpsr_qs_get_details
            output = fpsr_qs_get_details(frame, frame_multiplier, baseWaveFreq, stream2FreqMult, 
                quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, 
                stream1QuantDur, stream2QuantDur, finalRandSwitch, 
                sine_lod_level, lod, max_search_frames, seg_block_length)
        
        elif algo == 3:
            # Parameters for FPS-R:BD
            p_block_size = 64           # Size of the macro-rhythm block
            p_streams_number = 2        # Number of parallel bitstreams
            p_streams_offset = 10       # Frame offset between each stream's seed
            # Intra-stream operation
            #      Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
            #      Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
            p_intra_op = "rotl_dynamic" # Intra-stream operation on each stream
            p_dynamic_shift_bits = 6    # Dynamic shift bits for intra-op
            p_static_shift_amount = 1   # Static shift amount for intra-op
            p_inter_op = "xor"  # Inter-stream operation to combine transformed streams
                                #     Options: "xor", "or", "and".
            p_value_seed_offset = 78901 # Additional seed offset for final value
            max_search_frames = 100 # BD blocks can be large

            output = fpsr_bd_get_details(
                frame, frame_multiplier, p_block_size, p_streams_number, p_streams_offset,
                p_intra_op, p_dynamic_shift_bits, p_static_shift_amount,
                p_inter_op, p_value_seed_offset, lod, max_search_frames, seg_block_length
            )

        # Print the output for the current frame
        print(f"Frame {frame}: randVal {output.randVal:.6f}, prevVal {output.randVal_previous:.6f}, changed {output.has_changed}, "
              f"progress {output.hold_progress:.3f}, last {output.last_changed_frame}, next {output.next_changed_frame} ", end="")
        
        if algo == 2:
            print(f"| s_idx {output.selected_stream_idx}, s[0] {output.randStreams[0]:.3f}, s[1] {output.randStreams[1]:.3f} ", end="")
        
        if output.has_changed:
            print("(jumped)", end="")
        
        print() # Newline