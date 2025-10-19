// SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/fpsr

// ⚠️ This C version of FPS-R is the canonical reference implementation.
// All language bindings and variants should conform to this behavior.
// Do not alter without updating downstream bindings or reference docs.

/**
 * @file fpsr_algorithms.c
 * @brief Portable C implementation of FPS-R algorithms: 
 * Stacked Modulo (SM), Toggled Modulo (TM), Quantised Switching (QS), and Bitwise Decode (BD).
 * @details This file contains four stateless, frame-persistent randomisation algorithms.
 * It uses a custom portable_rand() function to ensure deterministic and
 * consistent results across any platform.
 */
 
// Online C compiler to run C program online
// Feel free to run this code and test it yourself
// with any online C compiler.
// https://www.programiz.com/c-programming/online-compiler/
#include <stdio.h>
#include <math.h> // For sin(), floor(), ceil(), log2()
// Add fixed-width integer headers for deterministic 64-bit math on all platforms.
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h> // For malloc(), free()
#include <string.h> // For strcmp(), memset()
#if defined(_MSC_VER)
#include <malloc.h> // For _alloca
#define alloca _alloca
#elif defined(__GNUC__) || defined(__clang__)
#include <alloca.h> // For alloca
#endif

// Bit-width used for chunked bit operations.
// It must remain 64 for deterministic compatibility with SplitMix64 and the 64-bit masking below. DO NOT MODIFY.
#define CHUNK_BITS 64
// Safety limit for stack allocation in fpsr_bd to prevent stack overflow.
#define BD_MAX_STACK_BLOCK_SIZE 8192

/**
 * Deterministic integer math helpers and PRNG
 *
 * Rationale for determinism across C and Python:
 * - Python's % and // are floor-based for negatives; C's % and / truncate toward zero.
 * Using floor-mod alignment here ensures identical behavior for negative frames/seeds.
 * - All integer counters, frames, durations, and seeds are int64_t for large-range support.
 * - All fractional math that converts to/from integers uses double to match Python's float.
 * - The PRNG uses a uint64_t mixer (SplitMix64) with well-defined wraparound, then maps the
 * top 53 bits to a double in [0,1). This yields bit-for-bit identical results across
 * compilers and mirrors a standard reference implementation in Python.
 */

// Floor-based modulo that matches Python's semantics for negative inputs.
// C's a % m truncates toward zero; Python's a % m is always in [0, m-1] for m>0.
// By normalizing remainders this way, all alignments and toggles match Python exactly.
static inline int64_t i64_floor_mod(int64_t a, int64_t m) {
    // assume m > 0
    int64_t r = a % m;
    if (r < 0) r += m;
    return r;
}

// Align-down to the nearest multiple of m using floor-mod semantics.
// This mirrors Python's a - (a % m) even when a is negative, guaranteeing parity
// between C and Python for all frame-alignment logic.
static inline int64_t i64_align_down(int64_t a, int64_t m) {
    return a - i64_floor_mod(a, m);
}

// SplitMix64: simple, robust 64-bit mixer using well-defined uint64_t wraparound.
// Produces identical bit patterns across platforms/compilers. Suitable for hashing
// integer seeds into pseudo-random 64-bit values.
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x ^= (x >> 31);
    return x;
}

// Map a uint64_t to a double in [0,1) by taking the top 53 bits (double mantissa width).
// Using the identical bit-extraction and scale factor (2^-53) in Python ensures that
// both languages produce the exact same floating value for the same 64-bit seed.
static inline double portable_rand_u64(uint64_t seed) {
    uint64_t r = splitmix64(seed);
    return (double)(r >> 11) * (1.0 / 9007199254740992.0); // 2^53
}

/**
 * A simple, portable pseudo-random number generator.
 * @brief Back-compat wrapper: generates a deterministic float in [0, 1) from an integer seed.
 * @details This now forwards to the 64-bit deterministic PRNG above to guarantee parity with Python.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
static inline float portable_rand(int seed) {
    // Keep seeding strictly in integer domain; cast via int64_t to preserve sign.
    return (float)portable_rand_u64((uint64_t)(int64_t)seed);
}

// --- Bitwise Rotation Helpers ---
// Performs a circular (rotate) left shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_left_shift(uint64_t value, int shift) {
    int s = shift % CHUNK_BITS;        // When CHUNK_BITS is 64, ensure shift is within 0-63 (wraps for >64)
    if (s == 0) return value;          // No shift needed if s is zero
    // Shift left by s, then fill in the lower bits with the upper bits shifted out
    return (value << s) | (value >> (CHUNK_BITS - s));
}

// Performs a circular (rotate) right shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_right_shift(uint64_t value, int shift) {
    int s = shift % CHUNK_BITS;        // When CHUNK_BITS is 64, ensure shift is within 0-63 (wraps for >64)
    if (s == 0) return value;          // No shift needed if s is zero
    // Shift right by s, then fill in the upper bits with the lower bits shifted out
    return (value >> s) | (value << (CHUNK_BITS - s));
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
 * int lod: The level of detail (LOD) that controls computational overhead. Valid values are 0 to 2.
 * * return 
 * FPSR_Output struct containing the random value and other details.
 * Output fields depend on the LOD level.
 * Refer to the FPSR_Output structure for details on the return values.
 *
 * float randVal: this is the main output, which is a float value between [0.0, 1.0]
 * when finalRandSwitch is 0: 
    * randVal will be a whole number representing the currently held   frame 
    * that remains constant for the hold duration.
 * when finalRandSwitch is 1: 
    * A float value between 0.0 and 1.0 that remains constant 
    * for the held duration.
 */
double fpsr_sm(
    int64_t frame, int64_t minHold, int64_t maxHold,
    int64_t reseedInterval, int64_t seedInner, int64_t seedOuter, int finalRandSwitch)
{
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    // Use floor-based modulo to match Python for negative frames.
    // Seed stays in integer domain for reproducibility.
    int64_t reseed_anchor = (seedInner + frame) - i64_floor_mod(frame, reseedInterval);

    // Deterministic PRNG over 64-bit integer seed; result is double in [0,1).
    double rand_for_duration = portable_rand_u64((uint64_t)reseed_anchor);

    // Compute duration with double intermediates then floor to int64.
    // Double math here mirrors Python's float behavior for cross-language parity.
    int64_t holdDuration = (int64_t)floor((double)minHold + rand_for_duration * (double)(maxHold - minHold));
    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // Align down using floor-mod semantics for negative inputs.
    int64_t held_integer_state = i64_align_down((seedOuter + frame), holdDuration);

    // --- 3. Use the stable state as a seed for the final random value ---
    // Keep all seed math in 64-bit integer space; rely on uint64 wraparound (well-defined).
    double fpsr_output = 0.0;
    if (finalRandSwitch) {
        uint64_t seed = (uint64_t)held_integer_state * 100000ULL;
        fpsr_output = portable_rand_u64(seed);
    } else {
        // Return the active stream value directly as a double (cast by caller if needed).
        fpsr_output = (double)held_integer_state; 
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
 * int frame: The current frame or time input.
 * int periodA: The first hold duration (in frames).
 * int periodB: The second hold duration (in frames).
 * int periodSwitch: The fixed interval at which the hold duration is toggled to switch between periodA and periodB.
 * int seedInner: An offset for the toggle clock to de-sync it from the main clock.
 * int seedOuter: An offset for the main clock to create unique sequences.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * return 
 * when finalRandSwitch is 0: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is 1: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */
double fpsr_tm(
    int64_t frame, int64_t periodA, int64_t periodB,
    int64_t periodSwitch, int64_t seedInner, int64_t seedOuter,
    int finalRandSwitch)
{
    // --- 1. Determine the hold duration by toggling between two periods ---
    if (periodSwitch < 1) { periodSwitch = 1; } // Prevent division by zero.

    // The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    int64_t inner_clock_frame = seedInner + frame;
    
    // Use floor-based modulo for cross-language consistency.
    int64_t r = i64_floor_mod(inner_clock_frame, periodSwitch);

    // Toggle threshold at exactly half the period using integer math.
    // Equivalent to: (r < 0.5 * periodSwitch) without floating-point rounding.
    int64_t holdDuration = (2 * r < periodSwitch) ? periodA : periodB;

    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // The "outer clock" is offset by seedOuter to create unique output sequences.
    int64_t outer_clock_frame = seedOuter + frame;
    int64_t held_integer_state = i64_align_down(outer_clock_frame, holdDuration);

    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    double fpsr_output;
    if (finalRandSwitch) {
        // Seed hashing in the 64-bit integer domain; well-defined wraparound.
        uint64_t seed = (uint64_t)held_integer_state * 100000ULL;
        fpsr_output = portable_rand_u64(seed);
    } else {
        // Return the raw integer state directly.
        fpsr_output = (double)held_integer_state; 
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
 * int frame: The current frame or time input.
 * float baseWaveFreq: The base frequency for the modulation wave of stream 1.
 * float stream2FreqMult: A multiplier for the second stream's frequency.
 * const int quantLevelsMinMax[]: An array of two integers for the min and max quantisation levels.
 * const int streamsOffset[]: An array of two integers to offset the frame for each stream's sine wave.
 * const int quantOffsets[]: An array of two integers to offset the random quantisation selection for each stream.
 * int streamSwitchDur: The number of frames after which the streams switch.
 * int stream1QuantDur: The duration for which stream 1's random quantisation level is held.
 * int stream2QuantDur: The duration for which stream 2's random quantisation level is held.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 */
double fpsr_qs(
    int64_t frame, double baseWaveFreq, double stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int64_t streamSwitchDur, int64_t stream1QuantDur, int64_t stream2QuantDur,
    int finalRandSwitch)
{
    // --- 1. Set default durations if not provided ---
    if (streamSwitchDur < 1) { streamSwitchDur = (int64_t)floor((1.0 / baseWaveFreq) * 0.76); }
    if (stream1QuantDur < 1) { stream1QuantDur = (int64_t)floor((1.0 / baseWaveFreq) * 1.2); }
    if (stream2QuantDur < 1) { stream2QuantDur = (int64_t)floor((1.0 / baseWaveFreq) * 0.9); }
    
    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate random quantisation levels for each stream ---
    int64_t quant_min = (int64_t)quantLevelsMinMax[0];
    int64_t quant_max = (int64_t)quantLevelsMinMax[1];
    int64_t quant_range = quant_max - quant_min + 1;

    // --- Stream 1 Quant Level ---
    int64_t s1_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[0] + frame, stream1QuantDur);
    double s1_rand_for_quant = portable_rand_u64((uint64_t)s1_quant_seed_aligned);
    int64_t s1_quant_level = quant_min + (int64_t)floor(s1_rand_for_quant * (double)quant_range);

    // --- Stream 2 Quant Level ---
    int64_t s2_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[1] + frame, stream2QuantDur);
    double s2_rand_for_quant = portable_rand_u64((uint64_t)s2_quant_seed_aligned);
    int64_t s2_quant_level = quant_min + (int64_t)floor(s2_rand_for_quant * (double)quant_range);

    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }

    // --- 3. Generate the two quantised sine wave streams ---
    if (stream2FreqMult < 0) { stream2FreqMult = 3.7; }

    // Ensure deterministic double math. sin() returns [-1,1]; map to [0,1] and quantise.
    double angle1 = ((double)streamsOffset[0] + (double)frame) * baseWaveFreq;
    double angle2 = ((double)streamsOffset[1] + (double)frame) * baseWaveFreq * stream2FreqMult;

    double stream1 = floor((sin(angle1) * 0.5 + 0.5) * (double)s1_quant_level) / (double)s1_quant_level;
    double stream2 = floor((sin(angle2) * 0.5 + 0.5) * (double)s2_quant_level) / (double)s2_quant_level;

    // --- 4. Switch between the two streams ---
    // Use floor-mod and an integer half-threshold (2*r < period) to match Python.
    double active_stream_val = 0.0;
    {
        int64_t r = i64_floor_mod(frame, streamSwitchDur);
        active_stream_val = (2 * r < streamSwitchDur) ? stream1 : stream2;
    }

    // --- 5. Hash the final output or bypass ---
    double fpsr_output;
    if (finalRandSwitch == 1) {
        // Derive a stable integer seed from the double stream using floor(), then hash.
        // This avoids ambiguous float->int casts and reproduces exactly in Python.
        int64_t hashed_int = (int64_t)floor(active_stream_val * 100000.0);
        fpsr_output = portable_rand_u64((uint64_t)hashed_int);
    } else {
        // Preserve original behavior (scaling applied by existing code path).
        fpsr_output = 0.5 * active_stream_val + 0.5;
    }
    return fpsr_output;
}

/******************************************************************************/
/* FPS-R: Bitwise Decode (BD)                                                 */
/******************************************************************************/

// Helper to get a specific bit from a chunk array, matching Python's out-of-bounds logic
static int get_bit(int64_t n, int64_t block_size, const uint64_t* chunks, int64_t num_chunks) {
    if (n < 0 || n >= block_size) return 0;
    // Bit ordering: chunk_index progresses block-wise, bit_index is LSB-first within a chunk.
    int64_t chunk_index = n / CHUNK_BITS;
    int bit_index = n % CHUNK_BITS;
    if (chunk_index >= num_chunks) return 0; // Should not happen with correct logic but safe
    return (chunks[chunk_index] >> bit_index) & 1;
}

/**
 * @brief Generates a phrased random value by decoding a deterministically generated bitstream.
 * @details This algorithm is stateless. For any given frame, it calculates its state by:
 * 1. Finding the start of its macro-block (`outer_anchor`).
 * 2. Generating one or more raw bitstreams for the block.
 * 3. Applying transformations (intra-stream op) to each stream, possibly in pairs for dynamic ops.
 * 4. Combining the transformed streams (inter-stream op).
 * 5. Decoding the final bitstream to produce phrased holds and jumps based on bit-flips.
 *
 * int64_t frame: The current frame or time input.
 * int64_t block_size: The size of the macro-rhythm in frames. Must be > 0.
 * int streams_number: The number of parallel bitstreams to generate.
 * int64_t streams_offset: The frame offset between each parallel stream's seed.
 * const char* intra_op: The unary (intra-stream) operation.
 *      Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
 *      Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
 * int dynamic_shift_bits: For dynamic ops, the number of controller bits to read
 *      to determine the shift/rotate amount (1-6 when chunk_bits=64).
 * int static_shift_amount: For static ops, the fixed number of bits to shift/rotate.
 * const char* inter_op: The binary (inter-stream) operation to combine multiple
 *      transformed streams. Options: "xor", "or", "and".
 * int64_t value_seed_offset: An additional seed offset for the final value calculation.
 * @return A deterministic, phrased pseudo-random double between 0.0 and 1.0.
 */
double fpsr_bd(
    int64_t frame,
    int64_t block_size,
    int streams_number,
    int64_t streams_offset,
    const char* intra_op,
    int dynamic_shift_bits,
    int static_shift_amount,
    const char* inter_op,
    int64_t value_seed_offset
) {
    if (block_size <= 0) block_size = 1;
    if (streams_number < 1) streams_number = 1;
    
    // --- Hardening: Sanitize static_shift_amount to prevent Undefined Behavior ---
    // This ensures the shift is always within the valid range [0, 63].
    int sanitized_static_shift = static_shift_amount & (CHUNK_BITS - 1);

    // --- Step 1: Find the Outer Anchor for the macro-block ---
    int64_t outer_anchor = i64_align_down(frame, block_size);
    int64_t num_chunks = (block_size + (CHUNK_BITS - 1)) / CHUNK_BITS;

    // --- Performance: Use stack allocation instead of heap allocation ---
    // This is much faster for a function called in a tight loop.
    // A safety check is added to fall back to malloc for unusually large blocks.
    uint64_t** raw_streams = NULL;
    uint64_t** transformed_streams = NULL;
    uint64_t* final_chunks = NULL;
    int use_heap = (block_size > BD_MAX_STACK_BLOCK_SIZE) || (streams_number > 32);

    size_t raw_streams_sz = streams_number * sizeof(uint64_t*);
    size_t transformed_streams_sz = streams_number * sizeof(uint64_t*);
    size_t chunks_sz = num_chunks * sizeof(uint64_t);

    if (use_heap) {
        raw_streams = (uint64_t**)malloc(raw_streams_sz);
        transformed_streams = (uint64_t**)malloc(transformed_streams_sz);
        final_chunks = (uint64_t*)malloc(chunks_sz);
        for (int i = 0; i < streams_number; ++i) {
            raw_streams[i] = (uint64_t*)malloc(chunks_sz);
            transformed_streams[i] = (uint64_t*)malloc(chunks_sz);
        }
    } else {
        raw_streams = (uint64_t**)alloca(raw_streams_sz);
        transformed_streams = (uint64_t**)alloca(transformed_streams_sz);
        final_chunks = (uint64_t*)alloca(chunks_sz);
        for (int i = 0; i < streams_number; ++i) {
            raw_streams[i] = (uint64_t*)alloca(chunks_sz);
            transformed_streams[i] = (uint64_t*)alloca(chunks_sz);
        }
    }
    
    // --- Safety: Initialize transformed_streams to zero ---
    for (int i = 0; i < streams_number; ++i) {
        memset(transformed_streams[i], 0, chunks_sz);
    }
    
    // --- Step 2: Generate the raw bitstream(s) for the entire block ---
    for (int i = 0; i < streams_number; ++i) {
        // Casting negative offsets to uint64_t is well-defined and deterministic.
        int64_t stream_seed = outer_anchor + (i * streams_offset);
        for (int j = 0; j < num_chunks; ++j) {
            raw_streams[i][j] = splitmix64((uint64_t)(stream_seed + j));
        }
    }
    
    // --- Step 3: Apply Intra-Stream Transformations ---
    int is_dynamic = (strcmp(intra_op, "lshift_dynamic") == 0 || strcmp(intra_op, "rshift_dynamic") == 0 ||
                      strcmp(intra_op, "rotl_dynamic") == 0 || strcmp(intra_op, "rotr_dynamic") == 0);

    int num_transformed_streams = is_dynamic ? (streams_number / 2 + streams_number % 2) : streams_number;

    if (is_dynamic) {
        // For dynamic ops, streams are processed in pairs (data, controller).
        for (int i = 0; i < streams_number / 2; ++i) {
            int data_idx = i * 2;
            int controller_idx = i * 2 + 1;
            int target_idx = i; // Simplified index for clarity

            uint64_t* data_stream = raw_streams[data_idx];
            uint64_t* controller_stream = raw_streams[controller_idx];
            
            // max_bits_for_shift is ceil(log2(CHUNK_BITS)), which is 6 for 64.
            int max_bits_for_shift = 6;
            int bit_mask_size = dynamic_shift_bits;
            if (bit_mask_size < 1) bit_mask_size = 1;
            if (bit_mask_size > max_bits_for_shift) bit_mask_size = max_bits_for_shift;
            uint64_t bit_mask = (1ULL << bit_mask_size) - 1;

            for (int j = 0; j < num_chunks; ++j) {
                int dynamic_shift = (controller_stream[j] & bit_mask); // Shift amount is derived from controller
                if (strcmp(intra_op, "lshift_dynamic") == 0) transformed_streams[target_idx][j] = data_stream[j] << (dynamic_shift % CHUNK_BITS);
                else if (strcmp(intra_op, "rshift_dynamic") == 0) transformed_streams[target_idx][j] = data_stream[j] >> (dynamic_shift % CHUNK_BITS);
                else if (strcmp(intra_op, "rotl_dynamic") == 0) transformed_streams[target_idx][j] = u64_circular_left_shift(data_stream[j], dynamic_shift);
                else if (strcmp(intra_op, "rotr_dynamic") == 0) transformed_streams[target_idx][j] = u64_circular_right_shift(data_stream[j], dynamic_shift);
            }
        }
        // If there's an odd number of streams, the last one is unpaired and copied directly.
        if (streams_number % 2 != 0) {
            int last_raw_idx = streams_number - 1;
            int last_target_idx = num_transformed_streams - 1;
            memcpy(transformed_streams[last_target_idx], raw_streams[last_raw_idx], chunks_sz);
        }
    } else { // Static operations
        for (int i = 0; i < streams_number; ++i) {
            for (int j = 0; j < num_chunks; ++j) {
                if (strcmp(intra_op, "not") == 0) transformed_streams[i][j] = ~raw_streams[i][j];
                else if (strcmp(intra_op, "lshift") == 0) transformed_streams[i][j] = raw_streams[i][j] << sanitized_static_shift;
                else if (strcmp(intra_op, "rshift") == 0) transformed_streams[i][j] = raw_streams[i][j] >> sanitized_static_shift;
                else if (strcmp(intra_op, "rotl") == 0) transformed_streams[i][j] = u64_circular_left_shift(raw_streams[i][j], sanitized_static_shift);
                else if (strcmp(intra_op, "rotr") == 0) transformed_streams[i][j] = u64_circular_right_shift(raw_streams[i][j], sanitized_static_shift);
                else transformed_streams[i][j] = raw_streams[i][j]; // "none"
            }
        }
    }
    
    // --- Step 4: Combine Streams with Inter-Stream Operation ---
    if (num_transformed_streams > 0) {
        memcpy(final_chunks, transformed_streams[0], chunks_sz);
        for (int i = 1; i < num_transformed_streams; ++i) {
            for (int j = 0; j < num_chunks; ++j) {
                if (strcmp(inter_op, "or") == 0) final_chunks[j] |= transformed_streams[i][j];
                else if (strcmp(inter_op, "and") == 0) final_chunks[j] &= transformed_streams[i][j];
                else final_chunks[j] ^= transformed_streams[i][j]; // "xor" is default
            }
        }
    } else {
        memset(final_chunks, 0, chunks_sz);
    }
    
    // --- Step 5: Decode the final bitstream ---
    int64_t current_pos_in_block = frame - outer_anchor;
    int64_t last_flip_pos = 0;
    // Scan backwards from the current frame's position to find the most recent bit flip.
    // If no flip is found (constant bitstream), last_flip_pos remains 0.
    // The hold is defined by the distance to this last flip.
    for (int64_t i = current_pos_in_block; i > 0; --i) {
        if (get_bit(i, block_size, final_chunks, num_chunks) != get_bit(i - 1, block_size, final_chunks, num_chunks)) {
            last_flip_pos = i;
            break;
        }
    }
    
    // --- Step 6: Generate the final random value from the last bit-flip position ---
    // The seed is a combination of the block start, the position of the last flip,
    // and a user-provided offset, ensuring a unique value for each held segment.
    uint64_t final_seed = (uint64_t)outer_anchor + (uint64_t)last_flip_pos + (uint64_t)value_seed_offset;
    double result = portable_rand_u64(final_seed);

    // --- Cleanup for heap allocation ---
    if (use_heap) {
        for (int i = 0; i < streams_number; ++i) {
            free(raw_streams[i]);
            free(transformed_streams[i]);
        }
        free(raw_streams);
        free(transformed_streams);
        free(final_chunks);
    }
    
    return result;
}

/******************************************************************************/
/* Main function to demonstrate usage of FPS-R algorithms                     */
/******************************************************************************/
int main() {
    // Write C code here
    // printf("Try programiz.pro\n");
 
    // algorithms: 0 - sm, 1 - tm, 2 - qs, 3 - bd
    int algo = 3; // Change this value to 0, 1, 2, or 3 to test different algorithms
    char algo_name[][3] = {"SM", "TM", "QS", "BD"}; // Names for the algorithms
    printf("Using algorithm FPS-R: %s\n", algo_name[algo]);

    int start_frames[] = {90, 100, 103, 100}; // starting frames for each algorithm
    int num_frames = 30; // run a loop of x frames to demonstrate changes
    
    // create main for loop to demonstrate changes
    for (int loop_frame = 0; loop_frame < num_frames; loop_frame++) {
        // printf("Frame %d\n", i);
    
        int frame = loop_frame + start_frames[algo]; // starting frame for the selected algorithm
        float randVal = 0.0; // variable to hold the random value output
        float randVal_previous = 0.0; // variable to hold the previous frame's random value
        int changed = 0; // Variable to track if the value has changed

        if (algo == 0) {
            // --------------------------------------------------------------------------
            // Sample code to call the FPS-R:SM function
            // --------------------------------------------------------------------------
            // Parameters
            // int frame = 90; // Replace with the current frame value
            int minHoldFrames = 10; // probable minimum held period
            int maxHoldFrames = 13; // maximum held period before cycling
            int reseedFrames = 5; // inner mod cycle timing
            int offsetInner = -34; // offsets the inner frame
            int offsetOuter = 22; // offsets the outer frame
            int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it
            
            // Call the FPS-R:SM function
            // call to fpsr_sm for the current frame
            randVal = 
                (float)fpsr_sm(
                    (int64_t)frame, (int64_t)minHoldFrames, (int64_t)maxHoldFrames, 
                    (int64_t)reseedFrames, (int64_t)offsetInner, (int64_t)offsetOuter, finalRandSwitch);
            // another call to fpsr_sm for the previous frame
            randVal_previous = 
                (float)fpsr_sm(
                    (int64_t)(frame - 1), (int64_t)minHoldFrames, (int64_t)maxHoldFrames, 
                    (int64_t)reseedFrames, (int64_t)offsetInner, (int64_t)offsetOuter, finalRandSwitch);
            changed = 0;
            if (randVal != randVal_previous) {
                changed = 1; // value has changed from the previous frame
            }
        }
        
        else if (algo == 1) {
            // --------------------------------------------------------------------------
            // Sample code to call the FPS-R:TM function
            // --------------------------------------------------------------------------
            // Parameters
            // int frame = 100; // Replace with the current frame value
            int period_A = 10; // The first hold duration
            int period_B = 16; // The second hold duration
            int periodSwitch = 9; // The toggle happens every 30 frames
            int offset_inner = 4; // offsets the inner (toggle) clock
            int offset_outer = 0; // offsets the outer (hold) clock
            int final_rand_switch = 1; // 1 to apply the final randomisation step, 0 to skip it
            
            // Call the FPS-R:TM function
            // call to fpsr_tm for the current frame
            randVal = 
                (float)fpsr_tm(
                    (int64_t)frame, (int64_t)period_A, (int64_t)period_B, 
                    (int64_t)periodSwitch, (int64_t)offset_inner, (int64_t)offset_outer, final_rand_switch);
            // another call to fpsr_tm for the previous frame
            randVal_previous = 
                (float)fpsr_tm(
                    (int64_t)(frame - 1), (int64_t)period_A, (int64_t)period_B, 
                    (int64_t)periodSwitch, (int64_t)offset_inner, (int64_t)offset_outer, final_rand_switch);
            changed = 0;
            if (randVal != randVal_previous) {
                changed = 1; // value has changed from the previous frame
            }
        }
        
        else if (algo == 2) {
            // --------------------------------------------------------------------------
            // Sample code to call the FPS-R:QS function
            // --------------------------------------------------------------------------
            // Parameters
            // int frame = 103; // Current frame number
            float baseWaveFreq = 0.012; // Base frequency for the modulation wave of stream 1
            float stream2freqMult = 3.1; // Multiplier for the second stream's frequency
            int quantLevelsMinMax[2] = {4, 12}; // Min, Max quantisation levels for the two streams
            int streamsOffset[2] = {0, 72}; // Offset for the two streams
            int quantOffsets[2] = {9, 81}; // Offset for the random quantisation selection
            int streamSwitchDur = 11; // Duration for switching streams in frames
            int stream1QuantDur = 13; // Duration for the first stream's quantisation switch cycle in frames
            int stream2QuantDur = 20; // Duration for the second stream's quantisation switch cycle in frames
            int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it
            
            // call to fpsr_qs for the current frame
            randVal = (float)fpsr_qs(
                (int64_t)frame, (double)baseWaveFreq, (double)stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch);
            // another call to fpsr_qs for the previous frame
            randVal_previous = (float)fpsr_qs(
                (int64_t)(frame - 1), (double)baseWaveFreq, (double)stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch);
            changed = 0; // Variable to track if the value has changed
            if (randVal != randVal_previous) {
                changed = 1; // Mark as changed if the value has changed from the previous frame
            }
        }
        
        else if (algo == 3) {
            // --------------------------------------------------------------------------
            // Sample code to call the FPS-R:BD function
            // --------------------------------------------------------------------------
            // Parameters
            int64_t p_block_size = 64; // size of the macro-rhythm in frames
            int p_streams_number = 2; // number of parallel bitstreams to generate
            int64_t p_streams_offset = 10; // frame offset between each parallel stream's seed
            const char* p_intra_op = "rotl_dynamic"; // The unary (intra-stream) operation.
                // p_intra_op modes:
                //      Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
                //      Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
            int p_dynamic_shift_bits = 6; // number of controller bits to read for dynamic ops
            int p_static_shift_amount = 1; // fixed number of bits to shift/rotate for static ops
            const char* p_inter_op = "xor"; // inter-stream operation to combine transformed streams
                // p_inter_op modes:
                //      The binary (inter-stream) operation to combine multiple
                //      transformed streams. Options: "xor", "or", "and".
            int64_t p_value_seed_offset = 78901; // additional seed offset for the final value calculation

            randVal = (float)fpsr_bd(
                (int64_t)frame, p_block_size, p_streams_number, p_streams_offset,
                p_intra_op, p_dynamic_shift_bits, p_static_shift_amount,
                p_inter_op, p_value_seed_offset
            );

            randVal_previous = (float)fpsr_bd(
                (int64_t)(frame - 1), p_block_size, p_streams_number, p_streams_offset,
                p_intra_op, p_dynamic_shift_bits, p_static_shift_amount,
                p_inter_op, p_value_seed_offset
            );

            if (randVal != randVal_previous) {
                changed = 1;
            }
        }

        printf("Frame %d: randVal %f, randVal_previous %f, changed %d ", 
                frame, randVal, randVal_previous, changed);
        printf("%s\n", (changed ? "(jumped)" : ""));
    } // end of main for loop
    return 0;
}

