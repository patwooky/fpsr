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
 * @details This file contains four stateless, Frame-Persistent Stateless Randomisation algorithms.
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

// --- Platform-Specific Includes for Thread-Safe Init ---
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    #include <pthread.h>
#else
    #warning "Unsupported platform: Thread-safe LUT initialization is not available. Falling back to non-thread-safe."
    // Define a fallback for unsupported platforms
    #define UNSUPPORTED_PLATFORM
#endif

#define SINE_LUT_SIZE_100 100
#define SINE_LUT_SIZE_500 500
#define SINE_LUT_SIZE_1000 1000
#define SINE_LUT_SIZE_4096 4096 // Highest precision default

// Global constant for 2*PI (double precision)
#define TWO_PI 6.28318530718

// Global sine lookup tables
static double _sine_lut_100[SINE_LUT_SIZE_100];
static double _sine_lut_500[SINE_LUT_SIZE_500];
static double _sine_lut_1000[SINE_LUT_SIZE_1000];
static double _sine_lut_4096[SINE_LUT_SIZE_4096]; // Highest precision default

// Forward declaration of the initialization function
void initialize_sine_luts(void);

// --- Thread-Safe "Call Once" Implementation ---
#if defined(_WIN32) || defined(_WIN64)
    // Windows implementation
    static int os = 0;
    static INIT_ONCE init_once_control = INIT_ONCE_STATIC_INIT;
    
    // Windows requires a specific callback signature
    BOOL CALLBACK InitLutsCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
        initialize_sine_luts();
        return TRUE;
    }

    static inline void init_once_func(void) {
        InitOnceExecuteOnce(&init_once_control, InitLutsCallback, NULL, NULL);
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    // POSIX (Linux, macOS, etc.) implementation
    static int os = 1;
    static pthread_once_t init_once_control = PTHREAD_ONCE_INIT;

    static inline void init_once_func(void) {
        pthread_once(&init_once_control, initialize_sine_luts);
    }

#else
    // Fallback implementation for unsupported platforms
    static int _luts_initialized_fallback = 0;
    static inline void init_once_func(void) {
        if (!_luts_initialized_fallback) {
            initialize_sine_luts();
            _luts_initialized_fallback = 1;
        }
    }
#endif

/**
 * @brief Initializes all global sine lookup tables.
 * @details This function is designed to be called exactly once, in a
 * thread-safe manner, by the init_once_func().
 * THIS FUNCTION MUST BE CALLED ONCE AT PROGRAM STARTUP (automatically handled by init_once_func)!
 */
void initialize_sine_luts(void) {
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
}

// Helper function to get sine value from a specific LUT with linear interpolation
double _get_sine_from_lod_lut(double phase, int lut_size, const double* lut_array) {
    /*
        * @brief Gets a sine value from a specific LUT with linear interpolation.
        * @param phase The input phase angle in radians.
        * @param lut_size The size of the lookup table.
        * @param lut_array Pointer to the sine lookup table array.
        * @return The interpolated sine value corresponding to the input phase.
    */

    // On unsupported platforms, fall back to direct sin() to avoid non-thread-safe LUT init.
    #if defined(UNSUPPORTED_PLATFORM)
        // Fallback or error if LUTs not initialized.
        return sin(phase); // Fallback
    #endif

    // 1. GUARANTEED THREAD-SAFE CALL
    // This will run initialize_sine_luts() on the first call across all threads
    // and will do nothing on subsequent calls.
    init_once_func();

    // 2. Interpolation logic
    // Wrap phase to 0 to 2*PI range
    phase = fmod(phase, TWO_PI);
    if (phase < 0) phase += TWO_PI; // Ensure positive for fmod results

    // Map phase to LUT index range
    double fractional_index = phase / TWO_PI * lut_size;
    
    // Get integer part and fractional part
    int index1 = (int)floor(fractional_index);
    double frac = fractional_index - index1;
    
    // Handle wrap-around for index2 (last point wraps to first)
    // Guard against index1 being exactly lut_size (when frac is 0.0)
    if (index1 >= lut_size) index1 = 0;
    int index2 = (index1 + 1) % lut_size;

    // Linear interpolation
    return lut_array[index1] * (1.0 - frac) + lut_array[index2] * frac;
}

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
 * @brief Back-compat wrapper: generates a deterministic float in [0, 1] from an integer seed.
 * @details This now forwards to the 64-bit deterministic PRNG above to guarantee parity with Python.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
static inline float portable_rand(int seed) {
    // Converts the given seed to a 64-bit unsigned integer, generates a pseudo-random number using portable_rand_u64,
    // and casts the result to a float for return.
    // Keep seeding strictly in integer domain; cast via int64_t to preserve sign.
    return (float)portable_rand_u64((uint64_t)(int64_t)seed);
}

// --- Bitwise Rotation Helpers ---
// Performs a circular (rotate) left shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_left_shift(uint64_t value, int shift) {
    // The modulo operator ensures the shift amount is always within [0, CHUNK_BITS-1],
    // preventing undefined behavior from shifts >= the type's bit-width.
    int s = shift % CHUNK_BITS;
    if (s == 0) return value;
    // The C standard guarantees that for unsigned types, right-shift is a logical shift
    // (fills with zeros), which is the correct behavior for rotation.
    return (value << s) | (value >> (CHUNK_BITS - s));
}

// Performs a circular (rotate) right shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_right_shift(uint64_t value, int shift) {
    // The modulo operator ensures the shift amount is always within [0, CHUNK_BITS-1],
    // preventing undefined behavior.
    int s = shift % CHUNK_BITS;
    if (s == 0) return value;
    // The right-shift on the unsigned 'value' is a well-defined logical shift.
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
double fpsr_sm_base(
    int64_t frame, int64_t minHold, int64_t maxHold,
    int64_t reseedInterval, int64_t seedInner, int64_t seedOuter, int finalRandSwitch)
{
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    // Use floor-based modulo to match Python for negative frames.
    // Seed stays in integer domain for reproducibility.
    int64_t reseed_anchor = (seedInner + frame) - i64_floor_mod(frame, reseedInterval);

    // Deterministic PRNG over 64-bit integer seed; result is double in [0,1].
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
        // The held_integer_state is already the unique identifier for this hold segment.
        // We pass it directly to the SplitMix64 hasher without needing an additional multiplier.
        uint64_t seed = (uint64_t)held_integer_state;
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
double fpsr_tm_base(
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
        // The held_integer_state is the unique identifier for the hold segment.
        // Pass it directly to the SplitMix64 hasher for a well-distributed random value.
        uint64_t seed = (uint64_t)held_integer_state;
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
 * @brief Generates a quantized sine-based persistent random value using two streams.
 * @details This function creates two sine wave streams with configurable frequencies
 * and offsets. For each stream, a new random quantisation level is chosen 
 * from within the [min, max] range at a set interval, and the output alternates
 * between the two streams based on a defined switch duration. The final output can
 * optionally be further randomized.
 *
 * int64_t frame: The current frame or time input.
 * double baseWaveFreq: The base frequency for the sine waves.
 * double stream2FreqMult: A multiplier for the second stream's frequency.
 * const int quantLevelsMinMax[2]: An array defining the minimum and maximum quantization levels.
 * const int streamsOffset[2]: An array defining the phase offsets for each sine stream.
 * const int quantOffsets[2]: An array defining the quantization seed offsets for each stream.
 * int64_t streamSwitchDur: The duration (in frames) before switching between streams.
 * int64_t stream1QuantDur: The quantization duration (in frames) for stream 1.
 * int64_t stream2QuantDur: The quantization duration (in frames) for stream 2.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * int sine_lod_level: Level of detail for sine calculation (0=direct, 1=LUT100, 2=LUT500, 3=LUT1000, 4=LUT4096).
 * @return A deterministic, pseudo-random double between 0.0 and 1.0.
 */
double fpsr_qs(
    int64_t frame, double baseWaveFreq, double stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int64_t streamSwitchDur, int64_t stream1QuantDur, int64_t stream2QuantDur,
    int finalRandSwitch, int sine_lod_level)
{
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
    if (stream2FreqMult <= 0) { stream2FreqMult = 3.7; }
    
    // Ensure deterministic double math. sin() returns [-1,1]; map to [0,1] and quantise.
    double angle1 = ((double)streamsOffset[0] + (double)frame) * baseWaveFreq;
    double angle2 = ((double)streamsOffset[1] + (double)frame) * baseWaveFreq * stream2FreqMult;
    
    double stream1_raw_sine, stream2_raw_sine;
    switch (sine_lod_level) {
        case 0: // Direct sin() call
            stream1_raw_sine = sin(angle1);
            stream2_raw_sine = sin(angle2);
            break;
        case 1: // LUT 100
            stream1_raw_sine = _get_sine_from_lod_lut(angle1, SINE_LUT_SIZE_100, _sine_lut_100);
            stream2_raw_sine = _get_sine_from_lod_lut(angle2, SINE_LUT_SIZE_100, _sine_lut_100);
            break;
        case 2: // LUT 500
            stream1_raw_sine = _get_sine_from_lod_lut(angle1, SINE_LUT_SIZE_500, _sine_lut_500);
            stream2_raw_sine = _get_sine_from_lod_lut(angle2, SINE_LUT_SIZE_500, _sine_lut_500);
            break;
        case 3: // LUT 1000
            stream1_raw_sine = _get_sine_from_lod_lut(angle1, SINE_LUT_SIZE_1000, _sine_lut_1000);
            stream2_raw_sine = _get_sine_from_lod_lut(angle2, SINE_LUT_SIZE_1000, _sine_lut_1000);
            break;
        case 4: default: // LUT 4096 (highest precision)
            stream1_raw_sine = _get_sine_from_lod_lut(angle1, SINE_LUT_SIZE_4096, _sine_lut_4096);
            stream2_raw_sine = _get_sine_from_lod_lut(angle2, SINE_LUT_SIZE_4096, _sine_lut_4096);
            break;
    }
    
    // Map sine from [-1,1] to [0,1] before quantizing
    double stream1 = floor((stream1_raw_sine * 0.5 + 0.5) * (double)s1_quant_level) / (double)s1_quant_level;
    double stream2 = floor((stream2_raw_sine * 0.5 + 0.5) * (double)s2_quant_level) / (double)s2_quant_level;
    
    // --- 4. Switch between the two streams based on streamSwitchDur ---
    int64_t r = i64_floor_mod(frame, streamSwitchDur);
    double active_stream_val = (2 * r < streamSwitchDur) ? stream1 : stream2;

    // --- 5. Hash the final output or bypass ---
    // If final randomisation is enabled, derive a deterministic seed from the active stream value
    double fpsr_output;
    if (finalRandSwitch == 1) {
        // --- FIX: Scale the quantized value to preserve level information ---
        // The active_stream_val is in [0, 1] and quantized to discrete levels.
        // We need to scale it to a larger range before flooring to get distinct seeds.
        // Using a large multiplier (e.g., 1000000) ensures different quantization levels
        // produce different integer seeds.
        int64_t hashed_int = (int64_t)floor(active_stream_val * 1000000.0);
        fpsr_output = portable_rand_u64((uint64_t)hashed_int);
    } else {
        // --- FIX: Correct range scaling ---
        // active_stream_val is already mapped to [0, 1].
        // The previous calculation incorrectly remapped it to [0.5, 1.0].
        // Assign directly to use the full [0, 1] range.
        fpsr_output = active_stream_val;
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

    // --- Safer Memory Allocation with comprehensive overflow checks ---
    size_t chunk_data_sz = num_chunks * sizeof(uint64_t);

    // --- FIX: Added comprehensive overflow checks ---
    // Check multiplication for total_chunk_data_sz
    if (streams_number > 0 && (SIZE_MAX / (size_t)streams_number < chunk_data_sz)) {
        fprintf(stderr, "ERROR in fpsr_bd: Overflow calculating total_chunk_data_sz. Returning 0.0.\n");
        return 0.0; // Overflow
    }
    size_t total_chunk_data_sz = (size_t)streams_number * chunk_data_sz;

    // Check multiplication for ptr_arrays_sz
    if ((SIZE_MAX / sizeof(uint64_t*)) / 2 < (size_t)streams_number) {
        fprintf(stderr, "ERROR in fpsr_bd: Overflow calculating ptr_arrays_sz. Returning 0.0.\n");
        return 0.0; // Overflow check
    }
    size_t ptr_arrays_sz = (size_t)streams_number * sizeof(uint64_t*) * 2; // raw_streams pointers + transformed_streams pointers

    // Check additions for total_alloc_size
    if (SIZE_MAX - ptr_arrays_sz < chunk_data_sz) {
        fprintf(stderr, "ERROR in fpsr_bd: Overflow calculating total_alloc_size (step 1). Returning 0.0.\n");
        return 0.0; // Overflow check
    }
    size_t temp_size = ptr_arrays_sz + chunk_data_sz; // Size for pointers + final_chunks
    if (SIZE_MAX - temp_size < 2 * total_chunk_data_sz) {
        fprintf(stderr, "ERROR in fpsr_bd: Overflow calculating total_alloc_size (step 2). Returning 0.0.\n");
        return 0.0; // Overflow check (adding space for raw_streams + transformed_streams)
    }
    size_t total_alloc_size = temp_size + (2 * total_chunk_data_sz); // final calculation

    void* buffer_base = malloc(total_alloc_size);
    if (!buffer_base) { 
        fprintf(stderr, "ERROR in fpsr_bd: malloc failed to allocate %zu bytes. Returning 0.0.\n", total_alloc_size);
        return 0.0; // Allocation failed, return neutral value.
    } 
    
    // Carve up the single buffer into the pointer arrays and per-stream chunk arrays.
    uint8_t* p = (uint8_t*)buffer_base;
    uint64_t** raw_streams = (uint64_t**)p;
    p += streams_number * sizeof(uint64_t*);
    uint64_t** transformed_streams = (uint64_t**)p;
    p += streams_number * sizeof(uint64_t*);
    
    for (int i = 0; i < streams_number; ++i) {
        raw_streams[i] = (uint64_t*)p;
        p += chunk_data_sz;
    }
    for (int i = 0; i < streams_number; ++i) {
        transformed_streams[i] = (uint64_t*)p;
        p += chunk_data_sz;
    }
    uint64_t* final_chunks = (uint64_t*)p;

    // --- Safety: Initialize transformed_streams to zero ---
    for (int i = 0; i < streams_number; ++i) {
        memset(transformed_streams[i], 0, chunk_data_sz);
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
            memcpy(transformed_streams[last_target_idx], raw_streams[last_raw_idx], chunk_data_sz);
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
        memcpy(final_chunks, transformed_streams[0], chunk_data_sz);
        for (int i = 1; i < num_transformed_streams; ++i) {
            for (int j = 0; j < num_chunks; ++j) {
                if (strcmp(inter_op, "or") == 0) final_chunks[j] |= transformed_streams[i][j];
                else if (strcmp(inter_op, "and") == 0) final_chunks[j] &= transformed_streams[i][j];
                else final_chunks[j] ^= transformed_streams[i][j]; // "xor" is default
            }
        }
    } else {
        memset(final_chunks, 0, chunk_data_sz);
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
    free(buffer_base);
    
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
            // int frame = 90;       // Replace with the current frame value
            int minHoldFrames = 7;   // probable minimum held period
            int maxHoldFrames = 9;   // maximum held period before cycling
            int reseedFrames = 6;    // inner mod cycle timing
            int offsetInner = -41;   // offsets the inner frame
            int offsetOuter = 23;    // offsets the outer frame
            int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it
            
            // Call the FPS-R:SM function
            // call to fpsr_sm for the current frame
            randVal = 
                (float)fpsr_sm_base(
                    (int64_t)frame, (int64_t)minHoldFrames, (int64_t)maxHoldFrames, 
                    (int64_t)reseedFrames, (int64_t)offsetInner, (int64_t)offsetOuter, finalRandSwitch);
            // another call to fpsr_sm for the previous frame
            randVal_previous = 
                (float)fpsr_sm_base(
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
            // int frame = 100;        // Replace with the current frame value
            int period_A = 8;          // The first hold duration
            int period_B = 5;          // The second hold duration
            int periodSwitch = 6;      // The toggle happens every 30 frames
            int offset_inner = 15;     // offsets the inner (toggle) clock
            int offset_outer = 0;      // offsets the outer (hold) clock
            int final_rand_switch = 1; // 1 to apply the final randomisation step, 0 to skip it
            
            // Call the FPS-R:TM function
            // call to fpsr_tm for the current frame
            randVal = 
                (float)fpsr_tm_base(
                    (int64_t)frame, (int64_t)period_A, (int64_t)period_B, 
                    (int64_t)periodSwitch, (int64_t)offset_inner, (int64_t)offset_outer, final_rand_switch);
            // another call to fpsr_tm for the previous frame
            randVal_previous = 
                (float)fpsr_tm_base(
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
            float baseWaveFreq = 0.012f;  // Base frequency for the modulation wave of stream 1
            float stream2freqMult = 3.1f; // Multiplier for the second stream's frequency
            int quantLevelsMinMax[2] = {4, 12}; // Min, Max quantisation levels for the two streams
            int streamsOffset[2] = {0, 76}; // Offset for the two streams
            int quantOffsets[2] = {10, 81}; // Offset for the random quantisation selection
            int streamSwitchDur = 8; // Duration for switching streams in frames
            int stream1QuantDur = 10; // Duration for the first stream's quantisation switch cycle in frames
            int stream2QuantDur = 13; // Duration for the second stream's quantisation switch cycle in frames
            int finalRandSwitch = 1; // 1 to apply the final randomisation step, 0 to skip it
            int sine_lod_level = 4; // Sine wave LOD level (0=direct sin, 1=LUT100, 2=LUT500, 3=LUT1000, 4=LUT4096)
            
            // call to fpsr_qs for the current frame
            randVal = (float)fpsr_qs(
                (int64_t)frame, (double)baseWaveFreq, (double)stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, sine_lod_level);
            
            // another call to fpsr_qs for the previous frame
            randVal_previous = (float)fpsr_qs(
                (int64_t)(frame - 1), (double)baseWaveFreq, (double)stream2freqMult, quantLevelsMinMax, 
                streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, sine_lod_level);
            
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

