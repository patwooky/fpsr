// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/FPSR_Algorithm
//

// ⚠️ This C version of FPS-R is the canonical reference implementation.
// All language bindings and variants should conform to this behavior.
// Do not alter without updating downstream bindings or reference docs.

/**
 * @file fpsr_algorithms_glsl.frag
 * @brief GLSL implementation of FPS-R algorithms: 
 * Stacked Modulo (SM), Toggled Modulo (TM) and Quantised Switching (QS).
 * @details This file contains three stateless, frame-persistent randomisation algorithms.
 * It uses a custom portable_rand() function to ensure deterministic and
 * consistent results across any platform.
 */

/**
 * A simple, portable pseudo-random number generator.
 * @brief Generates a deterministic float between 0.0 and 1.0 from an integer seed.
 * @param int seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
// A simple, portable pseudo-random number generator that takes an integer seed.
// Different languages have different rand() implementations, so using a custom
// one like this ensures identical results on any platform.
float portable_rand(in int seed) {
    // A common technique for a simple hash-like random number.
    // The large prime numbers are used to create a chaotic, unpredictable result.
    float val = float(seed) * 12.9898;

    // --- FIX for GPU float precision ---
    // On many GPUs/GLSL versions, sin() loses precision or returns 0 for large inputs.
    // This causes the random value to "saturate" and become constant over time.
    // By using the mathematical property sin(x) = sin(x mod 2π), we can wrap the
    // input to sin() into a high-precision range [0, 2π], ensuring the result
    // remains stable and correct indefinitely.
    const float TWO_PI = 6.28318530718;
    val = mod(val, TWO_PI);

    float result = sin(val) * 43758.5453;
    return fract(result);
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
 * @param int frame The current frame or time input.
 * @param int minHold The minimum duration (in frames) for a value to hold.
 * @param int maxHold The maximum duration (in frames) for a value to hold.
 * @param int reseedInterval The fixed interval at which a new hold duration is calculated.
 * @param int seedInner An offset for the random duration calculation to create unique sequences.
 * @param int seedOuter An offset for the final value calculation to create unique sequences.
 * @param bool finalRandSwitch A flag that can turn off the final randomisation step.
 * @return 
 * when finalRandSwitch is false: 
 * An integer value representing the currently held frame 
 * that remains constant for the hold duration, returned as a float.
 * when finalRandSwitch is true: 
 * A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
float fpsr_sm(
    in int frame, in int minHold, in int maxHold,
    in int reseedInterval, in int seedInner, in int seedOuter, in bool finalRandSwitch)
{
    // Helper for integer modulo, as '%' is not supported for ints in GLSL ES 1.0
    int intMod = int(mod(float(frame), float(reseedInterval)));
    
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    float rand_for_duration = portable_rand(seedInner + frame - intMod);
    int holdDuration = int(floor(float(minHold) + rand_for_duration * float(maxHold - minHold)));

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // This value is constant for the entire duration of the hold.
    int held_integer_state = (seedOuter + frame) - int(mod(float(seedOuter + frame), float(holdDuration)));

    // --- 3. Use the stable state as a seed for the final random value ---
    // Because the seed is stable, the final value is also stable.
    float fpsr_output = 0.0;
    if (finalRandSwitch) {
        // If finalRandSwitch is true, we apply the final randomisation step.
        // NOTE: The original C code implicitly casts the float product to an int.
        // In GLSL, we must do this explicitly.
        fpsr_output = portable_rand(int(float(held_integer_state) * 100000.0));
    } else {
        // If finalRandSwitch is false, we return the active stream value directly.
        fpsr_output = float(held_integer_state); 
    }
    return fpsr_output;
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
 * @param int frame The current frame or time input.
 * @param int periodA The first hold duration (in frames).
 * @param int periodB The second hold duration (in frames).
 * @param int periodSwitch The fixed interval at which the hold duration is toggled.
 * @param int seedInner An offset for the toggle clock to de-sync it from the main clock.
 * @param int seedOuter An offset for the main clock to create unique sequences.
 * @param bool finalRandSwitch A flag that can turn off the final randomisation step.
 * @return 
 * when finalRandSwitch is false: 
 * An integer value representing the currently held frame state, returned as a float.
 * when finalRandSwitch is true: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */
float fpsr_tm(
    in int frame, in int periodA, in int periodB,
    in int periodSwitch, in int seedInner, in int seedOuter,
    in bool finalRandSwitch)
{
    // --- 1. Determine the hold duration by toggling between two periods ---
    if (periodSwitch < 1) { periodSwitch = 1; } // Prevent division by zero.

    // The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    int inner_clock_frame = seedInner + frame;
    
    int holdDuration;
    // The ternary switch: toggle between periodA and periodB at a fixed rhythm.
    if (int(mod(float(inner_clock_frame), float(periodSwitch))) < (periodSwitch / 2)) {
        holdDuration = periodA;
    } else {
        holdDuration = periodB;
    }

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // The "outer clock" is offset by seedOuter to create unique output sequences.
    int outer_clock_frame = seedOuter + frame;
    int held_integer_state = outer_clock_frame - int(mod(float(outer_clock_frame), float(holdDuration)));

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    float fpsr_output;
    if (finalRandSwitch) {
        // If true, apply the final randomisation hash.
        // NOTE: The original C code implicitly casts the float product to an int.
        // In GLSL, we must do this explicitly.
        fpsr_output = portable_rand(int(float(held_integer_state) * 100000.0));
    } else {
        // If false, return the raw integer state directly.
        fpsr_output = float(held_integer_state); 
    }
    return fpsr_output;
}


/******************************************************************************/
/* FPS-R: Quantised Switching (QS)                                            */
/******************************************************************************/

/**
 * @brief Generates a flickering, quantised value by switching between two sine wave streams.
 * @details This function creates two separate, quantised sine waves. For each stream,
 * a new random quantisation level is chosen from within the [min, max] range at a
 * set interval. The function then switches between these two streams to create
 * complex, glitch-like patterns.
 *
 * @param int frame The current frame or time input.
 * @param float baseWaveFreq The base frequency for the modulation wave of stream 1.
 * @param float stream2FreqMult A multiplier for the second stream's frequency.
 * @param ivec2 quantLevelsMinMax An ivec2 for the min and max quantisation levels.
 * @param ivec2 streamsOffset An ivec2 to offset the frame for each stream's sine wave.
 * @param ivec2 quantOffsets An ivec2 to offset the random quantisation selection for each stream.
 * @param int streamSwitchDur The number of frames after which the streams switch.
 * @param int stream1QuantDur The duration for which stream 1's random quantisation level is held.
 * @param int stream2QuantDur The duration for which stream 2's random quantisation level is held.
 * @param bool finalRandSwitch A flag that can turn off the final randomisation step.
 */
float fpsr_qs(
    in int frame, in float baseWaveFreq, in float stream2FreqMult,
    in ivec2 quantLevelsMinMax, in ivec2 streamsOffset, in ivec2 quantOffsets,
    in int streamSwitchDur, in int stream1QuantDur, in int stream2QuantDur,
    in bool finalRandSwitch)
{
    // --- 1. Set default durations if not provided ---
    if (streamSwitchDur < 1) { streamSwitchDur = int(floor((1.0 / baseWaveFreq) * 0.76)); }
    if (stream1QuantDur < 1) { stream1QuantDur = int(floor((1.0 / baseWaveFreq) * 1.2)); }
    if (stream2QuantDur < 1) { stream2QuantDur = int(floor((1.0 / baseWaveFreq) * 0.9)); }
    
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate random quantisation levels for each stream ---
    int quant_min = quantLevelsMinMax.x;
    int quant_max = quantLevelsMinMax.y;
    int quant_range = quant_max - quant_min + 1;

    // --- Stream 1 Quant Level ---
    int s1_quant_seed = (quantOffsets.x + frame) - int(mod(float(quantOffsets.x + frame), float(stream1QuantDur)));
    float s1_rand_for_quant = portable_rand(s1_quant_seed);
    int s1_quant_level = quant_min + int(floor(s1_rand_for_quant * float(quant_range)));

    // --- Stream 2 Quant Level ---
    int s2_quant_seed = (quantOffsets.y + frame) - int(mod(float(quantOffsets.y + frame), float(stream2QuantDur)));
    float s2_rand_for_quant = portable_rand(s2_quant_seed);
    int s2_quant_level = quant_min + int(floor(s2_rand_for_quant * float(quant_range)));

    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }

    // --- 3. Generate the two quantised sine wave streams ---
    if (stream2FreqMult < 0.0) { stream2FreqMult = 3.7; }

    float stream1 = floor(sin(float(streamsOffset.x + frame) * baseWaveFreq) * float(s1_quant_level)) / float(s1_quant_level);
    float stream2 = floor(sin(float(streamsOffset.y + frame) * baseWaveFreq * stream2FreqMult) * float(s2_quant_level)) / float(s2_quant_level);

    // --- 4. Switch between the two streams ---
    float active_stream_val = (int(mod(float(frame), float(streamSwitchDur))) < streamSwitchDur / 2) ? stream1 : stream2;

    // --- 5. Hash the final output or bypass ---
    float fpsr_output;
    if (finalRandSwitch) {
        fpsr_output = portable_rand(int(active_stream_val * 100000.0));
    } else {
        fpsr_output = 0.5 * active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1]
    }
    return fpsr_output;
}


/******************************************************************************/
/* Shadertoy Main Image Function                                              */
/******************************************************************************/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Use a const to select the demo mode.
    // 1 = Stacked Modulo (SM)
    // 2 = Toggled Modulo (TM)
    // 3 = Quantised Switching (QS)
    const int demoMode = 1;

    // Use iFrame for frame-perfect stepping
    int frame = iFrame;

    float randVal = 0.0;
    float randVal_previous = 0.0;
    
    if (demoMode == 1) {
        // --- DEMO 1: Stacked Modulo (SM) ---
        int minHoldFrames = 16;     // probable minimum held period
        int maxHoldFrames = 24;     // maximum held period before cycling
        int reseedFrames = 9;       // inner mod cycle timing
        int offsetInner = -41;      // offsets the inner frame
        int offsetOuter = 23;       // offsets the outer frame
        bool finalRandSwitch = true;// true to apply the final randomisation step, false to skip it

        randVal = fpsr_sm(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
        randVal_previous = fpsr_sm(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
    } else if (demoMode == 2) {
        // --- DEMO 2: Toggled Modulo (TM) ---
        int period_A = 10;          // The first hold duration
        int period_B = 25;          // The second hold duration
        int switch_duration = 30;   // The toggle happens every 30 frames
        int offset_inner = 15;      // offsets the inner (toggle) clock
        int offset_outer = 0;       // offsets the outer (hold) clock
        bool final_rand_switch = true; // true to apply the final randomisation step, false to skip it

        randVal = fpsr_tm(frame, period_A, period_B, switch_duration, offset_inner, offset_outer, final_rand_switch);
        randVal_previous = fpsr_tm(frame - 1, period_A, period_B, switch_duration, offset_inner, offset_outer, final_rand_switch);
    } else if (demoMode == 3) {
        // --- DEMO 3: Quantised Switching (QS) ---
        float baseWaveFreq = 0.012;     // Base frequency for the modulation wave of stream 1
        float stream2freqMult = 3.1;      // Multiplier for the second stream's frequency
        ivec2 quantLevelsMinMax = ivec2(4, 12); // Min, Max quantisation levels for the two streams
        ivec2 streamsOffset = ivec2(0, 76);     // Offset for the two streams
        ivec2 quantOffsets = ivec2(10, 81);   // Offset for the random quantisation selection
        int streamSwitchDur = 24;         // Duration for switching streams in frames
        int stream1QuantDur = 16;         // Duration for the first stream's quantisation switch cycle in frames
        int stream2QuantDur = 20;         // Duration for the second stream's quantisation switch cycle in frames
        bool finalRandSwitchQS = true;    // true to apply the final randomisation step, false to skip it
        
        randVal = fpsr_qs(frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitchQS);
        randVal_previous = fpsr_qs(frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitchQS);
    }


    // --- Visualization ---
    // Base color is a grayscale value from the random number
    vec3 backgroundColor = vec3(randVal);
    vec3 flashColor = vec3(1.0, 0.0, 0.0); // Red

    // Normalize coordinates and center them, correcting for aspect ratio
    vec2 uv = (2.0 * fragCoord.xy - iResolution.xy) / iResolution.y;
    
    // Define circle size (1/10th of the canvas height)
    float circleRadius = 0.1; 
    float circleMask = (length(uv) < circleRadius) ? 1.0 : 0.0;

    vec3 finalColor = backgroundColor;
    // If the value changed from the previous frame, flash the circle
    if (randVal != randVal_previous) {
        finalColor = mix(backgroundColor, flashColor, circleMask);
    }

    fragColor = vec4(finalColor, 1.0);
}