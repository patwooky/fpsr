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

Generates pseudo-random values that holds for deterministic but random intervals to create seemingly unpredictable patterns.

This function uses a two-step process:
1. It determines a random "hold duration".
2. It generates a stable integer for that duration, which is then used as a seed 
   to produce the final, held random value.

Parameters:
- frame: The current frame or time input.
- minHold: The minimum duration (in frames) for a value to hold.
- maxHold: The maximum duration (in frames) for a value to hold.
- reseedInterval: The fixed interval at which a new hold duration is calculated.
- seedInner: An offset for the random duration calculation to create unique sequences.
- seedOuter: An offset for the final value calculation to create unique sequences.

Returns:
- A pseudo-random float value between 0.0 and 1.0 that remains constant for held durations.
"""

def fpsr_sm(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter):
    """
    Produces a pseudo-random value that persists across multiple frames, held for a calculated duration.
    The hold timing varies over time, driven by deterministic interference between reseeded modular rhythms.
    This method mimics structured hesitation and twitch-like behavior—creating motion that feels deliberate without relying on state or memory.
    
    :param frame: The current frame or time input.
    :param minHold: The minimum duration (in frames) for a value to hold.
    :param maxHold: The maximum duration (in frames) for a value to hold.
    :param reseedInterval: The fixed interval at which a new hold duration is calculated.
    :param seedInner: An offset for the random duration calculation to create unique sequences.
    :param seedOuter: An offset for the final value calculation to create unique sequences.
    :return: A float value between 0.0 and 1.0 that remains constant for the hold duration.
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

    # --- 3. Use the stable state as a seed for the final random value ---
    # Because the seed is stable, the final value is also stable.
    fpsr_output = portable_rand(held_integer_state)

    return fpsr_output

# Sample code to call the function
# Parameters
frame = 100  # Replace with the current frame value
minHoldFrames = 16  # probable minimum held period
maxHoldFrames = 24  # maximum held period before cycling
reseedFrames = 9    # inner mod cycle timing
offsetInner = -41   # offsets the inner frame
offsetOuter = 23    # offsets the outer frame

# Call the FPS-R:SM function
randVal = fpsr_sm(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter)
randVal_previous = fpsr_sm(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter)

# Check if the value has changed
changed = 1 if randVal != randVal_previous else 0

print(f'randVal_previous: {randVal_previous}')
print(f'randVal: {randVal}')
print(f'changed: {changed}')


"""
-------------------------------
FPS-R: Quantised Switching (QS)
-------------------------------

Produces a pseudo-random value by alternating between two quantised sine wave streams.
Each stream evolves independently with its own rhythm and quantisation resolution.
The output switches between streams at deterministic intervals, creating structured, flicker-like motion that appears chaotic yet follows repeatable modulation logic.

Parameters:
- frame: The current frame or time input.
- baseWaveFreq: The base frequency for the modulation wave of stream 1.
- stream2FreqMult: A multiplier for the second stream's frequency. 
                    If < 0, a default is used.
- quantLevelsMinMax: A list of two integers for the min and max quantisation levels.
- streamsOffset: A list of two integers to offset the frame for each stream.
- streamSwitchDur: The number of frames after which the streams switch. 
                    If < 1, a default is derived.
- stream1QuantDur: The duration for stream 1's quantisation switch. 
                    If < 1, a default is derived.
- stream2QuantDur: The duration for stream 2's quantisation switch. 
                    If < 1, a default is derived.

Returns:
- A pseudo-random float value between 0.0 and 1.0 that remains constant for held durations.
"""

def fpsr_qs(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, 
            streamSwitchDur, stream1QuantDur, stream2QuantDur):
    """
    Generates a flickering, quantised value by switching between two sine wave streams.
    
    :param frame: The current frame or time input.
    :param baseWaveFreq: The base frequency for the modulation wave of stream 1.
    :param stream2FreqMult: A multiplier for the second stream's frequency. If < 0, a default is used.
    :param quantLevelsMinMax: A list of two integers for the min and max quantisation levels.
    :param streamsOffset: A list of two integers to offset the frame for each stream.
    :param streamSwitchDur: The number of frames after which the streams switch. If < 1, a default is derived.
    :param stream1QuantDur: The duration for stream 1's quantisation switch. If < 1, a default is derived.
    :param stream2QuantDur: The duration for stream 2's quantisation switch. If < 1, a default is derived.
    :return: A pseudo-random float value between 0.0 and 1.0.
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
    s1_quant_level = quantLevelsMinMax[0] if (streamsOffset[0] + frame) % stream1QuantDur < stream1QuantDur / 2 else quantLevelsMinMax[1]
    s2_quant_level = quantLevelsMinMax[0] * 1.24 if (streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur / 2 else quantLevelsMinMax[1] * 0.66
    
    # Ensure quantisation levels are at least 1.
    s1_quant_level = max(math.floor(s1_quant_level), 1)
    s2_quant_level = max(math.floor(s2_quant_level), 1)

    # --- 3. Generate the two quantised sine wave streams ---
    if stream2FreqMult < 0:
        stream2FreqMult = 3.7  # Default multiplier.

    stream1 = math.floor(math.sin((streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / s1_quant_level
    stream2 = math.floor(math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / s2_quant_level

    # --- 4. Switch between the two streams ---
    active_stream_val = stream1 if (frame % streamSwitchDur) < streamSwitchDur / 2 else stream2

    # --- 5. Hash the final output to create a random-looking value ---
    fpsr_output = portable_rand(int(active_stream_val * 100000.0))
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

# Call the FPS-R:QS function
randVal = fpsr_qs(
    frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur
)

# Another call to fpsr_qs for the previous frame
randVal_previous = fpsr_qs(
    frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur
)

# Check if the value has changed
changed = 1 if randVal != randVal_previous else 0