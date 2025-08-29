// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/fpsr

// Frame-Persistent Stateless Randomisation (FPS-R) for Adobe After Effects
// FPS-R is a set of three algorithms that generate
//     frame-persistent and stateless random values. 
// This file contains three stateless, frame-persistent randomization algorithms:
//     Stacked Modulo (SM), Toggled Modulo (TM) and Quantised Switching (QS).
// It uses a custom portable_rand() function to ensure
// deterministic and consistent results across any platform.
//
// --- HOW TO USE ---
// 1. Create a Null Object named "FPSR_Controls".
// 2. Add Expression Controls (Sliders, Checkboxes) to the "FPSR_Controls" layer for each parameter you want to tweak.
// 3. Copy and paste this entire script into the expression editor of the property you want to animate (e.g., Position, Rotation, a Slider).
// 4. At the bottom of the script, choose which algorithm to run (fpsr_sm, fpsr_tm, or fpsr_qs) and assign its output to your property.

// ===================================================================================
// ==  PORTABLE RAND FUNCTION (Used by SM, TM and QS)                               ==
// ===================================================================================

function portable_rand(seed) {
    // A simple, portable pseudo-random number generator.
    // Ensures identical results on any platform.
    let val = seed * 12.9898;

    // --- FIX for float precision on GPUs and other platforms ---
    // By using the mathematical property sin(x) = sin(x mod 2π), we can wrap the
    // input to sin() into a high-precision range, ensuring the result
    // remains stable and correct indefinitely. JavaScript's '%' operator works on floats.
    val = val % (2 * Math.PI);

    let result = Math.sin(val) * 43758.5453;
    
    // JavaScript's equivalent of frac()
    return result - Math.floor(result);
}


// ===================================================================================
// ==  ALGORITHM 1: STACKED MODULO (SM)                                             ==
// ===================================================================================

/**
 * @brief Generates a persistent random value that holds for a calculated duration.
 * @details This function uses a two-step process. First, it determines a random
 * "hold duration". Second, it generates a stable integer for that duration,
 * which is then used as a seed to produce the final, held random value.
 *
 * @param {number} frame The current frame or time input.
 * @param {number} minHold The minimum duration (in frames) for a value to hold.
 * @param {number} maxHold The maximum duration (in frames) for a value to hold.
 * @param {number} reseedInterval The fixed interval at which a new hold duration is calculated.
 * @param {number} seedInner An offset for the random duration calculation to create unique sequences.
 * @param {number} seedOuter An offset for the final value calculation to create unique sequences.
 * @param {boolean} finalRandSwitch A flag that can turn off the final randomisation step.
 * @returns {number} 
 * when finalRandSwitch is false: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is true: 
 * A float value between 0.0 and 1.0 that remains constant for the hold duration.
 */
function fpsr_sm(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) {
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    let rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval));
    let holdDuration = Math.floor(minHold + rand_for_duration * (maxHold - minHold));

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // This value is constant for the entire duration of the hold.
    let held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    // Because the seed is stable, the final value is also stable.
    let fpsr_output;
    if (finalRandSwitch) {
        // If finalRandSwitch is true, we apply the final randomisation step.
        fpsr_output = portable_rand(held_integer_state * 100000.0);
    } else {
        // If finalRandSwitch is false, we return the raw integer state directly.
        fpsr_output = held_integer_state;
    }
    return fpsr_output;
}


// ===================================================================================
// ==  ALGORITHM 2: TOGGLED MODULO (TM)                                             ==
// ===================================================================================

/**
 * @brief Generates a persistent value that holds for a rhythmically toggled duration.
 * @details This function uses a deterministic switch to toggle the hold duration
 * between two fixed periods. This creates a predictable, rhythmic, or mechanical
 * "move-and-hold" pattern, as opposed to the organic randomness of SM.
 *
 * @param {number} frame The current frame or time input.
 * @param {number} periodA The first hold duration (in frames).
 * @param {number} periodB The second hold duration (in frames).
 * @param {number} periodSwitch The fixed interval at which the hold duration is toggled.
 * @param {number} seedInner An offset for the toggle clock to de-sync it from the main clock.
 * @param {number} seedOuter An offset for the main clock to create unique sequences.
 * @param {boolean} finalRandSwitch A flag that can turn off the final randomisation step.
 * @returns {number}
 * when finalRandSwitch is false: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is true: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */
function fpsr_tm(frame, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) {
    // --- 1. Determine the hold duration by toggling between two periods ---
    if (periodSwitch < 1) { periodSwitch = 1; } // Prevent division by zero.

    // The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    let inner_clock_frame = seedInner + frame;
    
    let holdDuration;
    // The ternary switch: toggle between periodA and periodB at a fixed rhythm.
    if ((inner_clock_frame % periodSwitch) < (periodSwitch * 0.5)) {
        holdDuration = periodA;
    } else {
        holdDuration = periodB;
    }

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // The "outer clock" is offset by seedOuter to create unique output sequences.
    let outer_clock_frame = seedOuter + frame;
    let held_integer_state = outer_clock_frame - (outer_clock_frame % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    let fpsr_output;
    if (finalRandSwitch) {
        // If true, apply the final randomisation hash.
        fpsr_output = portable_rand(held_integer_state);
    } else {
        // If false, return the raw integer state directly.
        fpsr_output = held_integer_state; 
    }
    return fpsr_output;
}


// ===================================================================================
// ==  ALGORITHM 3: QUANTISED SWITCHING (QS)                                        ==
// ===================================================================================

/**
 * @brief Generates a flickering, quantised value by switching between two sine wave streams.
 * @details This function creates two separate, quantised sine waves. For each stream,
 * a new random quantisation level is chosen from within the [min, max] range at a
 * set interval. The function then switches between these two streams to create
 * complex, glitch-like patterns.
 *
 * @param {number} frame The current frame or time input.
 * @param {number} baseWaveFreq The base frequency for the modulation wave of stream 1.
 * @param {number} stream2FreqMult A multiplier for the second stream's frequency.
 * @param {Array<number>} quantLevelsMinMax An array of two integers for the min and max quantisation levels.
 * @param {Array<number>} streamsOffset An array of two integers to offset the frame for each stream's sine wave.
 * @param {Array<number>} quantOffsets An array of two integers to offset the random quantisation selection for each stream.
 * @param {number} streamSwitchDur The number of frames after which the streams switch.
 * @param {number} stream1QuantDur The duration for which stream 1's random quantisation level is held.
 * @param {number} stream2QuantDur The duration for which stream 2's random quantisation level is held.
 * @param {boolean} finalRandSwitch A flag that can turn off the final randomisation step.
 * @returns {number} A float value between 0.0 and 1.0.
 */
function fpsr_qs(
    frame, baseWaveFreq, stream2FreqMult,
    quantLevelsMinMax, streamsOffset, quantOffsets,
    streamSwitchDur, stream1QuantDur, stream2QuantDur,
    finalRandSwitch)
{
    // --- 1. Set default durations if not provided ---
    if (streamSwitchDur < 1) { streamSwitchDur = Math.floor((1.0 / baseWaveFreq) * 0.76); }
    if (stream1QuantDur < 1) { stream1QuantDur = Math.floor((1.0 / baseWaveFreq) * 1.2); }
    if (stream2QuantDur < 1) { stream2QuantDur = Math.floor((1.0 / baseWaveFreq) * 0.9); }
    
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate random quantisation levels for each stream ---
    let quant_min = quantLevelsMinMax[0];
    let quant_max = quantLevelsMinMax[1];
    let quant_range = quant_max - quant_min + 1;

    // --- Stream 1 Quant Level ---
    let s1_quant_seed = (quantOffsets[0] + frame) - ((quantOffsets[0] + frame) % stream1QuantDur);
    let s1_rand_for_quant = portable_rand(s1_quant_seed);
    let s1_quant_level = quant_min + Math.floor(s1_rand_for_quant * quant_range);

    // --- Stream 2 Quant Level ---
    let s2_quant_seed = (quantOffsets[1] + frame) - ((quantOffsets[1] + frame) % stream2QuantDur);
    let s2_rand_for_quant = portable_rand(s2_quant_seed);
    let s2_quant_level = quant_min + Math.floor(s2_rand_for_quant * quant_range);

    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }

    // --- 3. Generate the two quantised sine wave streams ---
    if (stream2FreqMult < 0) { stream2FreqMult = 3.7; }

    let stream1 = Math.floor(Math.sin((streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / s1_quant_level;
    let stream2 = Math.floor(Math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / s2_quant_level;

    // --- 4. Switch between the two streams ---
    let active_stream_val = ((frame % streamSwitchDur) < streamSwitchDur / 2) ? stream1 : stream2;

    // --- 5. Hash the final output or bypass ---
    let fpsr_output;
    if (finalRandSwitch) {
        fpsr_output = portable_rand(Math.floor(active_stream_val * 100000.0));
    } else {
        fpsr_output = 0.5 * active_stream_val + 0.5; // Scale from [-1, 1] to [0, 1]
    }
    return fpsr_output;
}


// ===================================================================================
// ==  EXECUTION AND PARAMETER SETUP                                                ==
// ===================================================================================

// --- Common Parameters ---
// Link to a master frame offset slider to easily shift the entire animation in time.
try { 
    let frameOffset = thisComp.layer("FPSR_Controls").effect("Master frameOffset")("Slider"); 
} catch(e) { 
    let frameOffset = 0; 
}
let currentFrame = Math.round(time / thisComp.frameDuration) + frameOffset;

// --- Link to Expression Controls on your "FPSR_Controls" Null Layer ---
// If a control doesn't exist on the layer, a default value is used.

// SM Parameters
try { let p_sm_minHold = thisComp.layer("FPSR_Controls").effect("SM minHold")("Slider"); } catch(e) { let p_sm_minHold = 16; }
try { let p_sm_maxHold = thisComp.layer("FPSR_Controls").effect("SM maxHold")("Slider"); } catch(e) { let p_sm_maxHold = 24; }
try { let p_sm_reseed = thisComp.layer("FPSR_Controls").effect("SM reseedInterval")("Slider"); } catch(e) { let p_sm_reseed = 9; }
try { let p_sm_seedIn = thisComp.layer("FPSR_Controls").effect("SM seedInner")("Slider"); } catch(e) { let p_sm_seedIn = -41; }
try { let p_sm_seedOut = thisComp.layer("FPSR_Controls").effect("SM seedOuter")("Slider"); } catch(e) { let p_sm_seedOut = 23; }
try { let p_sm_bypass = thisComp.layer("FPSR_Controls").effect("SM Bypass Rand")("Checkbox").value ? false : true; } catch(e) { let p_sm_bypass = true; }

// TM Parameters
try { let p_tm_periodA = thisComp.layer("FPSR_Controls").effect("TM periodA")("Slider"); } catch(e) { let p_tm_periodA = 10; }
try { let p_tm_periodB = thisComp.layer("FPSR_Controls").effect("TM periodB")("Slider"); } catch(e) { let p_tm_periodB = 25; }
try { let p_tm_switchDur = thisComp.layer("FPSR_Controls").effect("TM switch_duration")("Slider"); } catch(e) { let p_tm_switchDur = 30; }
try { let p_tm_seedIn = thisComp.layer("FPSR_Controls").effect("TM seedInner")("Slider"); } catch(e) { let p_tm_seedIn = 15; }
try { let p_tm_seedOut = thisComp.layer("FPSR_Controls").effect("TM seedOuter")("Slider"); } catch(e) { let p_tm_seedOut = 0; }
try { let p_tm_bypass = thisComp.layer("FPSR_Controls").effect("TM Bypass Rand")("Checkbox").value ? false : true; } catch(e) { let p_tm_bypass = true; }

// QS Parameters
try { let p_qs_baseFreq = thisComp.layer("FPSR_Controls").effect("QS baseWaveFreq")("Slider"); } catch(e) { let p_qs_baseFreq = 0.012; }
try { let p_qs_freqMult = thisComp.layer("FPSR_Controls").effect("QS stream2FreqMult")("Slider"); } catch(e) { let p_qs_freqMult = 3.1; }
try { let p_qs_qMin = thisComp.layer("FPSR_Controls").effect("QS quantMin")("Slider"); } catch(e) { let p_qs_qMin = 4; }
try { let p_qs_qMax = thisComp.layer("FPSR_Controls").effect("QS quantMax")("Slider"); } catch(e) { let p_qs_qMax = 12; }
try { let p_qs_off1 = thisComp.layer("FPSR_Controls").effect("QS streamOffset1")("Slider"); } catch(e) { let p_qs_off1 = 0; }
try { let p_qs_off2 = thisComp.layer("FPSR_Controls").effect("QS streamOffset2")("Slider"); } catch(e) { let p_qs_off2 = 76; }
try { let p_qs_qOff1 = thisComp.layer("FPSR_Controls").effect("QS quantOffset1")("Slider"); } catch(e) { let p_qs_qOff1 = 10; }
try { let p_qs_qOff2 = thisComp.layer("FPSR_Controls").effect("QS quantOffset2")("Slider"); } catch(e) { let p_qs_qOff2 = 81; }
try { let p_qs_switchDur = thisComp.layer("FPSR_Controls").effect("QS streamSwitchDur")("Slider"); } catch(e) { let p_qs_switchDur = 24; }
try { let p_qs_qDur1 = thisComp.layer("FPSR_Controls").effect("QS quantDur1")("Slider"); } catch(e) { let p_qs_qDur1 = 16; }
try { let p_qs_qDur2 = thisComp.layer("FPSR_Controls").effect("QS quantDur2")("Slider"); } catch(e) { let p_qs_qDur2 = 20; }
try { let p_qs_bypass = thisComp.layer("FPSR_Controls").effect("QS Bypass Rand")("Checkbox").value ? false : true; } catch(e) { let p_qs_bypass = true; }


// --- CHOOSE WHICH ALGORITHM TO RUN ---
// Uncomment the algorithm you want to use and comment out the others.

let finalValue;

// Run Stacked Modulo (SM)
/*
// Sample calling code for SM
let frame = currentFrame;
let minHoldFrames = p_sm_minHold;
let maxHoldFrames = p_sm_maxHold;
let reseedFrames = p_sm_reseed;
let offsetInner = p_sm_seedIn;
let offsetOuter = p_sm_seedOut;
let finalRandSwitch = p_sm_bypass;

finalValue = fpsr_sm(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
let randVal_previous = fpsr_sm(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch);
let changed = (finalValue != randVal_previous); // 'changed' is true if the value changed from the previous frame
*/

// Run Toggled Modulo (TM)
/*
// Sample calling code for TM
let frame = currentFrame;
let period_A = p_tm_periodA;
let period_B = p_tm_periodB;
let switch_duration = p_tm_switchDur;
let offset_inner = p_tm_seedIn;
let offset_outer = p_tm_seedOut;
let final_rand_switch = p_tm_bypass;

finalValue = fpsr_tm(frame, period_A, period_B, switch_duration, offset_inner, offset_outer, final_rand_switch);
let randVal_previous = fpsr_tm(frame - 1, period_A, period_B, switch_duration, offset_inner, offset_outer, final_rand_switch);
let changed = (finalValue != randVal_previous); // 'changed' is true if the value changed from the previous frame
*/

// Run Quantised Switching (QS)
// Sample calling code for QS
let frame = currentFrame;
let baseWaveFreq = p_qs_baseFreq;
let stream2freqMult = p_qs_freqMult;
let quantLevelsMinMax = [p_qs_qMin, p_qs_qMax];
let streamsOffset = [p_qs_off1, p_qs_off2];
let quantOffsets = [p_qs_qOff1, p_qs_qOff2];
let streamSwitchDur = p_qs_switchDur;
let stream1QuantDur = p_qs_qDur1;
let stream2QuantDur = p_qs_qDur2;
let finalRandSwitch = p_qs_bypass;

finalValue = fpsr_qs(
    frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);
let randVal_previous = fpsr_qs(
    frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
    streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);
let changed = (finalValue != randVal_previous);

// --- APPLY THE FINAL VALUE TO THE PROPERTY ---

// For a 1D property (like Rotation or a Slider)
finalValue;

// For a 2D property (like 2D Position)
// [value[0], finalValue * 100]; // Example: drives Y position only, scaled by 100

// For a 3D property (like 3D Position)
// [value[0], finalValue * 100, value[2]]; // Example: drives Y position only, scaled by 100
