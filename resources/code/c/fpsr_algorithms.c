// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/FPSR_Algorithm

/**
 * @file fpsr_algorithms.c
 * @brief Portable C implementation of FPS-R algorithms: Stacked Modulo (SM) and Quantised Switching (QS).
 * @details This file contains two stateless, frame-persistent randomization algorithms.
 * It uses a custom portable_rand() function to ensure deterministic and
 * consistent results across any platform.
 */

 #include <math.h> // For sin() and floor()
 #include <stdio.h> // For NULL
 
/**
 * A simple, portable pseudo-random number generator.
 * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */

// A simple, portable pseudo-random number generator that takes an integer seed.
// Different languages have different rand() implementations, so using a custom
// one like this ensures identical results on any platform.
float portable_rand(int seed) {
    // A common technique for a simple hash-like random number.
    // The large prime numbers are used to create a chaotic, unpredictable result.
    // The frac() part (or fmod(x, 1.0)) ensures the result is in the [0, 1) range.
    float result = sin((float)seed * 12.9898) * 43758.5453;
    return result - floor(result);
}
 
 
/******************************************************************************/
/* FPS-R: Stacked Modulo (SM)                                                 */
/******************************************************************************/

/**
 * @brief Generates a persistent random value that holds for a calculated duration.
 * @details This function uses a two-step process. First, it determines a random
 * "hold duration". Second, it generates a stable integer for that duration,
 * which is then used as a seed to produce the final, held random value.
 *
 * int frame: The current frame or time input.
 * int minHold: The minimum duration (in frames) for a value to hold.
 * int maxHold: The maximum duration (in frames) for a value to hold.
 * int reseedInterval: The fixed interval at which a new hold duration is calculated.
 * int seedInner: An offset for the random duration calculation to create unique sequences.
 * int seedOuter: An offset for the final value calculation to create unique sequences.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * return 
 *     when finalRandSwitch is 0: 
 *         An integer value representing the currently held frame 
 *         that remains constant for the hold duration.
 *     when finalRandSwitch is 1: 
 *         A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
float fpsr_sm(
    int frame, int minHold, int maxHold,
    int reseedInterval, int seedInner, int seedOuter)
{
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    float rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval));
    int holdDuration = (int)floor(minHold + rand_for_duration * (maxHold - minHold));

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // This value is constant for the entire duration of the hold.
    int held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value ---
    // Because the seed is stable, the final value is also stable.
    float fpsr_output = 0.0;
    if (finalRandSwitch) {
        // If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand(held_integer_state * 100000.0);
    } else {
        // If finalRandSwitch is false, we return the active stream value directly.
        fpsr_output = held_integer_state; 
    }
    return fpsr_output;
}
 
// Sample code to call the FPS-R:SM function
// Parameters
int frame = 100; // Replace with the current frame value
int minHoldFrames = 16; // probable minimum held period
int maxHoldFrames = 24; // maximum held period before cycling
int reseedFrames = 9; // inner mod cycle timing
int offsetInner = -41; // offsets the inner frame
int offsetOuter = 23; // offsets the outer frame
int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it

// Call the FPS-R:SM function
float randVal = 
    fpsr_sm(
        int(frame), minHoldFrames, maxHoldFrames, 
        reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
float randVal_previous = 
    fpsr_sm(
        int(frame-1), minHoldFrames, maxHoldFrames, 
        reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
int changed = 0;
if (randVal != randVal_previous) {
    changed = 1; // value has changed from the previous frame
}


/******************************************************************************/
/* FPS-R: Toggled Modulo (TM)                                                 */
/******************************************************************************/

/**
 * @brief Generates a persistent value that holds for a rhythmically toggled duration.
 * @details This function uses a deterministic switch to toggle the hold duration
 * between two fixed periods. This creates a predictable, rhythmic, or mechanical
 * "move-and-hold" pattern, as opposed to the organic randomness of SM.
 *
 * int frame: The current frame or time input.
 * int periodA: The first hold duration (in frames).
 * int periodB: The second hold duration (in frames).
 * int periodSwitch: The fixed interval at which the hold duration is toggled.
 * int seedInner: An offset for the toggle clock to de-sync it from the main clock.
 * int seedOuter: An offset for the main clock to create unique sequences.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * return 
 * when finalRandSwitch is 0: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is 1: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */
float fpsr_tm(
    int frame, int periodA, int periodB,
    int periodSwitch, int seedInner, int seedOuter,
    int finalRandSwitch)
{
    // --- 1. Determine the hold duration by toggling between two periods ---
    if (periodSwitch < 1) { periodSwitch = 1; } // Prevent division by zero.

    // The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    int inner_clock_frame = seedInner + frame;
    
    int holdDuration;
    // The ternary switch: toggle between periodA and periodB at a fixed rhythm.
    if ((inner_clock_frame % periodSwitch) < (periodSwitch * 0.5)) {
        holdDuration = periodA;
    } else {
        holdDuration = periodB;
    }

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // The "outer clock" is offset by seedOuter to create unique output sequences.
    int outer_clock_frame = seedOuter + frame;
    int held_integer_state = outer_clock_frame - (outer_clock_frame % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    float fpsr_output;
    if (finalRandSwitch) {
        // If true, apply the final randomisation hash.
        fpsr_output = portable_rand(held_integer_state);
    } else {
        // If false, return the raw integer state directly.
        fpsr_output = (float)held_integer_state; 
    }
    return fpsr_output;
}

// Sample code to call the FPS-R:TM function
// Parameters
int frame_tm = 100; // Replace with the current frame value
int period_A = 10; // The first hold duration
int period_B = 25; // The second hold duration
int switch_duration = 30; // The toggle happens every 30 frames
int offset_inner_tm = 15; // offsets the inner (toggle) clock
int offset_outer_tm = 0; // offsets the outer (hold) clock
int final_rand_switch_tm = 1; // 1 to apply the final randomisation step, 0 to skip it

// Call the FPS-R:TM function
float randVal_tm = 
    fpsr_tm(
        frame_tm, period_A, period_B, 
        switch_duration, offset_inner_tm, offset_outer_tm, final_rand_switch_tm);
float randVal_previous_tm = 
    fpsr_tm(
        frame_tm - 1, period_A, period_B, 
        switch_duration, offset_inner_tm, offset_outer_tm, final_rand_switch_tm);
int changed_tm = 0;
if (randVal_tm != randVal_previous_tm) {
    changed_tm = 1; // value has changed from the previous frame
}


/******************************************************************************/
/* FPS-R: Quantised Switching (QS)                                            */
/******************************************************************************/

/**
 * @brief Generates a flickering, quantised value by switching between two sine wave streams.
 * @details This function creates two separate, quantised sine waves and switches
 * between them at a fixed interval to create complex, glitch-like patterns.
 *
 * int frame: The current frame or time input.
 * float baseWaveFreq: The base frequency for the modulation wave of stream 1.
 * float stream2FreqMult: A multiplier for the second stream's frequency. If < 0, a default is used.
 * int quantLevelsMinMax: An array of two integers for the min and max quantisation levels.
 * int streamsOffset: An array of two integers to offset the frame for each stream.
 * int streamSwitchDur: The number of frames after which the streams switch. If < 1, a default is derived.
 * int stream1QuantDur: The duration for stream 1's quantisation switch. If < 1, a default is derived.
 * int stream2QuantDur: The duration for stream 2's quantisation switch. If < 1, a default is derived.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * return: A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
float fpsr_qs(
    int frame, float baseWaveFreq, float stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2],
    int streamSwitchDur, int stream1QuantDur, int stream2QuantDur,
    int finalRandSwitch)
{
    // --- 1. Set default durations if not provided ---
    // This pattern allows for optional parameters in a portable C-style.
    if (streamSwitchDur < 1) {
        streamSwitchDur = (int)floor((1.0 / baseWaveFreq) * 0.76);
    }
    if (stream1QuantDur < 1) {
        stream1QuantDur = (int)floor((1.0 / baseWaveFreq) * 1.2);
    }
    if (stream2QuantDur < 1) {
        stream2QuantDur = (int)floor((1.0 / baseWaveFreq) * 0.9);
    }
    // Ensure durations are at least 1 frame to prevent division by zero.
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate quantisation levels for each stream ---
    // The quantisation level itself switches halfway through its own duration cycle.
    int s1_quant_level;
    if ((streamsOffset[0] + frame) % stream1QuantDur < stream1QuantDur * 0.5) {
        s1_quant_level = quantLevelsMinMax[0];
    } else {
        s1_quant_level = quantLevelsMinMax[1];
    }

    int s2_quant_level;
    // Magic numbers are used to create more variation in the second stream's character.
    // Stream 2 uses these values as a multiplier of Stream 1's quantisation levels.
    const float STREAM2_QUANT_RATIO_MIN = 1.24;
    const float STREAM2_QUANT_RATIO_MAX = 0.66;
    if ((streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur * 0.5) {
        s2_quant_level = (int)floor(quantLevelsMinMax[0] * STREAM2_QUANT_RATIO_MIN);
    } else {
        s2_quant_level = (int)floor(quantLevelsMinMax[1] * STREAM2_QUANT_RATIO_MAX);
    }
    // Ensure quantisation levels are at least 1.
    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }


    // --- 3. Generate the two quantised sine wave streams ---
    float STREAM2_DEFAULT_FREQ_MULT = 3.7; // Default multiplier for stream 2.
    if (stream2FreqMult < 0) { stream2FreqMult = STREAM2_DEFAULT_FREQ_MULT; } 

    float stream1 = floor(sin((float)(streamsOffset[0] + frame) * baseWaveFreq) 
                        * s1_quant_level) / s1_quant_level;
    float stream2 = floor(sin((float)(streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) 
                        * s2_quant_level) / s2_quant_level;

    // --- 4. Switch between the two streams ---
    float active_stream_val = 0.0;
    if ((frame % streamSwitchDur) < streamSwitchDur / 2) {
        active_stream_val = stream1;
    } else {
        active_stream_val = stream2;
    }

    // --- 5. Hash the final output to create a random-looking value ---
    // The stepped sine wave output is converted to a large integer and used
    // as a seed to produce the final, held random value.
    float fpsr_output = 0.0;
    if (finalRandSwitch) {
        // If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand((int)(active_stream_val * 100000.0));
    } else {
        // If finalRandSwitch is false, we need to scale down the sine curve ranges (-1 to 1)
        // to 0 to 1 before we can return the active stream value directly.
        fpsr_output = 0.5 * active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1];
    }

     return fpsr_output;
}
 
// Sample code to call the FPS-R:QS function
// Parameters
int frame = 103; // Current frame number
float baseWaveFreq = 0.012; // Base frequency for the modulation wave of stream 1
float stream2freqMult = 3.1; // Multiplier for the second stream's frequency
int quantLevelsMinMax[2] = {12, 22}; // Min, Max quantisation levels for the two streams
int streamsOffset[2] = {0, 76}; // Offset for the two streams
int streamSwitchDur = 24; // Duration for switching streams in frames
int stream1QuantDur = 16; // Duration for the first stream's quantisation switch cycle in frames
int stream2QuantDur = 20; // Duration for the second stream's quantisation switch cycle in frames
int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it

float randVal = fpsr_qs(
    frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);

// another call to fpsr_qs for the previous frame
float randVal_previous = fpsr_qs(
    int(@Frame - 1), baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);

int changed = 0; // Variable to track if the value has changed
if (randVal != randVal_previous) {
    changed = 1; // Mark as changed if the value has changed from the previous frame
} 