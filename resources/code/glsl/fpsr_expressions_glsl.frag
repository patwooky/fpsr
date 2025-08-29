// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/FPSR_Algorithm

/**
 * @file fpsr_expressions_glsl.frag
 * @brief GLSL one-line expression implementation of FPS-R algorithms: 
 * Stacked Modulo (SM), Toggled Modulo (TM) and Quantised Switching (QS).
 * @details This file contains three stateless, frame-persistent randomisation algorithms.
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
    float val = (float)seed * 12.9898;

    // --- FIX for float precision on GPUs and other platforms ---
    // On many platforms, sin() loses precision or returns 0 for large inputs.
    // This causes the random value to "saturate" and become constant over time.
    // By using the mathematical property sin(x) = sin(x mod 2π), we can wrap the
    // input to sin() into a high-precision range [0, 2π], ensuring the result
    // remains stable and correct indefinitely.
    const float TWO_PI = 6.28318530718f;
    val = fmod(val, TWO_PI);

    float result = sin(val) * 43758.5453f;
    
    // The fmod(result, 1.0) or (result - floor(result)) emulates GLSL's fract().
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
 * NOTE: The expression calls the included portable_rand(), 
 *       but you are free to use rand() from the host application.
 *
 * int frame: The current frame or time input.
 * int minHoldFrames: The minimum duration (in frames) for a value to hold.
 * int maxHoldFrames: The maximum duration (in frames) for a value to hold.
 * int reseedFrames: The fixed interval (number of frames) at which a new hold duration is calculated.
 * int seedInner: An offset for the random duration calculation to create unique sequences.
 * int seedOuter: An offset for the final value calculation to create unique sequences.
 * return 
 *     A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
 
// Sample parameter values to call the FPS-R:SM expression with
int frame = 100; // Replace with the current frame value
int minHoldFrames = 16; // probable minimum held period
int maxHoldFrames = 24; // maximum held period before cycling
int reseedFrames = 9; // inner mod cycle timing
int seedInner = -41; // offsets the inner frame
int seedOuter = 23; // offsets the outer frame

// Call the FPS-R:SM expression
float fpsr_sm_expression = portable_rand(
    (seedOuter + frame) - ((seedOuter + frame) % (
        minHoldFrames + floor(
            portable_rand(
                (seedInner + frame) - ((seedInner + frame) % reseedFrames)
            ) * (maxHoldFrames - minHoldFrames)
        )
    ))
);


/******************************************************************************/
/* FPS-R: Toggled Modulo (TM)                                                 */
/******************************************************************************/

/**
 * @brief Generates a persistent value that holds for a rhythmically toggled duration.
 * @details This function uses a deterministic switch to toggle the hold duration
 * between two fixed periods. This creates a predictable, rhythmic, or mechanical
 * "move-and-hold" pattern, as opposed to the organic randomness of SM.
 * NOTE: The expression calls the included portable_rand(), 
 *       but you are free to use rand() from the host application.
 *
 * int frame: The current frame or time input.
 * int period_A: The first hold duration (in frames).
 * int period_B: The second hold duration (in frames).
 * int periodSwitch: The fixed interval at which the hold duration is toggled.
 * int seedInner: An offset for the toggle clock to de-sync it from the main clock.
 * int seedOuter: An offset for the main clock to create unique sequences.
 * return 
 * when finalRandSwitch is 0: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is 1: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */

// Sample parameter values to call the FPS-R:TM expression with
int frame = 100; // Replace with the current frame value
int period_A = 10; // The first hold duration
int period_B = 25; // The second hold duration
int periodSwitch = 30; // The toggle happens every 30 frames
int seedInner = 15; // offsets the inner (toggle) clock
int seedOuter = 0; // offsets the outer (hold) clock

// Call the FPS-R:TM expression
float fpsr_tm_expression = portable_rand(
    (seedOuter + frame) - ((seedOuter + frame) % (
        ((seedInner + frame) % periodSwitch < periodSwitch * 0.5
        ) ? period_A : period_B
    ))
);


/******************************************************************************/
/* FPS-R: Quantised Switching (QS)                                            */
/******************************************************************************/

/**
 * @brief The nature of QS does not support a single line implementation.
 * Instead, it requires a more complex function with multiple parameters.
 */
