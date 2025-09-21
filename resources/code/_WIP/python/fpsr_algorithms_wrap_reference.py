# SPDX-License-Identifier: MIT — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

"""
@file fpsr_wrapped.c
@brief This file demonstrates a wrapper-based approach for getting rich metadata
from the core FPS-R algorithms.
@details This implementation separates the pure, stateless algorithms from the
functions that gather detailed metadata. The wrapper functions perform a robust,
two-phase search (exponential probe + binary search) 
to populate the FPSR_Output struct.
This method is highly efficient and avoids "false positive" value collisions.
"""

import math
import sys
from typing import List, Sequence

# Define block for compile-time constants
FPSR_INFLATION_FACTOR: float = 100000000.0  # 10^8 for 8 decimal places of precision

SINE_LUT_SIZE_100 = 100
SINE_LUT_SIZE_500 = 500
SINE_LUT_SIZE_1000 = 1000
SINE_LUT_SIZE_4096 = 4096  # Highest precision default

# Global constant for 2*PI (double precision)
TWO_PI: float = 6.28318530718  # Global constant for 2*PI (double precision)

# Global sine lookup tables
_sine_lut_100: List[float] = [0.0] * SINE_LUT_SIZE_100
_sine_lut_500: List[float] = [0.0] * SINE_LUT_SIZE_500
_sine_lut_1000: List[float] = [0.0] * SINE_LUT_SIZE_1000
_sine_lut_4096: List[float] = [0.0] * SINE_LUT_SIZE_4096  # Highest precision default

# Flag to track if LUTs are initialized
_luts_initialized: int = 0

# Function to initialize all sine lookup tables
# THIS FUNCTION MUST BE CALLED ONCE AT PROGRAM STARTUP!
def initialize_sine_luts() -> None:
    global _luts_initialized
    if _luts_initialized:
        return  # Only initialize once

    for i in range(SINE_LUT_SIZE_100):
        _sine_lut_100[i] = math.sin(i / SINE_LUT_SIZE_100 * TWO_PI)
    for i in range(SINE_LUT_SIZE_500):
        _sine_lut_500[i] = math.sin(i / SINE_LUT_SIZE_500 * TWO_PI)
    for i in range(SINE_LUT_SIZE_1000):
        _sine_lut_1000[i] = math.sin(i / SINE_LUT_SIZE_1000 * TWO_PI)
    for i in range(SINE_LUT_SIZE_4096):
        _sine_lut_4096[i] = math.sin(i / SINE_LUT_SIZE_4096 * TWO_PI)

    _luts_initialized = 1

# Helper function to get sine value from a specific LUT with linear interpolation
"""
Sine LUT determinism note:
- The LUT-based sine approximation yields deterministic, bit-for-bit identical results
  when given the same phase modulo 2π.
- Linear interpolation is performed in double precision to minimize rounding drift
  and to maintain cross-platform parity.
"""
def _get_sine_from_lod_lut(phase: float, lut_size: int, lut_array: Sequence[float]) -> float:
    if not _luts_initialized:
        # Fallback or error if LUTs not initialized.
        # For absolute determinism, this should ideally not happen in production.
        # For now, we will fall back to standard sin() with a warning.
        print("WARNING: Sine LUTs not initialized. Falling back to sin(). Call initialize_sine_luts() once.", file=sys.stderr)
        return math.sin(phase)

    # Wrap phase to 0 to 2*PI range
    phase = math.fmod(phase, TWO_PI)
    if phase < 0:
        phase += TWO_PI  # Ensure positive for fmod results

    # Map phase to LUT index range
    fractional_index = phase / TWO_PI * lut_size

    # Get integer part and fractional part
    index1 = int(math.floor(fractional_index))
    frac = fractional_index - index1

    # Handle wrap-around for index2 (last point wraps to first)
    index2 = (index1 + 1) % lut_size

    # Linear interpolation
    return lut_array[index1] * (1.0 - frac) + lut_array[index2] * frac


###############################################################################
# Core Components (Struct and portable_rand)                                  #
###############################################################################

# A simple, portable pseudo-random number generator that takes an integer seed.
# Internal calculations use double for higher precision, result is float.
# Now uses the global sine lookup table for absolute determinism.
"""
Bit-for-bit determinism note:
- Accepts a 64-bit seed and uses only integer math and LUT-based sine via _get_sine_from_lod_lut.
- Avoids platform-dependent libm sin rounding so results are identical across compilers/OS/CPU.
- The final subtraction with floor() mirrors GLSL fract() semantics to maintain parity.
"""
def portable_rand(seed: int) -> float:  # Changed seed to 64-bit to avoid overflow in seeds
    val = float(seed) * 12.9898  # Use double literal
    val = math.fmod(val, TWO_PI)  # This ensures 'val' is in [0, 2*PI)
    if val < 0:
        val += TWO_PI  # Ensure positive for fmod results

    # Use the highest precision LUT for sine calculation for absolute determinism
    # This ensures portable_rand's output is consistent across all LOD choices for QS.
    result_sin = _get_sine_from_lod_lut(val, SINE_LUT_SIZE_4096, _sine_lut_4096)
    result = result_sin * 43758.5453
    return float(result - math.floor(result))  # Use floor (double version), cast to float for return


###############################################################################
# FPS-R Output Structure                                                      #
###############################################################################
"""
This structure holds the output of the FPS-R algorithms.
The LOD (Level of Detail) determines the computational overhead and the amount of information returned.
This structure is designed to be flexible and can be extended in the future.
Different LODs will return different sets of fields:
- LOD 0: randVal
- LOD 1: randVal, has_changed
- LOD 2: randVal, has_changed, hold_progress, last_changed_frame, next_changed_frame,
randVal_next_changed_frame, randStreams[2], selected_stream (for QS algorithm)
Note: All fields will be set to 0 if the LOD is not applicable.
The fields are:
float randVal: LOD 0, 1, 2. The random value generated by the FPS-R algorithm.
int has_changed: LOD 1, 2. A flag indicating whether randVal has changed from the previous frame.
int randVal_previous: LOD 1, 2. The random value from the previous frame for change detection.
float hold_progress: LOD 2. The progress of the hold duration, normalised to [0, 1].
int last_changed_frame: LOD 2. The precise frame (integer) when the random value last changed.
int next_changed_frame: LOD 2. The precise frame (integer) when the random value will next change.
float randVal_next_changed_frame: LOD 2. The value that the algorithm will jump to at next_changed_frame.
double randStreams[2]: LOD 2. (Exclusive to QS) The raw values of stream1_double and stream2_double.
int selected_stream: LOD 2. (Exclusive to QS) The index of the stream (0 for stream1, 1 for stream2) that was selected by the algorithm.
"""

class FPSR_Output:
    # Python class mirroring the C struct. Fields default to zero-equivalents.
    def __init__(self):
        self.randVal: float = 0.0
        self.has_changed: int = 0
        self.randVal_previous: float = 0.0
        self.hold_progress: float = 0.0
        self.last_changed_frame: int = 0
        self.next_changed_frame: int = 0
        self.randVal_next_changed_frame: float = 0.0
        # New fields for QS details
        self.randStreams: List[float] = [0.0, 0.0]
        self.selected_stream_idx: int = 0  # 0 for stream1, 1 for stream2


###############################################################################
# Untouched, Low-Level FPS-R Algorithms                                       #
###############################################################################
# These functions remain pure, returning only a single float value.
# All three map the double-scaled time into a 64-bit integer timeline so that all
# modulo/offset/hold computations are done with integers for bit-for-bit stability.

def _fpsr_sm_base(
    frame_input_from_wrapper: float,  # frame is now double from wrapper
    minHold: int, maxHold: int,
    reseedInterval: int, seedInner: int, seedOuter: int, finalRandSwitch: int
) -> float:
    # Convert scaled double frame to large integer for pure integer math (64-bit to avoid overflow)
    int_frame = int(math.floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR))

    # Scale all time-based integer parameters to match the int_frame resolution (64-bit)
    internal_minHold = int(math.floor(float(minHold) * FPSR_INFLATION_FACTOR))
    internal_maxHold = int(math.floor(float(maxHold) * FPSR_INFLATION_FACTOR))
    internal_reseedInterval = int(math.floor(float(reseedInterval) * FPSR_INFLATION_FACTOR))
    internal_seedInner = int(math.floor(float(seedInner) * FPSR_INFLATION_FACTOR))
    internal_seedOuter = int(math.floor(float(seedOuter) * FPSR_INFLATION_FACTOR))

    # Ensure minimum tick (1 inflated unit)
    one_tick = int(math.floor(1.0 * FPSR_INFLATION_FACTOR))
    if internal_reseedInterval < one_tick:
        internal_reseedInterval = one_tick

    # Stable 64-bit reseed boundary and duration randomisation
    rand_for_duration_seed = internal_seedInner + int_frame - (int_frame % internal_reseedInterval)
    rand_for_duration = portable_rand(rand_for_duration_seed)

    holdDuration = internal_minHold + int(math.floor(rand_for_duration * float(internal_maxHold - internal_minHold)))
    if holdDuration < one_tick:
        holdDuration = one_tick

    # 64-bit modulo for held integer state (normalized to [0, holdDuration))
    rem = (internal_seedOuter + int_frame) % holdDuration
    if rem < 0:
        rem += holdDuration

    if finalRandSwitch:
        return portable_rand(rem)  # Seed is 64-bit integer state
    return float(rem)


def _fpsr_tm_base(
    frame_input_from_wrapper: float,  # frame is now double from wrapper
    periodA: int, periodB: int,
    periodSwitch: int, seedInner: int, seedOuter: int, finalRandSwitch: int
) -> float:
    # Convert scaled double frame to large integer for pure integer math (64-bit to avoid overflow)
    int_frame = int(math.floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR))

    # Scale all time-based integer parameters to match the int_frame resolution (64-bit)
    internal_periodA = int(math.floor(float(periodA) * FPSR_INFLATION_FACTOR))
    internal_periodB = int(math.floor(float(periodB) * FPSR_INFLATION_FACTOR))
    internal_periodSwitch = int(math.floor(float(periodSwitch) * FPSR_INFLATION_FACTOR))
    internal_seedInner = int(math.floor(float(seedInner) * FPSR_INFLATION_FACTOR))
    internal_seedOuter = int(math.floor(float(seedOuter) * FPSR_INFLATION_FACTOR))

    one_tick = int(math.floor(1.0 * FPSR_INFLATION_FACTOR))
    if internal_periodSwitch < one_tick:
        internal_periodSwitch = one_tick

    # Toggle using 64-bit modulo
    holdDuration: int
    if (int_frame % internal_periodSwitch) < (internal_periodSwitch // 2):
        holdDuration = internal_periodA
    else:
        holdDuration = internal_periodB
    if holdDuration < one_tick:
        holdDuration = one_tick

    # 64-bit modulo for held integer state (normalized)
    rem = (internal_seedOuter + int_frame) % holdDuration
    if rem < 0:
        rem += holdDuration

    if finalRandSwitch:
        return portable_rand(rem)  # Seed is 64-bit integer state
    return float(rem)


def _fpsr_qs_base(
    frame_input_from_wrapper: float,  # frame is now double from wrapper
    baseWaveFreq: float, stream2FreqMult: float,
    quantLevelsMinMax: Sequence[int], streamsOffset: Sequence[int], quantOffsets: Sequence[int],
    streamSwitchDur: int, stream1QuantDur: int, stream2QuantDur: int, finalRandSwitch: int,
    sine_lod_level: int  # New parameter for sine LOD
) -> FPSR_Output:
    output = FPSR_Output()  # Initialize output struct

    # Convert scaled double frame to large integer for pure integer math (64-bit)
    int_frame = int(math.floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR))

    # Scale all time-based integer parameters to match the int_frame resolution (64-bit)
    internal_streamSwitchDur = int(math.floor(float(streamSwitchDur) * FPSR_INFLATION_FACTOR))
    internal_stream1QuantDur = int(math.floor(float(stream1QuantDur) * FPSR_INFLATION_FACTOR))
    internal_stream2QuantDur = int(math.floor(float(stream2QuantDur) * FPSR_INFLATION_FACTOR))
    internal_streamsOffset_0 = int(math.floor(float(streamsOffset[0]) * FPSR_INFLATION_FACTOR))
    internal_streamsOffset_1 = int(math.floor(float(streamsOffset[1]) * FPSR_INFLATION_FACTOR))
    internal_quantOffsets_0 = int(math.floor(float(quantOffsets[0]) * FPSR_INFLATION_FACTOR))
    internal_quantOffsets_1 = int(math.floor(float(quantOffsets[1]) * FPSR_INFLATION_FACTOR))

    one_tick = int(math.floor(1.0 * FPSR_INFLATION_FACTOR))
    if internal_streamSwitchDur < one_tick:
        internal_streamSwitchDur = one_tick
    if internal_stream1QuantDur < one_tick:
        internal_stream1QuantDur = one_tick
    if internal_stream2QuantDur < one_tick:
        internal_stream2QuantDur = one_tick

    quant_min = int(quantLevelsMinMax[0])
    quant_max = int(quantLevelsMinMax[1])
    quant_range = quant_max - quant_min + 1
    if quant_range < 1:
        quant_range = 1

    # Use 64-bit modulo boundaries for seeds
    s1_quant_seed = internal_quantOffsets_0 + int_frame - (int_frame % internal_stream1QuantDur)
    s1_quant_level = quant_min + int(math.floor(portable_rand(s1_quant_seed) * quant_range))

    s2_quant_seed = internal_quantOffsets_1 + int_frame - (int_frame % internal_stream2QuantDur)
    s2_quant_level = quant_min + int(math.floor(portable_rand(s2_quant_seed) * quant_range))

    if s1_quant_level < 1:
        s1_quant_level = 1
    if s2_quant_level < 1:
        s2_quant_level = 1

    if stream2FreqMult <= 0:
        stream2FreqMult = 3.7  # Still a float input

    # Frequencies must be deflated to match the inflated int_frame resolution
    deflated_baseWaveFreq = float(baseWaveFreq) / FPSR_INFLATION_FACTOR
    # stream2FreqMult is a multiplier to deflated_baseWaveFreq
    deflated_stream2FreqMult_applied = deflated_baseWaveFreq * float(stream2FreqMult)

    # Select sine generation method based on sine_lod_level
    def _sin_lod(phase: float) -> float:
        if sine_lod_level == 0:
            return math.sin(phase)
        elif sine_lod_level == 1:
            return _get_sine_from_lod_lut(phase, SINE_LUT_SIZE_100, _sine_lut_100)
        elif sine_lod_level == 2:
            return _get_sine_from_lod_lut(phase, SINE_LUT_SIZE_500, _sine_lut_500)
        elif sine_lod_level == 3:
            return _get_sine_from_lod_lut(phase, SINE_LUT_SIZE_1000, _sine_lut_1000)
        else:  # 4 or invalid -> highest precision
            return _get_sine_from_lod_lut(phase, SINE_LUT_SIZE_4096, _sine_lut_4096)

    stream1_raw_sine = _sin_lod((float(internal_streamsOffset_0) + float(int_frame)) * deflated_baseWaveFreq)
    stream2_raw_sine = _sin_lod((float(internal_streamsOffset_1) + float(int_frame)) * deflated_stream2FreqMult_applied)

    output.randStreams[0] = math.floor(stream1_raw_sine * s1_quant_level) / float(s1_quant_level)
    output.randStreams[1] = math.floor(stream2_raw_sine * s2_quant_level) / float(s2_quant_level)

    # Use 64-bit frame for modulo operation
    output.selected_stream_idx = 0 if (int_frame % internal_streamSwitchDur) < (internal_streamSwitchDur // 2) else 1

    active_stream_val_double = output.randStreams[0] if output.selected_stream_idx == 0 else output.randStreams[1]

    if finalRandSwitch:
        seed = int(math.floor(active_stream_val_double * FPSR_INFLATION_FACTOR))  # 64-bit seed
        output.randVal = portable_rand(seed)  # Seed is large integer
    else:
        output.randVal = float(0.5 * active_stream_val_double + 0.5)  # Return float, calculations in double

    return output


###############################################################################
# High-Level Wrapper Functions with Robust Search                             #
###############################################################################

def fpsr_sm_get_details(
    frame: int, frame_multiplier: float,  # Reordered frame_multiplier
    minHold: int, maxHold: int,
    reseedInterval: int, seedInner: int, seedOuter: int, finalRandSwitch: int,
    lod: int, max_search_frames: int
) -> FPSR_Output:
    out = FPSR_Output()

    # Calculate the scaled frame input for the base algorithm (as double)
    current_scaled_frame_double = float(frame) * frame_multiplier

    # LOD 0: Get current value.
    out.randVal = _fpsr_sm_base(current_scaled_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch)

    if lod < 1:
        return out

    # LOD 1: Compare with previous frame to check for change.
    # Calculate the scaled frame input for the previous frame
    prev_scaled_frame_for_lod1_double = float(frame - 1) * frame_multiplier
    prev_val = _fpsr_sm_base(prev_scaled_frame_for_lod1_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch)
    out.randVal_previous = prev_val
    out.has_changed = int(out.randVal != prev_val)

    if lod < 2:
        return out

    # LOD 2: Use a robust two-phase search to find change frames.
    next_val_candidate = 0.0  # Stores the value at the next_changed_frame
    step_int = 1  # Used for exponential probe step

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = frame
    else:
        bound_low_int = frame  # operates on original frame space (int)
        step_int = 1  # Reset for exponential probe step
        while (frame - step_int) > (frame - max_search_frames):
            probe_frame_double = float(frame - step_int) * frame_multiplier
            val_at_probe = _fpsr_sm_base(probe_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch)
            if val_at_probe != out.randVal:
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2

        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1  # Default to earliest searched frame
        while low_int <= high_int:
            mid_int = low_int + (high_int - low_int) // 2
            mid_frame_double = float(mid_int) * frame_multiplier
            if _fpsr_sm_base(mid_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) == out.randVal:
                mid_minus_step_frame_double = float(mid_int - 1) * frame_multiplier
                if _fpsr_sm_base(mid_minus_step_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) != out.randVal:
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = result_int

    # --- Forwards Search for next_changed_frame ---
    bound_high_int = frame  # operates on original frame space (int)
    step_int = 1  # Reset for exponential probe step
    while (frame + step_int) < (frame + max_search_frames):
        probe_frame_double = float(frame + step_int) * frame_multiplier
        val_at_probe = _fpsr_sm_base(probe_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch)
        if val_at_probe != out.randVal:
            bound_high_int = frame + step_int
            next_val_candidate = val_at_probe  # Store the value at the first differing frame
            break
        bound_high_int = frame + step_int
        step_int *= 2

    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames  # Default if no change is found, in original frame space (int)
    while low_int <= high_int:
        mid_int = low_int + (high_int - low_int) // 2
        mid_frame_double = float(mid_int) * frame_multiplier
        mid_val = _fpsr_sm_base(mid_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch)
        if mid_val != out.randVal:
            result_int = mid_int
            next_val_candidate = mid_val  # Store the value at this frame
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = result_int
    out.randVal_next_changed_frame = next_val_candidate

    # Calculate hold progress based on scaled frame values
    scaled_last_changed_frame_val_double = float(out.last_changed_frame) * frame_multiplier
    scaled_next_changed_frame_val_double = float(out.next_changed_frame) * frame_multiplier
    hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double

    if hold_duration_scaled_double > 0.0:
        out.hold_progress = float((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double)
    else:
        out.hold_progress = 0.0  # Handle zero duration to avoid division by zero

    return out


def fpsr_tm_get_details(
    frame: int, frame_multiplier: float,  # Reordered frame_multiplier
    periodA: int, periodB: int,
    periodSwitch: int, seedInner: int, seedOuter: int, finalRandSwitch: int,
    lod: int, max_search_frames: int
) -> FPSR_Output:
    out = FPSR_Output()

    # Calculate the scaled frame input for the base algorithm (as double)
    current_scaled_frame_double = float(frame) * frame_multiplier  # Cast frame to double for multiplication

    # LOD 0
    out.randVal = _fpsr_tm_base(current_scaled_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch)

    if lod < 1:
        return out

    # LOD 1: Compare with previous frame to check for change.
    # Calculate the scaled frame input for the previous frame
    prev_scaled_frame_for_lod1_double = float(frame - 1) * frame_multiplier
    prev_val = _fpsr_tm_base(prev_scaled_frame_for_lod1_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch)
    out.randVal_previous = prev_val
    out.has_changed = int(out.randVal != prev_val)

    if lod < 2:
        return out

    # LOD 2: Robust Search
    next_val_candidate = 0.0  # Stores the value at the next_changed_frame
    step_int = 1  # Used for exponential probe step

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = frame
    else:
        bound_low_int = frame  # operates on original frame space (int)
        step_int = 1  # Reset for exponential probe step
        while (frame - step_int) > (frame - max_search_frames):  # All int
            # Scale the probe frame (as double) before passing to base algorithm
            probe_frame_double = float(frame - step_int) * frame_multiplier  # All double
            if _fpsr_tm_base(probe_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal:
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2  # Changed to int
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1  # Default to earliest searched frame
        while low_int <= high_int:
            mid_int = low_int + (high_int - low_int) // 2  # All int
            # Scale the mid frame (as double) before passing to base algorithm
            mid_frame_double = float(mid_int) * frame_multiplier  # All double
            if _fpsr_tm_base(mid_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) == out.randVal:
                # Check the frame immediately preceding 'mid' in the scaled timeline
                # using 1 as the step for comparison
                mid_minus_step_frame_double = float(mid_int - 1) * frame_multiplier
                if _fpsr_tm_base(mid_minus_step_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal:
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = result_int

    # Forwards search
    bound_high_int = frame  # operates on original frame space (int)
    step_int = 1  # Reset for exponential probe step
    while (frame + step_int) < (frame + max_search_frames):  # All int
        # Scale the probe frame (as double) before passing to base algorithm
        probe_frame_double = float(frame + step_int) * frame_multiplier  # All double
        val_at_probe = _fpsr_tm_base(probe_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch)
        if val_at_probe != out.randVal:
            bound_high_int = frame + step_int
            next_val_candidate = val_at_probe  # Store the value at the first differing frame
            break
        bound_high_int = frame + step_int
        step_int *= 2  # Changed to int
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames  # Default if no change is found, in original frame space (int)
    while low_int <= high_int:
        mid_int = low_int + (high_int - low_int) // 2  # All int
        # Scale the mid frame (as double) before passing to base algorithm
        mid_frame_double = float(mid_int) * frame_multiplier  # All double
        mid_val = _fpsr_tm_base(mid_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch)
        if mid_val != out.randVal:
            result_int = mid_int
            next_val_candidate = mid_val  # Store the value at this frame
            high_int = mid_int - 1  # All int
        else:
            low_int = mid_int + 1  # All int
    out.next_changed_frame = result_int

    # Calculate hold progress based on scaled frame values
    scaled_last_changed_frame_val_double = float(out.last_changed_frame) * frame_multiplier  # All double
    scaled_next_changed_frame_val_double = float(out.next_changed_frame) * frame_multiplier  # All double
    hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double

    if hold_duration_scaled_double > 0.0:  # Use double literal
        out.hold_progress = float((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double)  # Cast final result to float
    else:
        out.hold_progress = 0.0  # Handle zero duration to avoid division by zero

    out.randVal_next_changed_frame = next_val_candidate  # Assign the captured value

    return out


def fpsr_qs_get_details(
    frame: int, frame_multiplier: float,  # Reordered frame_multiplier
    baseWaveFreq: float, stream2FreqMult: float,
    quantLevelsMinMax: Sequence[int], streamsOffset: Sequence[int], quantOffsets: Sequence[int],
    streamSwitchDur: int, stream1QuantDur: int, stream2QuantDur: int, finalRandSwitch: int,
    sine_lod_level: int,  # New parameter for sine LOD
    lod: int, max_search_frames: int
) -> FPSR_Output:
    out = FPSR_Output()

    # Calculate the scaled frame input for the base algorithm (as double)
    current_scaled_frame_double = float(frame) * frame_multiplier  # Cast frame to double for multiplication

    # LOD 0
    base_qs_output = _fpsr_qs_base(current_scaled_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
    out.randVal = base_qs_output.randVal
    out.randStreams[0] = base_qs_output.randStreams[0]
    out.randStreams[1] = base_qs_output.randStreams[1]
    out.selected_stream_idx = base_qs_output.selected_stream_idx

    if lod < 1:
        return out

    # LOD 1: Compare with previous frame to check for change.
    # Calculate the scaled frame input for the previous frame
    prev_scaled_frame_for_lod1_double = float(frame - 1) * frame_multiplier
    prev_qs_output = _fpsr_qs_base(prev_scaled_frame_for_lod1_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
    out.randVal_previous = prev_qs_output.randVal
    out.has_changed = int(out.randVal != out.randVal_previous)

    if lod < 2:
        return out

    # LOD 2: Robust Search
    next_val_candidate = 0.0  # Stores the value at the next_changed_frame
    step_int = 1  # Used for exponential probe step

    # --- Backwards Search for last_changed_frame ---
    if out.has_changed:
        out.last_changed_frame = frame
    else:
        bound_low_int = frame  # operates on original frame space (int)
        step_int = 1  # Reset for exponential probe step
        while (frame - step_int) > (frame - max_search_frames):
            # Scale the probe frame (as double) before passing to base algorithm
            probe_frame_double = float(frame - step_int) * frame_multiplier
            probe_qs_output = _fpsr_qs_base(probe_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
            if probe_qs_output.randVal != out.randVal:
                bound_low_int = frame - step_int
                break
            bound_low_int = frame - step_int
            step_int *= 2
        low_int = bound_low_int
        high_int = frame
        result_int = frame - max_search_frames + 1  # Default to earliest searched frame
        while low_int <= high_int:
            mid_int = low_int + (high_int - low_int) // 2
            # Scale the mid frame (as double) before passing to base algorithm
            mid_frame_double = float(mid_int) * frame_multiplier
            mid_qs_output = _fpsr_qs_base(mid_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
            if mid_qs_output.randVal == out.randVal:
                # Check the frame immediately preceding 'mid' in the scaled timeline
                # using 1 as the step for comparison
                mid_minus_step_frame_double = float(mid_int - 1) * frame_multiplier
                mid_minus_step_qs_output = _fpsr_qs_base(mid_minus_step_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
                if mid_minus_step_qs_output.randVal != out.randVal:
                    result_int = mid_int
                    break
                high_int = mid_int - 1
            else:
                low_int = mid_int + 1
        out.last_changed_frame = result_int

    # Forwards search
    bound_high_int = frame  # operates on original frame space (int)
    step_int = 1  # Reset for exponential probe step
    while (frame + step_int) < (frame + max_search_frames):
        # Scale the probe frame (as double) before passing to base algorithm
        probe_frame_double = float(frame + step_int) * frame_multiplier
        probe_qs_output = _fpsr_qs_base(probe_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
        if probe_qs_output.randVal != out.randVal:
            bound_high_int = frame + step_int
            next_val_candidate = probe_qs_output.randVal  # Store the value at the first differing frame
            break
        bound_high_int = frame + step_int
        step_int *= 2
    low_int = frame
    high_int = bound_high_int
    result_int = frame + max_search_frames  # Default if no change is found, in original frame space (int)
    while low_int <= high_int:
        mid_int = low_int + (high_int - low_int) // 2
        # Scale the mid frame (as double) before passing to base algorithm
        mid_frame_double = float(mid_int) * frame_multiplier
        mid_qs_output = _fpsr_qs_base(mid_frame_double, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level)
        if mid_qs_output.randVal != out.randVal:
            result_int = mid_int
            next_val_candidate = mid_qs_output.randVal  # Store the value at this frame
            high_int = mid_int - 1
        else:
            low_int = mid_int + 1
    out.next_changed_frame = result_int

    # Calculate hold progress based on scaled frame values
    scaled_last_changed_frame_val_double = float(out.last_changed_frame) * frame_multiplier
    scaled_next_changed_frame_val_double = float(out.next_changed_frame) * frame_multiplier
    hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double

    if hold_duration_scaled_double > 0.0:
        out.hold_progress = float((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double)
    else:
        out.hold_progress = 0.0  # Handle zero duration to avoid division by zero

    out.randVal_next_changed_frame = next_val_candidate

    return out


def main() -> int:
    # Example usage of the FPSR algorithms with detailed output

    initialize_sine_luts()  # Initialize sine lookup tables

    # Algorithms: 0 - SM, 1 - TM, 2 - QS
    algo = 0  # Change this value to 0, 1, or 2 to test different algorithms
    algo_name = ["SM", "TM", "QS"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103]  # Starting frames for each algorithm
    num_frames = 20  # Run a loop of 20 frames to demonstrate changes
    lod = 2  # Level of detail (0, 1, or 2) for rich output

    for loop_frame in range(num_frames):
        frame = loop_frame + start_frames[algo]  # Starting frame for the selected algorithm
        frame_multiplier = 1.0  # Speed factor
        output = FPSR_Output()  # Variable to hold the FPSR_Output struct

        if algo == 0:
            # Parameters for FPS-R:SM
            minHoldFrames = 7
            maxHoldFrames = 9
            reseedFrames = 6
            offsetInner = -41
            offsetOuter = 23
            finalRandSwitch = 1
            max_search_frames = 50

            # Call fpsr_sm_get_details
            output = fpsr_sm_get_details(frame, frame_multiplier, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch, lod, max_search_frames)
        elif algo == 1:
            # Parameters for FPS-R:TM
            periodA = 8
            periodB = 5
            periodSwitch = 6
            offsetInner = 15
            offsetOuter = 0
            finalRandSwitch = 1
            max_search_frames = 50

            # Call fpsr_tm_get_details
            output = fpsr_tm_get_details(frame, frame_multiplier, periodA, periodB, periodSwitch, offsetInner, offsetOuter, finalRandSwitch, lod, max_search_frames)
        elif algo == 2:
            # Parameters for FPS-R:QS
            baseWaveFreq = 0.012
            stream2FreqMult = 3.1
            quantLevelsMinMax = [4, 12]
            streamsOffset = [0, 76]
            quantOffsets = [10, 81]
            streamSwitchDur = 8
            stream1QuantDur = 10
            stream2QuantDur = 13
            finalRandSwitch = 1
            sine_lod_level = 4
            max_search_frames = 50

            # Call fpsr_qs_get_details
            output = fpsr_qs_get_details(frame, frame_multiplier, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, lod, max_search_frames)

        # Print the output for the current frame
        print(
            f"Frame {frame}: randVal {output.randVal:.6f}, randVal_previous {output.randVal_previous:.6f}, "
            f"has_changed {output.has_changed}, hold_progress {output.hold_progress:.6f},  "
            f"last_changed_frame {output.last_changed_frame}, next_changed_frame {output.next_changed_frame}"
            + (" (jumped)" if output.has_changed else "")
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


