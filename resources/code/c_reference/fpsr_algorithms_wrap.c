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

 #include <math.h> // For sin() and floor()
 #include <stdio.h> // For NULL
 
 /******************************************************************************/
 /* Core Components (Struct and portable_rand)                                 */
 /******************************************************************************/
 
 // A simple, portable pseudo-random number generator that takes an integer seed.
 float portable_rand(int seed) {
     float val = (float)seed * 12.9898;
     const float TWO_PI = 6.28318530718f;
     val = fmod(val, TWO_PI);
     float result = sin(val) * 43758.5453f;
     return result - floor(result);
 }
 
 /*
 * This structure holds the output of the FPS-R algorithms.
 */
 typedef struct {
     float randVal; 
     int has_changed;
     float hold_progress;
     int last_changed_frame;
     int next_changed_frame;
 } FPSR_Output;
 
 
 /******************************************************************************/
 /* Untouched, Low-Level FPS-R Algorithms                                      */
 /******************************************************************************/
 // These functions remain pure, returning only a single float value.
 
 float _fpsr_sm_base(
     int frame, int minHold, int maxHold,
     int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch)
 {
     if (reseedInterval < 1) { reseedInterval = 1; }
     float rand_for_duration = portable_rand(seedInner + frame - (frame % reseedInterval));
     int holdDuration = (int)floor(minHold + rand_for_duration * (maxHold - minHold));
     if (holdDuration < 1) { holdDuration = 1; }
     int held_integer_state = (seedOuter + frame) - ((seedOuter + frame) % holdDuration);
     if (finalRandSwitch) {
         return portable_rand(held_integer_state * 100000.0);
     }
     return (float)held_integer_state;
 }
 
 float _fpsr_tm_base(
     int frame, int periodA, int periodB,
     int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch)
 {
     if (periodSwitch < 1) { periodSwitch = 1; }
     int inner_clock_frame = seedInner + frame;
     int holdDuration = ((inner_clock_frame % periodSwitch) < (periodSwitch / 2)) ? periodA : periodB;
     if (holdDuration < 1) { holdDuration = 1; }
     int outer_clock_frame = seedOuter + frame;
     int held_integer_state = outer_clock_frame - (outer_clock_frame % holdDuration);
     if (finalRandSwitch) {
         return portable_rand(held_integer_state * 100000.0);
     }
     return (float)held_integer_state;
 }
 
 float _fpsr_qs_base(
     int frame, float baseWaveFreq, float stream2FreqMult,
     const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
     int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch)
 {
     if (streamSwitchDur < 1) { streamSwitchDur = (int)floor((1.0 / baseWaveFreq) * 0.76); }
     if (stream1QuantDur < 1) { stream1QuantDur = (int)floor((1.0 / baseWaveFreq) * 1.2); }
     if (stream2QuantDur < 1) { stream2QuantDur = (int)floor((1.0 / baseWaveFreq) * 0.9); }
     if (streamSwitchDur < 1) { streamSwitchDur = 1; }
     if (stream1QuantDur < 1) { stream1QuantDur = 1; }
     if (stream2QuantDur < 1) { stream2QuantDur = 1; }
 
     int quant_min = quantLevelsMinMax[0];
     int quant_max = quantLevelsMinMax[1];
     int quant_range = quant_max - quant_min + 1;
     if (quant_range < 1) quant_range = 1;
 
     int s1_quant_seed = (quantOffsets[0] + frame) - ((quantOffsets[0] + frame) % stream1QuantDur);
     int s1_quant_level = quant_min + (int)floor(portable_rand(s1_quant_seed) * quant_range);
     int s2_quant_seed = (quantOffsets[1] + frame) - ((quantOffsets[1] + frame) % stream2QuantDur);
     int s2_quant_level = quant_min + (int)floor(portable_rand(s2_quant_seed) * quant_range);
     if (s1_quant_level < 1) { s1_quant_level = 1; }
     if (s2_quant_level < 1) { s2_quant_level = 1; }
 
     if (stream2FreqMult <= 0) { stream2FreqMult = 3.7f; }
     float stream1 = floor(sin((float)(streamsOffset[0] + frame) * baseWaveFreq) * s1_quant_level) / (float)s1_quant_level;
     float stream2 = floor(sin((float)(streamsOffset[1] + frame) * baseWaveFreq * stream2FreqMult) * s2_quant_level) / (float)s2_quant_level;
     float active_stream_val = ((frame % streamSwitchDur) < streamSwitchDur / 2) ? stream1 : stream2;
 
     if (finalRandSwitch == 1) {
         return portable_rand((int)(active_stream_val * 100000.0));
     }
     return 0.5f * active_stream_val + 0.5f;
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
     
     // Calculate the scaled frame input for the base algorithm
     int current_scaled_frame = (int)floor(frame * frame_multiplier);
 
     // LOD 0: Get current value.
     out.randVal = _fpsr_sm_base(current_scaled_frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
 
     if (lod < 1) return out;
 
     // LOD 1: Compare with previous frame to check for change.
     // Calculate the scaled frame input for the previous frame
     int prev_scaled_frame_for_lod1 = (int)floor((frame - 1) * frame_multiplier);
     float prev_val = _fpsr_sm_base(prev_scaled_frame_for_lod1, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
     out.has_changed = (out.randVal != prev_val);
 
     if (lod < 2) return out;
 
     // LOD 2: Use a robust two-phase search to find change frames.
     int low, high, mid, result;
 
     // --- Backwards Search for last_changed_frame ---
     // Phase 1: Exponential probe to find a "dirty" region.
     int bound_low = frame; // operates on original frame space
     int step = 1;
     while (frame - step > frame - max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         float val_at_probe = _fpsr_sm_base((int)floor((frame - step) * frame_multiplier), minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch);
         if (val_at_probe != out.randVal) {
             bound_low = frame - step;
             break;
         }
         bound_low = frame - step;
         step *= 2;
     }
     
     // Phase 2: Binary search within the safe, "dirty" region.
     low = bound_low;
     high = frame;
     result = frame; // result stores the original frame number
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_sm_base((int)floor(mid * frame_multiplier), minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) == out.randVal) {
             // Check the frame immediately preceding 'mid' in the scaled timeline
             if (_fpsr_sm_base((int)floor((mid - 1) * frame_multiplier), minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
                 result = mid; // Found the exact first original frame of the current hold
                 break;
             }
             high = mid - 1; // Continue searching in the lower half (original frame space)
         } else {
             low = mid + 1; // Search in the upper half (original frame space)
         }
     }
     out.last_changed_frame = result;
 
 
     // --- Forwards Search for next_changed_frame ---
     // Phase 1: Exponential probe.
     int bound_high = frame; // operates on original frame space
     step = 1;
     while (frame + step < frame + max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         if (_fpsr_sm_base((int)floor((frame + step) * frame_multiplier), minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
             bound_high = frame + step;
             break;
         }
         bound_high = frame + step;
         step *= 2;
     }
 
     // Phase 2: Binary search.
     low = frame;
     high = bound_high;
     result = frame + max_search_frames; // Default if no change is found, in original frame space
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_sm_base((int)floor(mid * frame_multiplier), minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
             result = mid; // Found the exact first original frame of the next hold
             high = mid - 1; // Try to find an even earlier change
         } else {
             low = mid + 1;
         }
     }
     out.next_changed_frame = result;
     
     // Calculate hold progress based on scaled frame values
     int scaled_last_changed_frame_val = (int)floor(out.last_changed_frame * frame_multiplier);
     int scaled_next_changed_frame_val = (int)floor(out.next_changed_frame * frame_multiplier);
     int hold_duration_scaled = scaled_next_changed_frame_val - scaled_last_changed_frame_val;
     
     if (hold_duration_scaled > 0) {
         out.hold_progress = (float)(current_scaled_frame - scaled_last_changed_frame_val) / (float)hold_duration_scaled;
     } else {
         out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
     }
 
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
     int frame, float frame_multiplier,
     int periodA, int periodB,
     int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch,
     int lod, int max_search_frames)
 {
     FPSR_Output out = {0};
 
     // Calculate the scaled frame input for the base algorithm
     int current_scaled_frame = (int)floor(frame * frame_multiplier);
 
     // LOD 0
     out.randVal = _fpsr_tm_base(current_scaled_scaled_frame, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
     if (lod < 1) return out;
 
     // LOD 1
     // Calculate the scaled frame input for the previous frame
     int prev_scaled_frame_for_lod1 = (int)floor((frame - 1) * frame_multiplier);
     float prev_val = _fpsr_tm_base(prev_scaled_frame_for_lod1, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch);
     out.has_changed = (out.randVal != prev_val);
     if (lod < 2) return out;
 
     // LOD 2: Robust Search
     int low, high, mid, result;
 
     // Backwards search
     int bound_low = frame;
     int step = 1;
     while (frame - step > frame - max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         if (_fpsr_tm_base((int)floor((frame - step) * frame_multiplier), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
             bound_low = frame - step;
             break;
         }
         bound_low = frame - step;
         step *= 2;
     }
     low = bound_low;
     high = frame;
     result = frame;
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_tm_base((int)floor(mid * frame_multiplier), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) == out.randVal) {
             // Check the frame immediately preceding 'mid' in the scaled timeline
             if (_fpsr_tm_base((int)floor((mid - 1) * frame_multiplier), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
                 result = mid; break;
             }
             high = mid - 1;
         } else {
             low = mid + 1;
         }
     }
     out.last_changed_frame = result;
 
     // Forwards search
     int bound_high = frame;
     step = 1;
     while (frame + step < frame + max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         if (_fpsr_tm_base((int)floor((frame + step) * frame_multiplier), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
             bound_high = frame + step;
             break;
         }
         bound_high = frame + step;
         step *= 2;
     }
     low = frame;
     high = bound_high;
     result = frame + max_search_frames;
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_tm_base((int)floor(mid * frame_multiplier), periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch) != out.randVal) {
             result = mid;
             high = mid - 1;
         } else {
             low = mid + 1;
         }
     }
     out.next_changed_frame = result;
     
     // Calculate hold progress based on scaled frame values
     int scaled_last_changed_frame_val = (int)floor(out.last_changed_frame * frame_multiplier);
     int scaled_next_changed_frame_val = (int)floor(out.next_changed_frame * frame_multiplier);
     int hold_duration_scaled = scaled_next_changed_frame_val - scaled_last_changed_frame_val;
     
     if (hold_duration_scaled > 0) {
         out.hold_progress = (float)(current_scaled_frame - scaled_last_changed_frame_val) / (float)hold_duration_scaled;
     } else {
         out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
     }
 
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
  * @param lod (int) The level of detail to calculate.
  * @param max_search_frames (int) A safety limit for the backward/forward search to prevent infinite loops.
  * @return FPSR_Output struct with metadata populated based on the LOD.
  */
 FPSR_Output fpsr_qs_get_details(
     int frame, float frame_multiplier,
     float baseWaveFreq, float stream2FreqMult,
     const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
     int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch,
     int lod, int max_search_frames)
 {
     FPSR_Output out = {0};
 
     // Calculate the scaled frame input for the base algorithm
     int current_scaled_frame = (int)floor(frame * frame_multiplier);
 
     // LOD 0
     out.randVal = _fpsr_qs_base(current_scaled_frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);
     if (lod < 1) return out;
 
     // LOD 1
     // Calculate the scaled frame input for the previous frame
     int prev_scaled_frame_for_lod1 = (int)floor((frame - 1) * frame_multiplier);
     float prev_val = _fpsr_qs_base(prev_scaled_frame_for_lod1, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch);
     out.has_changed = (out.randVal != prev_val);
     if (lod < 2) return out;
 
     // LOD 2: Robust Search
     int low, high, mid, result;
 
     // Backwards search
     int bound_low = frame;
     int step = 1;
     while (frame - step > frame - max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         if (_fpsr_qs_base((int)floor((frame - step) * frame_multiplier), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) != out.randVal) {
             bound_low = frame - step;
             break;
         }
         bound_low = frame - step;
         step *= 2;
     }
     low = bound_low;
     high = frame;
     result = frame;
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_qs_base((int)floor(mid * frame_multiplier), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) == out.randVal) {
             // Check the frame immediately preceding 'mid' in the scaled timeline
             if (_fpsr_qs_base((int)floor((mid - 1) * frame_multiplier), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) != out.randVal) {
                 result = mid; break;
             }
             high = mid - 1;
         } else {
             low = mid + 1;
         }
     }
     out.last_changed_frame = result;
 
     // Forwards search
     int bound_high = frame;
     step = 1;
     while (frame + step < frame + max_search_frames) {
         // Scale the probe frame before passing to base algorithm
         if (_fpsr_qs_base((int)floor((frame + step) * frame_multiplier), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) != out.randVal) {
             bound_high = frame + step;
             break;
         }
         bound_high = frame + step;
         step *= 2;
     }
     low = frame;
     high = bound_high;
     result = frame + max_search_frames;
     while(low <= high) {
         mid = low + (high - low) / 2;
         // Scale the mid frame before passing to base algorithm
         if (_fpsr_qs_base((int)floor(mid * frame_multiplier), baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch) != out.randVal) {
             result = mid;
             high = mid - 1;
         } else {
             low = mid + 1;
         }
     }
     out.next_changed_frame = result;
     
     // Calculate hold progress based on scaled frame values
     int scaled_last_changed_frame_val = (int)floor(out.last_changed_frame * frame_multiplier);
     int scaled_next_changed_frame_val = (int)floor(out.next_changed_frame * frame_multiplier);
     int hold_duration_scaled = scaled_next_changed_frame_val - scaled_last_changed_frame_val;
     
     if (hold_duration_scaled > 0) {
         out.hold_progress = (float)(current_scaled_frame - scaled_last_changed_frame_val) / (float)hold_duration_scaled;
     } else {
         out.hold_progress = 0.0f; // Handle zero duration to avoid division by zero
     }
 
     return out;
 }
