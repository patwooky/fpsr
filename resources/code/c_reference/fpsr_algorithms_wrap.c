// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/FPSR_Algorithm

/**
 * @file fpsr_wrapped.c
 * @brief This file demonstrates a wrapper-based approach for getting rich metadata
 * from the core FPS-R algorithms.
 * @details This implementation separates the pure, stateless algorithms from the
 * functions that gather detailed metadata. The wrapper functions perform a robust,
 * two-phase search (exponential probe + binary search) to populate the FPSR_Output struct.
 * This method is highly efficient and avoids "false positive" value collisions.
 */

#include <math.h> // For sin() and floor() (double versions)
#include <stdio.h> // For NULL

// Define the global inflation factor for absolute determinism
// This factor scales the 'frame' input to a high-resolution integer timeline.
// All time-based parameters (durations, offsets) must be scaled by this factor
// internally within the base algorithms. Frequencies must be deflated by it.
#define FPSR_INFLATION_FACTOR 100000000.0 // 10^8 for 8 decimal places of precision

// Define the sizes of the sine lookup tables for different LODs
#define SINE_LUT_SIZE_100 100
#define SINE_LUT_SIZE_500 500
#define SINE_LUT_SIZE_1000 1000
#define SINE_LUT_SIZE_4096 4096 // Highest precision default

// Global constant for 2*PI (double precision)
const double TWO_PI = 6.28318530718;

// Global sine lookup tables
static double _sine_lut_100[SINE_LUT_SIZE_100];
static double _sine_lut_500[SINE_LUT_SIZE_500];
static double _sine_lut_1000[SINE_LUT_SIZE_1000];
static double _sine_lut_4096[SINE_LUT_SIZE_4096]; // Highest precision default

// Flag to track if LUTs are initialized
static int _luts_initialized = 0;

// Function to initialize all sine lookup tables
// THIS FUNCTION MUST BE CALLED ONCE AT PROGRAM STARTUP!
void initialize_sine_luts() {
    if (_luts_initialized) return; // Only initialize once

    for (int i = 0; i < SINE_LUT_SIZE_100; ++i) {
        _sine_lut_100[i] = sin((double)i / SINE_LUT_SIZE_100 * TWO_PI);
    }
    for (int i = 0; i < SINE_LUT_SIZE_500; ++i) {
        _sine_lut_500[i] = sin((double)i / SINE_LUT_SIZE_500 * TWO_PI);
    }
    for (int i = 0; i < SINE_LUT_SIZE_1000; ++i) {
        _sine_lut_1000[i] = sin((double)i / SINE_LUT_SIZE_1000 * TWO_PI);
    }
    for (int i = 0; i < SINE_LUT_SIZE_4096; ++i) {
        _sine_lut_4096[i] = sin((double)i / SINE_LUT_SIZE_4096 * TWO_PI);
    }
    _luts_initialized = 1;
}

// Helper function to get sine value from a specific LUT with linear interpolation
double _get_sine_from_lod_lut(double phase, int lut_size, const double* lut_array) {
    if (!_luts_initialized) {
        // Fallback or error if LUTs not initialized.
        // For absolute determinism, this should ideally not happen in production.
        // For now, we'll fall back to standard sin() with a warning.
        fprintf(stderr, "WARNING: Sine LUTs not initialized. Falling back to sin(). Call initialize_sine_luts() once.\n");
        return sin(phase); 
    }

    // Wrap phase to 0 to 2*PI range
    phase = fmod(phase, TWO_PI);
    if (phase < 0) phase += TWO_PI; // Ensure positive for fmod results

    // Map phase to LUT index range
    double fractional_index = phase / TWO_PI * lut_size;

    // Get integer part and fractional part
    int index1 = (int)floor(fractional_index);
    double frac = fractional_index - index1;

    // Handle wrap-around for index2 (last point wraps to first)
    int index2 = (index1 + 1) % lut_size;

    // Linear interpolation
    return lut_array[index1] * (1.0 - frac) + lut_array[index2] * frac;
}

/******************************************************************************/
/* Core Components (Struct and portable_rand)                                 */
/******************************************************************************/

// A simple, portable pseudo-random number generator that takes an integer seed.
// Internal calculations use double for higher precision, result is float.
// Now uses the global sine lookup table for absolute determinism.
float portable_rand(int seed) {
    double val = (double)seed * 12.9898; // Use double literal
    val = fmod(val, TWO_PI); // This ensures 'val' is in [0, 2*PI)
    if (val < 0) val += TWO_PI; // Ensure positive for fmod results

    // Use the highest precision LUT for sine calculation for absolute determinism
    // This ensures portable_rand's output is consistent across all LOD choices for QS.
    double result_sin = _get_sine_from_lod_lut(val, SINE_LUT_SIZE_4096, _sine_lut_4096); 
    double result = result_sin * 43758.5453;
    return (float)(result - floor(result)); // Use floor (double version), cast to float for return
}

/******************************************************************************/
/* FPS-R Output Structure                                                     */
/******************************************************************************/
/* * This structure holds the output of the FPS-R algorithms.
* The LOD (Level of Detail) determines the computational overhead and the amount of information returned.
* This structure is designed to be flexible and can be extended in the future.
*
* Different LODs will return different sets of fields:
* - LOD 0: randVal
* - LOD 1: randVal, has_changed
* - LOD 2: randVal, has_changed, hold_progress, last_changed_frame, next_changed_frame,
* randVal_next_changed_frame, randStreams[2], selected_stream (for QS algorithm)
* Note: All fields will be set to 0 if the LOD is not applicable.
*
* The fields are:
* float randVal: LOD 0, 1, 2. The random value generated by the FPS-R algorithm.
* int has_changed: LOD 1, 2. A flag indicating whether randVal has changed from the previous frame.
* float hold_progress: LOD 2. The progress of the hold duration, normalised to [0, 1].
* int last_changed_frame: LOD 2. The precise frame (integer) when the random value last changed.
* int next_changed_frame: LOD 2. The precise frame (integer) when the random value will next change.
* float randVal_next_changed_frame: LOD 2. The value that the algorithm will jump to at next_changed_frame.
* double randStreams[2]: LOD 2. (Exclusive to QS) The raw values of stream1_double and stream2_double.
* int selected_stream: LOD 2. (Exclusive to QS) The index of the stream (0 for stream1, 1 for stream2) that was selected by the algorithm.
* */
typedef struct {
    float randVal; 
    int has_changed;
    float hold_progress;
    int last_changed_frame; 
    int next_changed_frame; 
    float randVal_next_changed_frame; 
    // New fields for QS details
    double randStreams[2]; 
    int selected_stream;   
} FPSR_Output;

// Internal struct for _fpsr_qs_base to return multiple values
typedef struct {
    float randVal;
    double stream1_val;
    double stream2_val;
    int selected_stream_idx; // 0 for stream1, 1 for stream2
} _FPSR_QS_Base_Output;

/******************************************************************************/
/* Untouched, Low-Level FPS-R Algorithms                                      */
/******************************************************************************/
// These functions remain pure, returning only a single float value.

float _fpsr_sm_base(
    double frame_input_from_wrapper, // frame is now double from wrapper
    int minHold, int maxHold,
    int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch)
{
    // Convert scaled double frame to large integer for pure integer math
    // This ensures bit-for-bit determinism for all modulo operations.
    int int_frame = (int)floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR); 

    // Scale all time-based integer parameters to match the int_frame resolution
    // These are now 'internal_' variables and replace the direct use of input parameters
    int internal_minHold = (int)floor(minHold * FPSR_INFLATION_FACTOR);
    int internal_maxHold = (int)floor(maxHold * FPSR_INFLATION_FACTOR);
    int internal_reseedInterval = (int)floor(reseedInterval * FPSR_INFLATION_FACTOR);
    int internal_seedInner = (int)floor(seedInner * FPSR_INFLATION_FACTOR);
    int internal_seedOuter = (int)floor(seedOuter * FPSR_INFLATION_FACTOR);


    if (internal_reseedInterval < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { internal_reseedInterval = (int)floor(1.0 * FPSR_INFLATION_FACTOR); }
    // Use int_frame for modulo operations
    double rand_for_duration_seed_double = (double)internal_seedInner + int_frame - (int_frame % internal_reseedInterval);
    double rand_for_duration = portable_rand((int)floor(rand_for_duration_seed_double));
    
    int holdDuration = (int)floor(internal_minHold + rand_for_duration * (internal_maxHold - internal_minHold));
    if (holdDuration < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { holdDuration = (int)floor(1.0 * FPSR_INFLATION_FACTOR); }
    
    // Use int_frame for modulo operations
    double held_integer_state_double = (double)internal_seedOuter + int_frame - ((double)(internal_seedOuter + int_frame) / holdDuration - floor((double)(internal_seedOuter + int_frame) / holdDuration)) * holdDuration; // Explicit integer modulo for absolute determinism
    int held_integer_state = (int)floor(held_integer_state_double);
    
    if (finalRandSwitch) {
        return portable_rand(held_integer_state); // Seed is already large integer
    }
    return (float)held_integer_state;
}

float _fpsr_tm_base(
    double frame_input_from_wrapper, // frame is now double from wrapper
    int periodA, int periodB,
    int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch)
{
    // Convert scaled double frame to large integer for pure integer math
    int int_frame = (int)floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR);

    // Scale all time-based integer parameters to match the int_frame resolution
    // These are now 'internal_' variables and replace the direct use of input parameters
    int internal_periodA = (int)floor(periodA * FPSR_INFLATION_FACTOR);
    int internal_periodB = (int)floor(periodB * FPSR_INFLATION_FACTOR);
    int internal_periodSwitch = (int)floor(periodSwitch * FPSR_INFLATION_FACTOR);
    int internal_seedInner = (int)floor(seedInner * FPSR_INFLATION_FACTOR);
    int internal_seedOuter = (int)floor(seedOuter * FPSR_INFLATION_FACTOR);

    if (internal_periodSwitch < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { internal_periodSwitch = (int)floor(1.0 * FPSR_INFLATION_FACTOR); }
    double inner_clock_frame_double = (double)internal_seedInner + int_frame; // Use int_frame
    
    int holdDuration;
    // Use int_frame for modulo operations
    if ((int_frame % internal_periodSwitch) < (internal_periodSwitch / 2)) { // Pure integer modulo
        holdDuration = internal_periodA;
    } else {
        holdDuration = internal_periodB;
    }
    if (holdDuration < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { holdDuration = (int)floor(1.0 * FPSR_INFLATION_FACTOR); }
    
    double outer_clock_frame_double = (double)internal_seedOuter + int_frame; // Use int_frame
    // Use int_frame for modulo operations
    double held_integer_state_double = outer_clock_frame_double - ((double)(internal_seedOuter + int_frame) / holdDuration - floor((double)(internal_seedOuter + int_frame) / holdDuration)) * holdDuration; // Explicit integer modulo for absolute determinism
    int held_integer_state = (int)floor(held_integer_state_double);

    if (finalRandSwitch) {
        return portable_rand(held_integer_state); // Seed is already large integer
    }
    return (float)held_integer_state;
}

_FPSR_QS_Base_Output _fpsr_qs_base( // Return type changed to _FPSR_QS_Base_Output
    double frame_input_from_wrapper, // frame is now double from wrapper
    float baseWaveFreq, float stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch,
    int sine_lod_level) // New parameter for sine LOD
{
    _FPSR_QS_Base_Output output = {0}; // Initialize output struct
    
    // Convert scaled double frame to large integer for pure integer math
    int int_frame = (int)floor(frame_input_from_wrapper * FPSR_INFLATION_FACTOR);
    
    // Scale all time-based integer parameters to match the int_frame resolution
    // These are now 'internal_' variables and replace the direct use of input parameters
    int internal_streamSwitchDur = (int)floor(streamSwitchDur * FPSR_INFLATION_FACTOR);
    int internal_stream1QuantDur = (int)floor(stream1QuantDur * FPSR_INFLATION_FACTOR);
    int internal_stream2QuantDur = (int)floor(stream2QuantDur * FPSR_INFLATION_FACTOR);
    int internal_streamsOffset_0 = (int)floor(streamsOffset[0] * FPSR_INFLATION_FACTOR);
    int internal_streamsOffset_1 = (int)floor(streamsOffset[1] * FPSR_INFLATION_FACTOR);
    int internal_quantOffsets_0 = (int)floor(quantOffsets[0] * FPSR_INFLATION_FACTOR);
    int internal_quantOffsets_1 = (int)floor(quantOffsets[1] * FPSR_INFLATION_FACTOR);

    if (internal_streamSwitchDur < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { internal_streamSwitchDur = (int)floor(1.0 * FPSR_INFLATION_FACTOR); } 
    if (internal_stream1QuantDur < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { internal_stream1QuantDur = (int)floor(1.0 * FPSR_INFLATION_FACTOR); } 
    if (internal_stream2QuantDur < (int)floor(1.0 * FPSR_INFLATION_FACTOR)) { internal_stream2QuantDur = (int)floor(1.0 * FPSR_INFLATION_FACTOR); } 

    int quant_min = quantLevelsMinMax[0];
    int quant_max = quantLevelsMinMax[1];
    int quant_range = quant_max - quant_min + 1;
    if (quant_range < 1) quant_range = 1;

    // Use int_frame for modulo operations
    double s1_quant_seed_double = (double)internal_quantOffsets_0 + int_frame - (int_frame % internal_stream1QuantDur);
    int s1_quant_level = quant_min + (int)floor(portable_rand((int)floor(s1_quant_seed_double)) * quant_range);
    
    double s2_quant_seed_double = (double)internal_quantOffsets_1 + int_frame - (int_frame % internal_stream2QuantDur);
    int s2_quant_level = quant_min + (int)floor(portable_rand((int)floor(s2_quant_seed_double)) * quant_range);
    
    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }

    if (stream2FreqMult <= 0) { stream2FreqMult = 3.7f; } // Still a float input
    
    // Frequencies must be deflated to match the inflated int_frame resolution
    double deflated_baseWaveFreq = (double)baseWaveFreq / FPSR_INFLATION_FACTOR;
    // stream2FreqMult is a multiplier to deflated_baseWaveFreq
    double deflated_stream2FreqMult_applied = deflated_baseWaveFreq * (double)stream2FreqMult; 

    double stream1_raw_sine, stream2_raw_sine; // Declare variables for raw sine values

    // Select sine generation method based on sine_lod_level
    switch (sine_lod_level) {
        case 0: // Direct sin() call (double precision)
            stream1_raw_sine = sin(((double)internal_streamsOffset_0 + int_frame) * deflated_baseWaveFreq);
            stream2_raw_sine = sin(((double)internal_streamsOffset_1 + int_frame) * deflated_stream2FreqMult_applied);
            break;
        case 1: // LUT 100 samples
            stream1_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_0 + int_frame) * deflated_baseWaveFreq, SINE_LUT_SIZE_100, _sine_lut_100);
            stream2_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_1 + int_frame) * deflated_stream2FreqMult_applied, SINE_LUT_SIZE_100, _sine_lut_100);
            break;
        case 2: // LUT 500 samples
            stream1_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_0 + int_frame) * deflated_baseWaveFreq, SINE_LUT_SIZE_500, _sine_lut_500);
            stream2_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_1 + int_frame) * deflated_stream2FreqMult_applied, SINE_LUT_SIZE_500, _sine_lut_500);
            break;
        case 3: // LUT 1000 samples
            stream1_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_0 + int_frame) * deflated_baseWaveFreq, SINE_LUT_SIZE_1000, _sine_lut_1000);
            stream2_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_1 + int_frame) * deflated_stream2FreqMult_applied, SINE_LUT_SIZE_1000, _sine_lut_1000);
            break;
        case 4: // LUT 4096 samples (highest precision default)
        default: // Default to highest precision LUT if invalid LOD is provided
            stream1_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_0 + int_frame) * deflated_baseWaveFreq, SINE_LUT_SIZE_4096, _sine_lut_4096);
            stream2_raw_sine = _get_sine_from_lod_lut(((double)internal_streamsOffset_1 + int_frame) * deflated_stream2FreqMult_applied, SINE_LUT_SIZE_4096, _sine_lut_4096);
            break;
    }
    
    output.stream1_val = floor(stream1_raw_sine * s1_quant_level) / (double)s1_quant_level;
    output.stream2_val = floor(stream2_raw_sine * s2_quant_level) / (double)s2_quant_level;

    // Use frame for modulo operation
    output.selected_stream_idx = ((int_frame % internal_streamSwitchDur) < (internal_streamSwitchDur / 2)) ? 0 : 1;
    
    double active_stream_val_double = (output.selected_stream_idx == 0) ? output.stream1_val : output.stream2_val;

    if (finalRandSwitch == 1) {
        output.randVal = portable_rand((int)(active_stream_val_double * FPSR_INFLATION_FACTOR)); // Seed is already large integer
    } else {
        output.randVal = (float)(0.5 * active_stream_val_double + 0.5); // Return float, calculations in double
    }
    return output;
}

/******************************************************************************/
/* High-Level Wrapper Functions with Robust Search                            */
/******************************************************************************/

/**
 * @brief Wrapper for fpsr_sm that returns a detailed FPSR_Output struct.
 * @param frame (int) The current frame or time input.
 * @param frame_multiplier (float) A float value to scale the frame input, effectively speeding
 * up or slowing down the algorithm's internal timeline.
 * @param minHold (int) The minimum duration (in frames) for a value to hold.
 * @param maxHold (int) The maximum duration (in frames) for a value to hold.
 * @param reseedInterval (int) The fixed interval at which a new hold duration is calculated.
 * @param seedInner (int) An offset for the random duration calculation to create unique sequences.
 * @param seedOuter (int) An offset for the final value calculation to create unique sequences.
 * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search to prevent infinite loops.
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_sm_get_details(
    int frame, float frame_multiplier, // Reordered frame_multiplier
    int minHold, int maxHold,
    int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch,
    int lod, int max_search_frames)
{
    FPSR_Output out = {0};
    
    // Calculate the scaled frame input for the base algorithm (as double)
    double current_scaled_frame_double = (double)frame * frame_multiplier; 

    // LOD 0: Get current value.
    out.randVal = _fpsr_sm_base(current_scaled_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);

    if (lod < 1) return out;

    // LOD 1: Compare with previous frame to check for change.
    // Calculate the scaled frame input for the previous frame
    double prev_scaled_frame_for_lod1_double = (double)(frame - 1) * frame_multiplier; 
    float prev_val = _fpsr_sm_base(prev_scaled_frame_for_lod1_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
    out.has_changed = (out.randVal != prev_val);

    if (lod < 2) return out;

    // LOD 2: Use a robust two-phase search to find change frames.
    int low_int, high_int, mid_int, result_int; 
    float next_val_candidate = 0.0f; // Stores the value at the next_changed_frame

    // --- Backwards Search for last_changed_frame ---
    if (out.has_changed) {
        // Optimization: If has_changed == 1, assign last_changed_frame = frame - 1.
        // Avoids 4 calls to _fpsr_xx_base(): (1 in exponential probe, 3 in binary search)
        // Avoids 3 loops iterations (1 in exponential probe, 2 in binary search)
        out.last_changed_frame = frame;
    } else {
        // Phase 1: Exponential probe to find a "dirty" region.
        int bound_low_int = frame; // operates on original frame space (int)
        int step_int = 1; // Used for exponential probe step
        while (frame - step_int > frame - max_search_frames) { 
            // Scale the probe frame (as double) before passing to base algorithm
            double probe_frame_double = (double)(frame - step_int) * frame_multiplier; 
            float val_at_probe = _fpsr_sm_base(probe_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
            if (val_at_probe != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2; 
        }
        
        // Phase 2: Binary search within the safe, "dirty" region.
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame; // result stores the original frame number (int)
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2; 
            // Scale the mid frame (as double) before passing to base algorithm
            double mid_frame_double = (double)mid_int * frame_multiplier; 
            if (_fpsr_sm_base(mid_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) == out.randVal) {
                // Check the frame immediately preceding 'mid' in the scaled timeline
                // using 1 as the step for comparison
                double mid_minus_step_frame_double = (double)(mid_int - 1) * frame_multiplier; 
                if (_fpsr_sm_base(mid_minus_step_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = result_int;
    }


    // --- Forwards Search for next_changed_frame ---
    // Phase 1: Exponential probe.
    int bound_high_int = frame; // operates on original frame space (int)
    int step_int = 1; // Used for exponential probe step
    while (frame + step_int < frame + max_search_frames) { 
        // Scale the probe frame (as double) before passing to base algorithm
        double probe_frame_double = (double)(frame + step_int) * frame_multiplier; 
        float val_at_probe = _fpsr_sm_base(probe_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe; // Store the value at the first differing frame
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2; 
    }

    // Phase 2: Binary search.
    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames; // Default if no change is found, in original frame space (int)
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2; 
        // Scale the mid frame (as double) before passing to base algorithm
        double mid_frame_double = (double)mid_int * frame_multiplier; 
        float mid_val = _fpsr_sm_base(mid_frame_double, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val; // Store the value at this frame
            high_int = mid_int - 1; 
        } else {
            low_int = mid_int + 1; 
        }
    }
    out.next_changed_frame = result_int;
    
    // Calculate hold progress based on scaled frame values
    // These calculations now also use double for precision before final cast
    double scaled_last_changed_frame_val_double = (double)out.last_changed_frame * frame_multiplier; 
    double scaled_next_changed_frame_val_double = (double)out.next_changed_frame * frame_multiplier; 
    double hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double;
    
    if (hold_duration_scaled_double > 0.0) { 
        out.hold_progress = (float)((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double); 
    } else {
        out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
    }

    out.randVal_next_changed_frame = next_val_candidate; 

    return out;
}

/**
 * @brief Wrapper for fpsr_tm that returns a detailed FPSR_Output struct.
 * @param frame (int) The current frame or time input.
 * @param frame_multiplier (float) A float value to scale the frame input, effectively speeding
 * up or slowing down the algorithm's internal timeline.
 * @param periodA (int) The first hold duration (in frames).
 * @param periodB (int) The second hold duration (in frames).
 * @param periodSwitch (int) The fixed interval at which the hold duration is toggled.
 * @param seedInner (int) An offset for the toggle clock to de-sync it from the main clock.
 * @param seedOuter (int) An offset for the main clock to create unique sequences.
 * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search to prevent infinite loops.
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_tm_get_details(
    int frame, float frame_multiplier, // Reordered frame_multiplier
    int periodA, int periodB,
    int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch,
    int lod, int max_search_frames)
{
    FPSR_Output out = {0};

    // Calculate the scaled frame input for the base algorithm (as double)
    double current_scaled_frame_double = (double)frame * frame_multiplier; // Cast frame to double for multiplication

    // LOD 0
    out.randVal = _fpsr_tm_base(current_scaled_frame_double, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
    
    if (lod < 1) return out;

    // LOD 1: Compare with previous frame to check for change.
    // Calculate the scaled frame input for the previous frame
    double prev_scaled_frame_for_lod1_double = (double)(frame - 1) * frame_multiplier; 
    float prev_val = _fpsr_tm_base((int)floor(prev_scaled_frame_for_lod1_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
    out.has_changed = (out.randVal != prev_val);
    
    if (lod < 2) return out;

    // LOD 2: Robust Search
    int low_int, high_int, mid_int, result_int; 
    float next_val_candidate = 0.0f; // Stores the value at the next_changed_frame

    // --- Backwards Search for last_changed_frame ---
    if (out.has_changed) {
        // Optimization: If has_changed == 1, assign last_changed_frame = frame - 1.
        // Avoids 4 calls to _fpsr_xx_base(): (1 in exponential probe, 3 in binary search)
        // Avoids 3 loops iterations (1 in exponential probe, 2 in binary search)
        out.last_changed_frame = frame;
    } else {
        int bound_low_int = frame; // operates on original frame space (int)
        int step_int = 1; // Used for exponential probe step
        while (frame - step_int > frame - max_search_frames) { // All int
            // Scale the probe frame (as double) before passing to base algorithm
            double probe_frame_double = (double)(frame - step_int) * frame_multiplier; // All double
            if (_fpsr_tm_base((int)floor(probe_frame_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2; // Changed to int
        }
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame; // result stores the original frame number (int)
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2; // All int
            // Scale the mid frame (as double) before passing to base algorithm
            double mid_frame_double = (double)mid_int * frame_multiplier; // All double
            if (_fpsr_tm_base((int)floor(mid_frame_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) == out.randVal) {
                // Check the frame immediately preceding 'mid' in the scaled timeline
                // using 1 as the step for comparison
                double mid_minus_step_frame_double = (double)(mid_int - 1) * frame_multiplier; 
                if (_fpsr_tm_base((int)floor(mid_minus_step_frame_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = result_int;
    }

    // Forwards search
    int bound_high_int = frame; // operates on original frame space (int)
    step_int = 1; // Used for exponential probe step
    while (frame + step_int < frame + max_search_frames) { // All int
        // Scale the probe frame (as double) before passing to base algorithm
        double probe_frame_double = (double)(frame + step_int) * frame_multiplier; // All double
        float val_at_probe = _fpsr_tm_base((int)floor(probe_frame_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe; // Store the value at the first differing frame
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2; // Changed to int
    }
    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames; // Default if no change is found, in original frame space (int)
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2; // All int
        // Scale the mid frame (as double) before passing to base algorithm
        double mid_frame_double = (double)mid_int * frame_multiplier; // All double
        float mid_val = _fpsr_tm_base((int)floor(mid_frame_double), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val; // Store the value at this frame
            high_int = mid_int - 1; // All int
        } else {
            low_int = mid_int + 1; // All int
        }
    }
    out.next_changed_frame = result_int;
    
    // Calculate hold progress based on scaled frame values
    // These calculations now also use double for precision before final cast
    double scaled_last_changed_frame_val_double = (double)out.last_changed_frame * frame_multiplier; // All double
    double scaled_next_changed_frame_val_double = (double)out.next_changed_frame * frame_multiplier; // All double
    double hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double;
    
    if (hold_duration_scaled_double > 0.0) { // Use double literal
        out.hold_progress = (float)((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double); // Cast final result to float
    } else {
        out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
    }

    out.randVal_next_changed_frame = next_val_candidate; // Assign the captured value

    return out;
}

/**
 * @brief Wrapper for fpsr_qs that returns a detailed FPSR_Output struct.
 * @param frame (int) The current frame or time input.
 * @param frame_multiplier (float) A float value to scale the frame input, effectively speeding
 * up or slowing down the algorithm's internal timeline.
 * @param baseWaveFreq (float) The base frequency for the modulation wave of stream 1.
 * @param stream2FreqMult (float) A multiplier for the second stream's frequency.
 * @param quantLevelsMinMax (const int[2]) An array of two integers for the min and max quantisation levels.
 * @param streamsOffset (const int[2]) An array of two integers to offset the frame for each stream's sine wave.
 * @param quantOffsets (const int[2]) An array of two integers to offset the random quantisation selection for each stream.
 * @param streamSwitchDur (int) The number of frames after which the streams switch.
 * @param stream1QuantDur (int) The duration for which stream 1's random quantisation level is held.
 * @param stream2QuantDur (int) The duration for which stream 2's random quantisation level is held.
 * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
 * @param sine_lod_level (int) Level of detail for sine wave generation 
 * (0: direct sin(), 1-3: (100, 500, 1000 samples) LUTs, 4: (4096 samples) highest precision LUT).
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search to prevent infinite loops.
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_qs_get_details(
    int frame, float frame_multiplier, // Reordered frame_multiplier
    float baseWaveFreq, float stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch,
    int sine_lod_level, // New parameter for sine LOD
    int lod, int max_search_frames)
{
    FPSR_Output out = {0};

    // Calculate the scaled frame input for the base algorithm (as double)
    double current_scaled_frame_double = (double)frame * frame_multiplier; // Cast frame to double for multiplication

    // LOD 0
    _FPSR_QS_Base_Output base_qs_output = _fpsr_qs_base((int)floor(current_scaled_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
    out.randVal = base_qs_output.randVal;
    out.randStreams[0] = base_qs_output.stream1_val;
    out.randStreams[1] = base_qs_output.stream2_val;
    out.selected_stream = base_qs_output.selected_stream_idx;

    if (lod < 1) return out;

    // LOD 1: Compare with previous frame to check for change.
    // Calculate the scaled frame input for the previous frame
    double prev_scaled_frame_for_lod1_double = (double)(frame - 1) * frame_multiplier; 
    _FPSR_QS_Base_Output prev_qs_output = _fpsr_qs_base((int)floor(prev_scaled_frame_for_lod1_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
    float prev_val = prev_qs_output.randVal;
    out.has_changed = (out.randVal != prev_val);
    
    if (lod < 2) return out;

    // LOD 2: Robust Search
    int low_int, high_int, mid_int, result_int; 
    float next_val_candidate = 0.0f; // Stores the value at the next_changed_frame

    // --- Backwards Search for last_changed_frame ---
    if (has_changed) {
        // Optimization: If has_changed == 1, assign last_changed_frame = frame - 1.
        // Avoids 4 calls to _fpsr_xx_base(): (1 in exponential probe, 3 in binary search)
        // Avoids 3 loops iterations (1 in exponential probe, 2 in binary search)
        out.last_changed_frame = frame;
    } else {
        // Phase 1: Exponential probe to find a "dirty" region.
        int bound_low_int = frame; // operates on original frame space (int)
        int step_int = 1; // Used for exponential probe step
        while (frame - step_int > frame - max_search_frames) { 
            // Scale the probe frame (as double) before passing to base algorithm
            double probe_frame_double = (double)(frame - step_int) * frame_multiplier; 
            _FPSR_QS_Base_Output probe_qs_output = _fpsr_qs_base((int)floor(probe_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
            if (probe_qs_output.randVal != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2; 
        }
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame; // result stores the original frame number (int)
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2; 
            // Scale the mid frame (as double) before passing to base algorithm
            double mid_frame_double = (double)mid_int * frame_multiplier; 
            _FPSR_QS_Base_Output mid_qs_output = _fpsr_qs_base((int)floor(mid_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
            if (mid_qs_output.randVal == out.randVal) {
                // Check the frame immediately preceding 'mid' in the scaled timeline
                // using 1 as the step for comparison
                double mid_minus_step_frame_double = (double)(mid_int - 1) * frame_multiplier; 
                _FPSR_QS_Base_Output mid_minus_step_qs_output = _fpsr_qs_base((int)floor(mid_minus_step_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
                if (mid_minus_step_qs_output.randVal != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = result_int;
    }

    // Forwards search
    int bound_high_int = frame; // operates on original frame space (int)
    step_int = 1; // Used for exponential probe step
    while (frame + step_int < frame + max_search_frames) { 
        // Scale the probe frame (as double) before passing to base algorithm
        double probe_frame_double = (double)(frame + step_int) * frame_multiplier; 
        _FPSR_QS_Base_Output probe_qs_output = _fpsr_qs_base((int)floor(probe_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
        if (probe_qs_output.randVal != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = probe_qs_output.randVal; // Store the value at the first differing frame
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2; 
    }
    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames; // Default if no change is found, in original frame space (int)
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2; 
        // Scale the mid frame (as double) before passing to base algorithm
        double mid_frame_double = (double)mid_int * frame_multiplier; 
        _FPSR_QS_Base_Output mid_qs_output = _fpsr_qs_base((int)floor(mid_frame_double), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level);
        if (mid_qs_output.randVal != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_qs_output.randVal; // Store the value at this frame
            high_int = mid_int - 1; 
        } else {
            low_int = mid_int + 1; 
        }
    }
    out.next_changed_frame = result_int;
    
    // Calculate hold progress based on scaled frame values
    // These calculations now also use double for precision before final cast
    double scaled_last_changed_frame_val_double = (double)out.last_changed_frame * frame_multiplier; 
    double scaled_next_changed_frame_val_double = (double)out.next_changed_frame * frame_multiplier; 
    double hold_duration_scaled_double = scaled_next_changed_frame_val_double - scaled_last_changed_frame_val_double;
    
    if (hold_duration_scaled_double > 0.0) { 
        out.hold_progress = (float)((current_scaled_frame_double - scaled_last_changed_frame_val_double) / hold_duration_scaled_double); 
    } else {
        out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
    }

    out.randVal_next_changed_frame = next_val_candidate; 

    return out;
}
