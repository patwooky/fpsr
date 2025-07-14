// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/FPSR_Algorithm

// FPS-R (Frame-Persistent Stateless Randomisation) for Adobe After Effects
// This expression contains both the Stacked Modulo (SM) and Quantised Switching (QS) algorithms.
//
// --- HOW TO USE ---
// 1. Create a Null Object named "FPSR_Controls".
// 2. Add Expression Controls (Sliders, Checkboxes) to the "FPSR_Controls" layer for each parameter you want to tweak.
// 3. Copy and paste this entire script into the expression editor of the property you want to animate (e.g., Position, Rotation, a Slider).
// 4. At the bottom of the script, choose which algorithm to run (fpsr_sm or fpsr_qs) and assign its output to your property.

// ===================================================================================
// ==  PORTABLE RAND FUNCTION (Used by both SM and QS)                            ==
// ===================================================================================

function portable_rand(seed) {
    // A simple, portable pseudo-random number generator.
    // Ensures identical results on any platform.
    var result = Math.sin(seed * 12.9898) * 43758.5453;
    return result - Math.floor(result);
}


// ===================================================================================
// ==  ALGORITHM 1: STACKED MODULO (SM)                                           ==
// ===================================================================================

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
function fpsr_sm(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) {
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; }

    var rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval));
    var holdDuration = Math.floor(minHold + rand_for_duration * (maxHold - minHold));

    if (holdDuration < 1) { holdDuration = 1; }

    // --- 2. Generate the stable integer "state" for the hold period ---
    var held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration);

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    var fpsr_output;
    if (finalRandSwitch) {
        // If true, apply the final randomisation hash.
        fpsr_output = portable_rand(held_integer_state * 100000.0);
    } else {
        // If false, return the raw integer state directly.
        fpsr_output = held_integer_state;
    }
    return fpsr_output;
}


// ===================================================================================
// ==  ALGORITHM 2: QUANTISED SWITCHING (QS)                                      ==
// ===================================================================================

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
function fpsr_qs(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) {
    // --- 1. Set default durations if not provided ---
    if (streamSwitchDur < 1) {
        streamSwitchDur = Math.floor((1.0 / baseWaveFreq) * 0.76);
    }
    if (stream1QuantDur < 1) {
        stream1QuantDur = Math.floor((1.0 / baseWaveFreq) * 1.2);
    }
    if (stream2QuantDur < 1) {
        stream2QuantDur = Math.floor((1.0 / baseWaveFreq) * 0.9);
    }
    // Ensure durations are at least 1 frame
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate quantisation levels for each stream ---
    var s1_quant_level;
    if ((streamsOffset[0] + frame) % stream1QuantDur < stream1QuantDur * 0.5) {
        s1_quant_level = quantLevelsMinMax[0];
    } else {
        s1_quant_level = quantLevelsMinMax[1];
    }

    var s2_quant_level;
    var STREAM2_QUANT_RATIO_MIN = 1.24;
    var STREAM2_QUANT_RATIO_MAX = 0.66;
    if ((streamsOffset[1] + frame) % stream2QuantDur < stream2QuantDur * 0.5) {
        s2_quant_level = Math.floor(quantLevelsMinMax[0] * STREAM2_QUANT_RATIO_MIN);
    } else {
        s2_quant_level = Math.floor(quantLevelsMinMax[1] * STREAM2_QUANT_RATIO_MAX);
    }
    // Ensure quantisation levels are at least 1
    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }

    // --- 3. Generate the two quantised sine wave streams ---
    var STREAM2_DEFAULT_FREQ_MULT = 3.7;
    if (stream2FreqMult < 0) {
        stream2FreqMult = STREAM2_DEFAULT_FREQ_MULT;
    }
    
    // The output of floor(sin(...) * level) / level is in the range [-1, 1]
    var stream1 = Math.floor(Math.sin((streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / s1_quant_level;
    var stream2 = Math.floor(Math.sin((streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / s2_quant_level;

    // --- 4. Switch between the two streams ---
    var active_stream_val;
    if ((frame % streamSwitchDur) < streamSwitchDur / 2) {
        active_stream_val = stream1;
    } else {
        active_stream_val = stream2;
    }

    // --- 5. Hash the final output or bypass to return the raw signal ---
    var fpsr_output;
    if (finalRandSwitch) {
        // If true, apply the final randomisation step.
        fpsr_output = portable_rand(active_stream_val * 100000.0);
    } else {
        // If false, scale the [-1, 1] signal to the [0, 1] range.
        fpsr_output = 0.5 * active_stream_val + 0.5;
    }
    return fpsr_output;
}


// ===================================================================================
// ==  EXECUTION AND PARAMETER SETUP                                              ==
// ===================================================================================

// --- Common Parameters ---
// Common params SM - comment out if not using
// frame offset slider
// try { var frameOffset = thisComp.layer("FPSR_Controls").effect("SM frameOffset")("Slider"); } catch(e) { var smFrameOffset = 0; }

// Common params QS - comment out if not using
// frame offset slider
try { var frameOffset = thisComp.layer("FPSR_Controls").effect("QS frameOffset")("Slider"); } catch(e) { var smFrameOffset = 0; }

// do not coment out
var currentFrame = Math.round(time / thisComp.frameDuration) + frameOffset;

// --- Link to Expression Controls on your "FPSR_Controls" Null Layer ---
// Example: var minHold = thisComp.layer("FPSR_Controls").effect("minHold")("Slider");
// If a control doesn't exist, a default value is used.

// SM Parameters
try { var p_sm_minHold = thisComp.layer("FPSR_Controls").effect("SM minHold")("Slider"); } catch(e) { var p_sm_minHold = 16; }
try { var p_sm_maxHold = thisComp.layer("FPSR_Controls").effect("SM maxHold")("Slider"); } catch(e) { var p_sm_maxHold = 24; }
try { var p_sm_reseed = thisComp.layer("FPSR_Controls").effect("SM reseedInterval")("Slider"); } catch(e) { var p_sm_reseed = 9; }
try { var p_sm_seedIn = thisComp.layer("FPSR_Controls").effect("SM seedInner")("Slider"); } catch(e) { var p_sm_seedIn = -41; }
try { var p_sm_seedOut = thisComp.layer("FPSR_Controls").effect("SM seedOuter")("Slider"); } catch(e) { var p_sm_seedOut = 23; }
try { var p_sm_bypass = thisComp.layer("FPSR_Controls").effect("SM Bypass Rand")("Checkbox") == 1 ? false : true; } catch(e) { var p_sm_bypass = true; }

// QS Parameters
try { var p_qs_baseFreq = thisComp.layer("FPSR_Controls").effect("QS baseWaveFreq")("Slider"); } catch(e) { var p_qs_baseFreq = 0.012; }
try { var p_qs_freqMult = thisComp.layer("FPSR_Controls").effect("QS stream2FreqMult")("Slider"); } catch(e) { var p_qs_freqMult = 3.1; }
try { var p_qs_qMin = thisComp.layer("FPSR_Controls").effect("QS quantMin")("Slider"); } catch(e) { var p_qs_qMin = 12; }
try { var p_qs_qMax = thisComp.layer("FPSR_Controls").effect("QS quantMax")("Slider"); } catch(e) { var p_qs_qMax = 22; }
try { var p_qs_off1 = thisComp.layer("FPSR_Controls").effect("QS offset1")("Slider"); } catch(e) { var p_qs_off1 = 0; }
try { var p_qs_off2 = thisComp.layer("FPSR_Controls").effect("QS offset2")("Slider"); } catch(e) { var p_qs_off2 = 76; }
try { var p_qs_switchDur = thisComp.layer("FPSR_Controls").effect("QS streamSwitchDur")("Slider"); } catch(e) { var p_qs_switchDur = 24; }
try { var p_qs_qDur1 = thisComp.layer("FPSR_Controls").effect("QS quantDur1")("Slider"); } catch(e) { var p_qs_qDur1 = 16; }
try { var p_qs_qDur2 = thisComp.layer("FPSR_Controls").effect("QS quantDur2")("Slider"); } catch(e) { var p_qs_qDur2 = 20; }
try { var p_qs_bypass = thisComp.layer("FPSR_Controls").effect("QS Bypass Rand")("Checkbox") == 1 ? false : true; } catch(e) { var p_qs_bypass = true; }


// --- CHOOSE WHICH ALGORITHM TO RUN ---
// Uncomment the algorithm you want to use.

// Run Stacked Modulo (SM)
// var finalValue = fpsr_sm(currentFrame, p_sm_minHold, p_sm_maxHold, p_sm_reseed, p_sm_seedIn, p_sm_seedOut, p_sm_bypass);

// Run Quantised Switching (QS)
var finalValue = fpsr_qs(currentFrame, p_qs_baseFreq, p_qs_freqMult, [p_qs_qMin, p_qs_qMax], [p_qs_off1, p_qs_off2], p_qs_switchDur, p_qs_qDur1, p_qs_qDur2, p_qs_bypass);


// --- APPLY THE FINAL VALUE TO THE PROPERTY ---

// For a 1D property (like Rotation or a Slider)
finalValue;

// For a 2D property (like 2D Position)
// [value[0], finalValue]; // Example: drives Y position only

// For a 3D property (like 3D Position)
// [value[0], finalValue, value[2]]; // Example: drives Y position only
