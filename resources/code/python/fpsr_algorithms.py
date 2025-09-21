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

def portable_rand(seed):
    """
    A simple, portable pseudo-random number generator.
    Generates a deterministic float between 0.0 and 1.0 from an integer seed.
    Different languages have different rand() implementations, so using a custom
    one like this ensures identical results on any platform.

    Args:
        seed (int): An integer used to generate the random number.

    Returns:
        float: A pseudo-random float between 0.0 and 1.0.
    """
    # A common technique for a simple hash-like random number.
    # The large prime numbers are used to create a chaotic, unpredictable result.
    val = float(seed) * 12.9898
    
    # --- FIX for float precision on GPUs and other platforms ---
    # By using the mathematical property sin(x) = sin(x mod 2π), we can wrap the
    # input to sin() into a high-precision range, ensuring the result
    # remains stable and correct indefinitely.
    # Python's math.fmod is the C equivalent, and math.pi is available.
    val = math.fmod(val, 2 * math.pi)

    result = math.sin(val) * 43758.5453
    
    # Python's equivalent of frac()
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
        fpsr_output = portable_rand(held_integer_state * 100000)  # integer seed
    else:
        # If finalRandSwitch is false, we return the raw integer state directly.
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

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
    
    # The ternary switch: toggle between periodA and periodB at a fixed rhythm.
    if int(inner_clock_frame % periodSwitch) < (periodSwitch // 2):
        holdDuration = periodA
    else:
        holdDuration = periodB

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # The "outer clock" is offset by seedOuter to create unique output sequences.
    outer_clock_frame = seedOuter + frame
    held_integer_state = outer_clock_frame - (outer_clock_frame % holdDuration)

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # If true, apply the final randomisation hash.
        fpsr_output = portable_rand(held_integer_state * 100000)  # integer seed
    else:
        # If false, return the raw integer state directly.
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

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
    s1_quant_seed = (quantOffsets[0] + frame) - ((quantOffsets[0] + frame) % stream1QuantDur)
    s1_rand_for_quant = portable_rand(s1_quant_seed)
    s1_quant_level = quant_min + math.floor(s1_rand_for_quant * quant_range)

    # --- Stream 2 Quant Level ---
    s2_quant_seed = (quantOffsets[1] + frame) - ((quantOffsets[1] + frame) % stream2QuantDur)
    s2_rand_for_quant = portable_rand(s2_quant_seed)
    s2_quant_level = quant_min + math.floor(s2_rand_for_quant * quant_range)

    s1_quant_level = max(s1_quant_level, 1)
    s2_quant_level = max(s2_quant_level, 1)

    # --- 3. Generate the two quantised sine wave streams ---
    if stream2FreqMult < 0: stream2FreqMult = 3.7

    stream1 = math.floor((math.sin((streamsOffset[0] + frame) * baseWaveFreq) / 2.0 + 0.5) * s1_quant_level) / s1_quant_level
    stream2 = math.floor((math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) / 2.0 + 0.5) * s2_quant_level) / s2_quant_level

    # --- 4. Switch between the two streams ---
    active_stream_val = stream1 if int(frame % streamSwitchDur) < int(streamSwitchDur / 2.0) else stream2

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        # If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand(int(active_stream_val * 100000))  # integer seed
    else:
        # If finalRandSwitch is false, we must scale the sine curve ranges (-1 to 1)
        # to 0 to 1 before we can return the active stream value.
        fpsr_output = 0.5 * active_stream_val + 0.5
        
    return fpsr_output

# end of fpsr_qs function


def main():
    # algorithms: 0 - sm, 1 - tm, 2 - qs
    algo = 0  # Change this value to 0, 1, or 2 to test different algorithms
    algo_name = ["SM", "TM", "QS"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103]  # starting frames for each algorithm
    num_frames = 20  # run a loop of x frames to demonstrate changes
    
    # create main for loop to demonstrate changes
    for loop_frame in range(num_frames):
        # printf("Frame %d\n", i);
    
        frame = loop_frame + start_frames[algo]  # starting frame for the selected algorithm
        randVal = 0.0  # variable to hold the random value output
        randVal_previous = 0.0  # variable to hold the previous frame's random value
        changed = 0  # Variable to track if the value has changed

        if algo == 0:
            # Sample code to call the FPS-R:SM function
            # Parameters
            # int frame = 90; // Replace with the current frame value
            minHoldFrames = 10  # probable minimum held period
            maxHoldFrames = 13  # maximum held period before cycling
            reseedFrames = 5    # inner mod cycle timing
            offsetInner = -34   # offsets the inner frame
            offsetOuter = 22    # offsets the outer frame
            finalRandSwitch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:SM function        
            # call to fpsr_sm for the current frame
            randVal = fpsr_sm(
                frame, minHoldFrames, maxHoldFrames, 
                reseedFrames, offsetInner, offsetOuter, finalRandSwitch)
            # another call to fpsr_sm for the previous frame
            randVal_previous = fpsr_sm(
                frame - 1, minHoldFrames, maxHoldFrames, 
                reseedFrames, offsetInner, offsetOuter, finalRandSwitch)
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
            # sample output for each frame in the loop
            # printf("Frame %d: randVal %f, randVal_previous %f, changed %d\n", 
            #     loop_frame, randVal, randVal_previous, changed)
        
        elif algo == 1:
            # Sample code to call the FPS-R:TM function
            # Parameters
            # int frame = 100; // Replace with the current frame value
            period_A = 10  # The first hold duration
            period_B = 16  # The second hold duration
            periodSwitch = 9  # The toggle happens every 30 frames
            offset_inner = 4  # offsets the inner (toggle) clock
            offset_outer = 0  # offsets the outer (hold) clock
            final_rand_switch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:TM function
            # call to fpsr_tm for the current frame
            randVal = fpsr_tm(
                frame, period_A, period_B, 
                periodSwitch, offset_inner, offset_outer, final_rand_switch)
            # another call to fpsr_tm for the previous frame
            randVal_previous = fpsr_tm(
                frame - 1, period_A, period_B, 
                periodSwitch, offset_inner, offset_outer, final_rand_switch)
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
            # sample output for each frame in the loop
            # printf("Frame %d: randVal %f, randVal_previous %f, changed %d\n", 
            #     loop_frame, randVal, randVal_previous, changed)
        
        elif algo == 2:
            # Sample code to call the FPS-R:QS function
            # Parameters
            # int frame = 103; // Current frame number
            baseWaveFreq = 0.012  # Base frequency for the modulation wave of stream 1
            stream2freqMult = 3.1  # Multiplier for the second stream's frequency
            quantLevelsMinMax = [4, 12]  # Min, Max quantisation levels for the two streams
            streamsOffset = [0, 72]  # Offset for the two streams
            quantOffsets = [9, 81]  # Offset for the random quantisation selection
            streamSwitchDur = 11  # Duration for switching streams in frames
            stream1QuantDur = 13  # Duration for the first stream's quantisation switch cycle in frames
            stream2QuantDur = 20  # Duration for the second stream's quantisation switch cycle in frames
            finalRandSwitch = 1  # 1 to apply the final randomisation step, 0 to skip it
            
            # call to fpsr_qs for the current frame
            randVal = fpsr_qs(
                frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch)
            # another call to fpsr_qs for the previous frame
            randVal_previous = fpsr_qs(
                frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch)
            changed = 0  # Variable to track if the value has changed
            if randVal != randVal_previous:
                changed = 1  # Mark as changed if the value has changed from the previous frame
            # sample output for each frame in the loop
            # printf("Frame %d: randVal %f, randVal_previous %f, changed %d\n", 
            #     loop_frame, randVal, randVal_previous, changed)
        
        # print per-frame result, appending "(jumped)" if changed
        print("Frame %d: randVal %f, randVal_previous %f, changed %d " % (frame, randVal, randVal_previous, changed), end="")
        if changed:
            print("(jumped)", end="")
        print()


if __name__ == "__main__":
    main()