# SPDX-License-Identifier: MIT — See LICENSE for full terms

'''
file: fpsr_algorithms.py
brief: Python implementation of FPS-R algorithms: Stacked Modulo (SM) and Quantised Switching (QS).
details: 
    This file contains two stateless, frame-persistent randomization algorithms.
    
    It uses a custom portable_rand() function to ensure deterministic and consistent results across any platform.
'''

import math

def portable_rand(seed):
    """
    A simple, portable pseudo-random number generator.
    Generates a deterministic float between 0.0 and 1.0 from an integer seed.
    
    :param seed: An integer used to generate the random number.
    :return: A pseudo-random float between 0.0 and 1.0.
    """
    result = math.sin(seed * 12.9898) * 43758.5453
    return result - math.floor(result)


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
        int frame: The current frame or time input.
        int minHold: The minimum duration (in frames) for a value to hold.
        int maxHold: The maximum duration (in frames) for a value to hold.
        int reseedInterval: The fixed interval at which a new hold duration is calculated.
        int seedInner: An offset for the random duration calculation to create unique sequences.
        int seedOuter: An offset for the final value calculation to create unique sequences.
        bool finalRandSwitch: A flag to enable/disable the final randomisation step.
    
    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw integer state value.
    """
    # --- 1. Calculate the random hold duration ---
    if reseedInterval < 1:
        reseedInterval = 1  # Prevent division by zero.

    rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval))
    holdDuration = math.floor(minHold + rand_for_duration * (maxHold - minHold))

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # This value is constant for the entire duration of the hold.
    held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration)

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    # Because the seed is stable, the final value is also stable.
    if finalRandSwitch:
        # If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand(held_integer_state)
    else:
        # If finalRandSwitch is false, we return the raw integer state directly.
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
use_final_random_sm = True # Set to False to bypass final randomization

# Call the FPS-R:SM function
randVal = fpsr_sm(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random_sm)
randVal_previous = fpsr_sm(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random_sm)

# Check if the value has changed
changed = 1 if randVal != randVal_previous else 0

print("--- Stacked Modulo (SM) Sample ---")
print(f'randVal_previous: {randVal_previous}')
print(f'randVal: {randVal}')
print(f'changed: {changed}\n')


"""
-------------------------------
FPS-R: Quantised Switching (QS)
-------------------------------
"""

def fpsr_qs(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, 
            streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch=True):
    """
    Generates a flickering, quantised value by switching between two sine wave streams.
    
    Args:
        int frame: The current frame or time input.
        float baseWaveFreq: The base frequency for the modulation wave of stream 1.
        float stream2FreqMult: A multiplier for the second stream's frequency. If < 0, a default is used.
        list[int] quantLevelsMinMax: A list of two integers for the min and max quantisation levels.
        list[int] streamsOffset: A list of two integers to offset the frame for each stream.
        int streamSwitchDur: The number of frames after which the streams switch. If < 1, a default is derived.
        int stream1QuantDur: The duration for stream 1's quantisation switch. If < 1, a default is derived.
        int stream2QuantDur: The duration for stream 2's quantisation switch. If < 1, a default is derived.
        bool finalRandSwitch: A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw stepped signal value.
    """
    # --- 1. Set default durations if not provided ---
    if streamSwitchDur < 1:
        streamSwitchDur = math.floor((1.0 / baseWaveFreq) * 0.76)
    if stream1QuantDur < 1:
        stream1QuantDur = math.floor((1.0 / baseWaveFreq) * 1.2)
    if stream2QuantDur < 1:
        stream2QuantDur = math.floor((1.0 / baseWaveFreq) * 0.9)
    
    # Ensure durations are at least 1 frame to prevent division by zero.
    streamSwitchDur = max(streamSwitchDur, 1)
    stream1QuantDur = max(stream1QuantDur, 1)
    stream2QuantDur = max(stream2QuantDur, 1)

    # --- 2. Calculate quantisation levels for each stream ---
    s1_quant_level = quantLevelsMinMax[0] if (streamsOffset[0] + frame) % stream1QuantDur < stream1QuantDur * 0.5 else quantLevelsMinMax[1]
    
    # Magic numbers are used to create more variation in the second stream's character.
    # Stream 2 uses these values as a multiplier of Stream 1's quantisation levels.
    STREAM2_QUANT_RATIO_MIN = 1.24
    STREAM2_QUANT_RATIO_MAX = 0.66
    if (streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur * 0.5:
        s2_quant_level = quantLevelsMinMax[0] * STREAM2_QUANT_RATIO_MIN
    else:
        s2_quant_level = quantLevelsMinMax[1] * STREAM2_QUANT_RATIO_MAX

    # Ensure quantisation levels are at least 1.
    s1_quant_level = max(math.floor(s1_quant_level), 1)
    s2_quant_level = max(math.floor(s2_quant_level), 1)

    # --- 3. Generate the two quantised sine wave streams ---
    STREAM2_DEFAULT_FREQ_MULT = 3.7
    if stream2FreqMult < 0:
        stream2FreqMult = STREAM2_DEFAULT_FREQ_MULT  # Default multiplier.

    stream1 = math.floor(math.sin((streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / s1_quant_level
    stream2 = math.floor(math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / s2_quant_level

    # --- 4. Switch between the two streams ---
    active_stream_val = stream1 if (frame % streamSwitchDur) < streamSwitchDur / 2 else stream2

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        # If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand(int(active_stream_val * 100000.0))
    else:
        # If finalRandSwitch is false, we return the active stream value directly.
        fpsr_output = active_stream_val
        
    return fpsr_output

# Sample code to call the FPS-R:QS function
# Parameters
frame = 103  # Current frame number
baseWaveFreq = 0.012  # Base frequency for the modulation wave of stream 1
stream2freqMult = 3.1  # Multiplier for the second stream's frequency
quantLevelsMinMax = [12, 22]  # Min, Max quantisation levels for the two streams
streamsOffset = [0, 76]  # Offset for the two streams
streamSwitchDur = 24  # Duration for switching streams in frames
stream1QuantDur = 16  # Duration for the first stream's quantisation switch cycle in frames
stream2QuantDur = 20  # Duration for the second stream's quantisation switch cycle in frames
use_final_random_qs = True # Set to False to bypass final randomization

# Call the FPS-R:QS function
randVal = fpsr_qs(
    frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random_qs
)

# Another call to fpsr_qs for the previous frame
randVal_previous = fpsr_qs(
    frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random_qs
)

# Check if the value has changed
changed = 1 if randVal != randVal_previous else 0

print("--- Quantised Switching (QS) Sample ---")
print(f'randVal_previous: {randVal_previous}')
print(f'randVal: {randVal}')
print(f'changed: {changed}')
