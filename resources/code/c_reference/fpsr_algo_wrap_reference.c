// SPDX-License-Identifier: MIT — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/fpsr

/**
 * @file fpsr_wrapped.c
 * @brief This file demonstrates a wrapper-based approach for getting rich metadata
 * from the core FPS-R algorithms.
 * @details This implementation now contains the pure, stateless algorithms. The wrapper
 * functions perform a robust, two-phase search (exponential probe + binary search) to
 * populate the FPSR_Output struct. This method is highly efficient and avoids
 * "false positive" value collisions.
 *
 * This version includes the "Hierarchical Coherence" wrapper logic, which
 * implements a "fractal zoom" for time scaling (frame_multiplier).
 */

 #include <math.h> // For sin(), floor(), ceil(), log2(), fabs()
 #include <stdio.h> // For NULL, printf
 #include <stdint.h> // For deterministic 64-bit integer types (int64_t, uint64_t)
 #include <stdlib.h> // For malloc(), free(), size_t
 #include <string.h> // For strcmp(), memset(), memcpy()
 
 #if defined(_MSC_VER)
 #include <malloc.h> // For _alloca
 #define alloca _alloca
 #elif defined(__GNUC__) || defined(__clang__)
 #include <alloca.h> // For alloca
 #endif
 
 // Bit-width used for chunked bit operations.
 // It must remain 64 for deterministic compatibility with SplitMix64. DO NOT MODIFY.
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
 
 /******************************************************************************/
 /* Core Components (Deterministic PRNG and Integer Math)                      */
 /******************************************************************************/
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
 static inline int64_t i64_floor_mod(int64_t a, int64_t m) {
     int64_t r = a % m;
     if (r < 0) r += m;
     return r;
 }
 
 // Align-down to the nearest multiple of m using floor-mod semantics.
 static inline int64_t i64_align_down(int64_t a, int64_t m) {
     return a - i64_floor_mod(a, m);
 }
 
 // SplitMix64: simple, robust 64-bit mixer using well-defined uint64_t wraparound.
 static inline uint64_t splitmix64(uint64_t x) {
     x += 0x9E3779B97F4A7C15ULL;
     x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
     x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
     x ^= (x >> 31);
     return x;
 }
 
 // Map a uint64_t to a double in [0,1) by taking the top 53 bits (double mantissa width).
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
     return (float)portable_rand_u64((uint64_t)(int64_t)seed);
 }
 
 // --- Bitwise Rotation Helpers ---
 // Circular left shift for uint64_t
 static inline uint64_t u64_circular_left_shift(uint64_t value, int shift) {
     int s = shift % CHUNK_BITS;
     if (s == 0) return value;
     return (value << s) | (value >> (CHUNK_BITS - s));
 }
 
 // Circular right shift for uint64_t
 static inline uint64_t u64_circular_right_shift(uint64_t value, int shift) {
     int s = shift % CHUNK_BITS;
     if (s == 0) return value;
     return (value >> s) | (value << (CHUNK_BITS - s));
 }
 
 // Helper to get a specific bit from a chunk array, matching Python's out-of-bounds logic
 static int get_bit(int64_t n, int64_t block_size, const uint64_t* chunks, int64_t num_chunks) {
     if (n < 0 || n >= block_size) return 0;
     // Bit ordering: chunk_index progresses block-wise, bit_index is LSB-first within a chunk.
     int64_t chunk_index = n / CHUNK_BITS;
     int bit_index = n % CHUNK_BITS;
     if (chunk_index >= num_chunks) return 0; // Should not happen with correct logic but safe
     return (chunks[chunk_index] >> bit_index) & 1;
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
 * int randVal_previous: LOD 1, 2. The random value from the previous frame for change detection.
 * float hold_progress: LOD 2. The progress of the hold duration, normalised to [0, 1].
 * int last_changed_frame: LOD 2. The precise frame (integer) when the random value last changed.
 * int next_changed_frame: LOD 2. The precise frame (integer) when the random value will next change.
 * float randVal_next_changed_frame: LOD 2. The value that the algorithm will jump to at next_changed_frame.
 * double randStreams[2]: LOD 2. (Exclusive to QS) The raw values of stream1_double and stream2_double.
 * int selected_stream: LOD 2. (Exclusive to QS) The index of the stream (0 for stream1, 1 for stream2) that was selected by the algorithm.
 * */
 typedef struct {
     float randVal; // The random value output of FPS-R algorithm. (LOD 0,1,2)
     int has_changed; // Flag indicating if randVal changed from previous frame. (LOD 1,2)
     float randVal_previous; // The random value from the previous frame. (LOD 1,2)
     float hold_progress; // Normalised progress of the hold duration [0,1]. (LOD 2)
     int last_changed_frame; // The frame when randVal last changed. (LOD 2)
     int next_changed_frame; // The frame when randVal will next change. (LOD 2)
     float randVal_next_changed_frame; // The value at next_changed_frame. (LOD 2)
     // New fields for QS details (LOD 2)
     double randStreams[2]; // stream1_double and stream2_double raw values (LOD 2)
     int selected_stream_idx; // 0 for stream1, 1 for stream2 (LOD 2)
 } FPSR_Output;
 
 /******************************************************************************/
 /* Pure, Canonical FPS-R Algorithms                                           */
 /******************************************************************************/
 // These functions are the pure, canonical reference implementations. They operate
 // on a 64-bit integer timeline for absolute determinism.
 
 //-----------------------------------------------------------------------------/
 // FPS-R: Stacked Modulo (SM)                                                  /
 //-----------------------------------------------------------------------------/
 // The pure 'base' version of SM for the wrapper. It returns just the float value.
 /*
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
     if (reseedInterval < 1) { reseedInterval = 1; }
 
     int64_t reseed_anchor = (seedInner + frame) - i64_floor_mod(frame, reseedInterval);
     double rand_for_duration = portable_rand_u64((uint64_t)reseed_anchor);
     int64_t holdDuration = (int64_t)floor((double)minHold + rand_for_duration * (double)(maxHold - minHold));
     if (holdDuration < 1) { holdDuration = 1; }
 
     int64_t held_integer_state = i64_align_down((seedOuter + frame), holdDuration);
 
     double fpsr_output = 0.0;
     if (finalRandSwitch) {
         uint64_t seed = (uint64_t)held_integer_state * 100000ULL;
         fpsr_output = portable_rand_u64(seed);
     } else {
         fpsr_output = (double)held_integer_state; 
     }
     return fpsr_output;
 }
 
 //-----------------------------------------------------------------------------/
 // FPS-R: Toggle Modulo (TM)                                                   /
 //-----------------------------------------------------------------------------/
 // This is the pure 'base' version of TM for the wrapper. It returns just the float value.
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
     if (periodSwitch < 1) { periodSwitch = 1; }
 
     int64_t inner_clock_frame = seedInner + frame;
     int64_t r = i64_floor_mod(inner_clock_frame, periodSwitch);
     int64_t holdDuration = (2 * r < periodSwitch) ? periodA : periodB;
     if (holdDuration < 1) { holdDuration = 1; }
 
     int64_t outer_clock_frame = seedOuter + frame;
     int64_t held_integer_state = i64_align_down(outer_clock_frame, holdDuration);
 
     double fpsr_output;
     if (finalRandSwitch) {
         uint64_t seed = (uint64_t)held_integer_state * 100000ULL;
         fpsr_output = portable_rand_u64(seed);
     } else {
         fpsr_output = (double)held_integer_state; 
     }
     return fpsr_output;
 }
 
 //-----------------------------------------------------------------------------/
 // FPS-R: Quantized Sine (QS)                                                  /
 //-----------------------------------------------------------------------------/
 // This special 'base' version of QS is for the wrapper. It returns the full
 // struct needed for rich output, and uses the Sine-LUT for determinism.
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
  * return 
  * FPSR_Output struct containing the random value and other details.
  * Output fields depend on the LOD level.
  * Refer to the FPSR_Output structure for details on the return values.
  */
 FPSR_Output fpsr_qs_base(
     int64_t frame, double baseWaveFreq, double stream2FreqMult,
     const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
     int64_t streamSwitchDur, int64_t stream1QuantDur, int64_t stream2QuantDur,
     int finalRandSwitch, int sine_lod_level)
 {
     FPSR_Output output = {0};
 
     if (streamSwitchDur < 1) { streamSwitchDur = 1; }
     if (stream1QuantDur < 1) { stream1QuantDur = 1; }
     if (stream2QuantDur < 1) { stream2QuantDur = 1; }
 
     int64_t quant_min = (int64_t)quantLevelsMinMax[0];
     int64_t quant_max = (int64_t)quantLevelsMinMax[1];
     int64_t quant_range = quant_max - quant_min + 1;
 
     int64_t s1_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[0] + frame, stream1QuantDur);
     double s1_rand_for_quant = portable_rand_u64((uint64_t)s1_quant_seed_aligned);
     int64_t s1_quant_level = quant_min + (int64_t)floor(s1_rand_for_quant * (double)quant_range);
 
     int64_t s2_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[1] + frame, stream2QuantDur);
     double s2_rand_for_quant = portable_rand_u64((uint64_t)s2_quant_seed_aligned);
     int64_t s2_quant_level = quant_min + (int64_t)floor(s2_rand_for_quant * (double)quant_range);
 
     if (s1_quant_level < 1) { s1_quant_level = 1; }
     if (s2_quant_level < 1) { s2_quant_level = 1; }
     if (stream2FreqMult < 0) { stream2FreqMult = 3.7; }
     
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
     output.randStreams[0] = floor((stream1_raw_sine * 0.5 + 0.5) * (double)s1_quant_level) / (double)s1_quant_level;
     output.randStreams[1] = floor((stream2_raw_sine * 0.5 + 0.5) * (double)s2_quant_level) / (double)s2_quant_level;
     
     int64_t r = i64_floor_mod(frame, streamSwitchDur);
     output.selected_stream_idx = (2 * r < streamSwitchDur) ? 0 : 1;
     double active_stream_val = (output.selected_stream_idx == 0) ? output.randStreams[0] : output.randStreams[1];
 
     if (finalRandSwitch == 1) {
         int64_t hashed_int = (int64_t)floor(active_stream_val * 100000.0);
         output.randVal = (float)portable_rand_u64((uint64_t)hashed_int);
     } else {
         output.randVal = (float)(0.5 * active_stream_val + 0.5);
     }
     return output;
 }
 
 //-----------------------------------------------------------------------------/
 // FPS-R: Bitstream Distortion (BD)                                            /
 //-----------------------------------------------------------------------------/
 // This is the pure 'base' version of BD for the wrapper. It returns just the float value.
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
 
     // --- Safer Memory Allocation ---
     // Calculate total memory needed for all buffers to make one contiguous allocation.
     // This is safer than multiple alloca() calls and simplifies cleanup.
     size_t chunk_data_sz = num_chunks * sizeof(uint64_t);
     // Check for overflow before calculating total size, though unlikely with realistic params.
     if (streams_number > 0 && (SIZE_MAX / streams_number < chunk_data_sz)) return 0.0; // Overflow
     size_t total_chunk_data_sz = streams_number * chunk_data_sz;
 
     // Allocate a single contiguous buffer on the heap for pointer arrays and chunk data to avoid alloca().
     size_t ptr_arrays_sz = (size_t)streams_number * sizeof(uint64_t*) * 2; // raw_streams pointers + transformed_streams pointers
     size_t total_alloc_size = ptr_arrays_sz + (2 * total_chunk_data_sz) + chunk_data_sz; // pointer arrays + raw_streams + transformed_streams + final_chunks
 
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
 /* Hierarchical Coherence Helper                                              */
 /******************************************************************************/
 
 /**
  * @brief Generates a stable, hierarchical seed for a "gap frame".
  * @details This function traverses a binary subdivision of the gap between
  * master_frame and master_frame + 1, based on the sub_frame_fraction.
  * It generates a unique, stable seed for any fractional position, ensuring
  * that the midpoint of a 2x zoom is identical to the midpoint of an 8x zoom.
  *
  * @param master_frame The integer "anchor" frame to the left of the gap.
  * @param sub_frame_fraction The fractional component [0.0, 1.0) of the scaled time.
  * @return A unique and stable uint64_t seed for this specific hierarchical position.
  */
 static inline uint64_t _get_hierarchical_seed(int64_t master_frame, double sub_frame_fraction) {
     // Start with the master_frame as the root seed for this gap.
     uint64_t seed = (uint64_t)master_frame;
     
     double current_pos = sub_frame_fraction;
     double boundary = 0.5;
     double step = 0.25;
 
     // Iterate down the binary tree. 53 levels is the max precision for a 64-bit double.
     for (int level = 1; level < 53; ++level) {
         // Hash the current seed with the level to create a unique seed for this tier.
         seed = splitmix64(seed + (uint64_t)level);
 
         // Check if we've hit the exact position.
         // Use a small epsilon for floating-point comparison.
         if (fabs(current_pos - boundary) < 1e-9) {
             break; 
         }
 
         if (current_pos > boundary) {
             // "Right" branch
             seed = splitmix64(seed + 1ULL); // Add 1 for the right branch
             boundary += step;
         } else {
             // "Left" branch
             seed = splitmix64(seed); // Add 0 (or nothing) for the left branch
             boundary -= step;
         }
         
         // If the step is too small, we've reached the limit of precision.
         if (step < 1e-9) {
             break;
         }
         step *= 0.5;
     }
     
     return seed;
 }
 
 /******************************************************************************/
 /* High-Level Wrapper Functions with Hierarchical Time                        */
 /******************************************************************************/
 
 /**
  * @brief Helper to get the scaled frame position, used for hold_progress.
  * @details This is a tiny helper to avoid duplicating the frame_multiplier logic.
  * We wrap the call to _get_details in this to get the scaled_frame_pos.
  */
 static inline FPSR_Output _get_details_with_scaled_pos(
     int64_t frame, double frame_multiplier,
     int (*get_details_func)(), // This is a placeholder, C limitations...
     // ... C doesn't support easy generic function pointers like this.
     // We will duplicate the logic instead.
     double* p_scaled_frame_pos_out) 
 {
     // This helper is not feasible without complex void* pointers.
     // The logic will be duplicated in each _get_details function instead.
     FPSR_Output out = {0};
     return out;
 }
 
 
 /**
  * ---- SM: Stacked Modulo Wrapper with Details ----
  * @brief Wrapper for fpsr_sm that returns a detailed FPSR_Output struct.
  * @param frame (int64_t) The current frame or time input.
  * @param frame_multiplier (double) The time scaling factor. >1.0 is slow-mo, <1.0 is fast-mo.
  * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
  * @param minHold (int) The minimum duration (in frames) for a value to hold.
  * @param maxHold (int) The maximum duration (in frames) for a value to hold.
  * @param reseedInterval (int) The fixed interval at which a new hold duration is calculated.
  * @param seedInner (int) An offset for the random duration calculation.
  * @param seedOuter (int) An offset for the final value calculation.
  * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
  * @param lod (int) The level of detail to calculate.
  * @param max_search_frames (int) A safety limit for the backward/forward search.
  * @return FPSR_Output struct with metadata populated based on the LOD.
  */
 FPSR_Output fpsr_sm_get_details(
     int64_t frame, double frame_multiplier,
     double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
     int minHold, int maxHold,
     int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch,
     int lod, int max_search_frames)
 {
     FPSR_Output out = {0};
     
     // --- HIERARCHICAL COHERENCE LOGIC (LOD 0) ---
     // This is the new "frame_zoom" logic.
     
     // 1. Calculate the 'true' floating-point position on the content timeline.
     double scaled_frame_position = (double)frame / (frame_multiplier == 0.0 ? 1.0 : frame_multiplier);
     if (p_scaled_frame_pos_out) {
         *p_scaled_frame_pos_out = scaled_frame_position;
     }
 
     // 2. Get the integer part ('master frame') and fractional part ('gap').
     int64_t master_frame = (int64_t)floor(scaled_frame_position);
     double sub_frame_fraction = scaled_frame_position - (double)master_frame;
 
     // 3. Check if we are on a 'Master Frame' or in a 'Gap Frame'.
     if (sub_frame_fraction == 0.0) {
         // --- MASTER FRAME ---
         // We landed perfectly on an integer.
         // Just call the base algorithm with that integer.
         out.randVal = (float)fpsr_sm_base(master_frame, (int64_t)minHold, (int64_t)maxHold, (int64_t)reseedInterval, (int64_t)seedInner, (int64_t)seedOuter, finalRandSwitch);
     } else {
         // --- GAP FRAME ---
         // We are in a 'fractal zoom' gap.
         
         // 1. Find the hierarchical seed for this exact fractional position.
         int64_t hierarchical_seed = _get_hierarchical_seed(master_frame, sub_frame_fraction);
         
         // 2. Make the nested call.
         // We use '0' as the frame (as it's a "local" event) and pass the
         // unique hierarchical_seed as the 'seedInner' to generate the value.
         out.randVal = (float)fpsr_sm_base(0, (int64_t)minHold, (int64_t)maxHold, (int64_t)reseedInterval, hierarchical_seed, (int64_t)seedOuter, finalRandSwitch);
     }
     
     if (lod < 1) return out;
 
     // LOD 1: Compare with previous frame to check for change.
     // This is now a recursive call to THIS function, ensuring the
     // previous value is also calculated using the hierarchical logic.
     FPSR_Output prev_out = fpsr_sm_get_details(frame - 1, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0);
     out.randVal_previous = prev_out.randVal; 
     out.has_changed = (out.randVal != prev_out.randVal);
 
     if (lod < 2) return out;
 
     // --- LOD 2: MODIFIED Robust Two-Phase Search ---
     // The search logic MUST now call this _get_details function (with lod=0)
     // to get values, not the _base function.
     int64_t low_int, high_int, mid_int, result_int; 
     float next_val_candidate = 0.0f;
     int64_t step_int = 1;
 
     // --- Backwards Search for last_changed_frame ---
     if (out.has_changed) {
         out.last_changed_frame = (int)frame;
     } else {
         int64_t bound_low_int = frame;
         step_int = 1;
         // Search in "app time"
         while (frame - step_int > frame - max_search_frames) { 
             // Call this function recursively (LOD 0) to get the hierarchical value
             float val_at_probe = fpsr_sm_get_details(frame - step_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
             if (val_at_probe != out.randVal) {
                 bound_low_int = frame - step_int;
                 break;
             }
             bound_low_int = frame - step_int;
             step_int *= 2; 
         }
         
         low_int = bound_low_int;
         high_int = frame;
         result_int = frame - max_search_frames + 1;
         while(low_int <= high_int) {
             mid_int = low_int + (high_int - low_int) / 2; 
             float mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
             if (mid_val == out.randVal) {
                 float prev_mid_val = fpsr_sm_get_details(mid_int - 1, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
                 if (prev_mid_val != out.randVal) {
                     result_int = mid_int; break;
                 }
                 high_int = mid_int - 1; 
             } else {
                 low_int = mid_int + 1; 
             }
         }
         out.last_changed_frame = (int)result_int;
     }
 
     // --- Forwards Search for next_changed_frame ---
     int64_t bound_high_int = frame;
     step_int = 1;
     while (frame + step_int < frame + max_search_frames) { 
         float val_at_probe = fpsr_sm_get_details(frame + step_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
         if (val_at_probe != out.randVal) {
             bound_high_int = frame + step_int;
             next_val_candidate = val_at_probe;
             break;
         }
         bound_high_int = frame + step_int;
         step_int *= 2; 
     }
 
     low_int = frame;
     high_int = bound_high_int;
     result_int = frame + max_search_frames;
     while(low_int <= high_int) {
         mid_int = low_int + (high_int - low_int) / 2; 
         float mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
         if (mid_val != out.randVal) {
             result_int = mid_int;
             next_val_candidate = mid_val;
             high_int = mid_int - 1; 
         } else {
             low_int = mid_int + 1; 
         }
     }
     out.next_changed_frame = (int)result_int;
     out.randVal_next_changed_frame = next_val_candidate; 
     
     // --- MODIFIED hold_progress Calculation ---
     // We need the *true scaled time* at the change boundaries
     // to calculate progress correctly in the "content time" domain.
     double scaled_last_changed = 0.0;
     double scaled_next_changed = 0.0;
     
     // We re-call (LOD 0) at the boundaries we found, just to get their scaled time.
     fpsr_sm_get_details(out.last_changed_frame, frame_multiplier, &scaled_last_changed, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0);
     fpsr_sm_get_details(out.next_changed_frame, frame_multiplier, &scaled_next_changed, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0);
 
     double hold_duration = scaled_next_changed - scaled_last_changed;
     if (hold_duration > 0.0) { 
         // Use the scaled_frame_position we got from LOD 0
         out.hold_progress = (float)((scaled_frame_position - scaled_last_changed) / hold_duration); 
     } else {
         out.hold_progress = 0.0f;
     }
     
     return out;
 }
 
 /**
  * ---- TM: Toggle Modulo Wrapper with Details ----
  * @brief Wrapper for fpsr_tm that returns a detailed FPSR_Output struct.
  * @param frame (int64_t) The current frame or time input.
  * @param frame_multiplier (double) The time scaling factor. >1.0 is slow-mo, <1.0 is fast-mo.
  * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
  * @param periodA (int) The first hold duration (in frames).
  * @param periodB (int) The second hold duration (in frames).
  * @param periodSwitch (int) The fixed interval at which the hold duration is toggled.
  * @param seedInner (int) An offset for the toggle clock.
  * @param seedOuter (int) An offset for the main clock.
  * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
  * @param lod (int) The level of detail to calculate.
  * @param max_search_frames (int) A safety limit for the backward/forward search.
  * @return FPSR_Output struct with metadata populated based on the LOD.
  */
 FPSR_Output fpsr_tm_get_details(
     int64_t frame, double frame_multiplier,
     double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
     int periodA, int periodB,
     int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch,
     int lod, int max_search_frames)
 {
     FPSR_Output out = {0};
     
     // --- HIERARCHICAL COHERENCE LOGIC (LOD 0) ---
     double scaled_frame_position = (double)frame / (frame_multiplier == 0.0 ? 1.0 : frame_multiplier);
     if (p_scaled_frame_pos_out) {
         *p_scaled_frame_pos_out = scaled_frame_position;
     }
     int64_t master_frame = (int64_t)floor(scaled_frame_position);
     double sub_frame_fraction = scaled_frame_position - (double)master_frame;
 
     if (sub_frame_fraction == 0.0) {
         // --- MASTER FRAME ---
         out.randVal = (float)fpsr_tm_base(master_frame, (int64_t)periodA, (int64_t)periodB, (int64_t)periodSwitch, (int64_t)seedInner, (int64_t)seedOuter, finalRandSwitch);
     } else {
         // --- GAP FRAME ---
         int64_t hierarchical_seed = _get_hierarchical_seed(master_frame, sub_frame_fraction);
         // Use '0' as the frame, pass hierarchical_seed as 'seedInner'
         out.randVal = (float)fpsr_tm_base(0, (int64_t)periodA, (int64_t)periodB, (int64_t)periodSwitch, hierarchical_seed, (int64_t)seedOuter, finalRandSwitch);
     }
     
     if (lod < 1) return out;
 
     // LOD 1
     FPSR_Output prev_out = fpsr_tm_get_details(frame - 1, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0);
     out.randVal_previous = prev_out.randVal; 
     out.has_changed = (out.randVal != prev_out.randVal);
     
     if (lod < 2) return out;
 
     // --- LOD 2: MODIFIED Robust Search ---
     int64_t low_int, high_int, mid_int, result_int; 
     float next_val_candidate = 0.0f;
     int64_t step_int = 1;
 
     // --- Backwards Search for last_changed_frame ---
     if (out.has_changed) {
         out.last_changed_frame = (int)frame;
     } else {
         int64_t bound_low_int = frame;
         step_int = 1;
         while (frame - step_int > frame - max_search_frames) {
             float val_at_probe = fpsr_tm_get_details(frame - step_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
             if (val_at_probe != out.randVal) {
                 bound_low_int = frame - step_int;
                 break;
             }
             bound_low_int = frame - step_int;
             step_int *= 2;
         }
         low_int = bound_low_int;
         high_int = frame;
         result_int = frame - max_search_frames + 1;
         while(low_int <= high_int) {
             mid_int = low_int + (high_int - low_int) / 2;
             float mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
             if (mid_val == out.randVal) {
                 float prev_mid_val = fpsr_tm_get_details(mid_int - 1, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
                 if (prev_mid_val != out.randVal) {
                     result_int = mid_int; break;
                 }
                 high_int = mid_int - 1; 
             } else {
                 low_int = mid_int + 1; 
             }
         }
         out.last_changed_frame = (int)result_int;
     }
 
     // --- Forwards search ---
     int64_t bound_high_int = frame;
     step_int = 1;
     while (frame + step_int < frame + max_search_frames) {
         float val_at_probe = fpsr_tm_get_details(frame + step_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
         if (val_at_probe != out.randVal) {
             bound_high_int = frame + step_int;
             next_val_candidate = val_at_probe;
             break;
         }
         bound_high_int = frame + step_int;
         step_int *= 2;
     }
     low_int = frame;
     high_int = bound_high_int;
     result_int = frame + max_search_frames;
     while(low_int <= high_int) {
         mid_int = low_int + (high_int - low_int) / 2;
         float mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0).randVal;
         if (mid_val != out.randVal) {
             result_int = mid_int;
             next_val_candidate = mid_val;
             high_int = mid_int - 1;
         } else {
             low_int = mid_int + 1;
         }
     }
     out.next_changed_frame = (int)result_int;
     out.randVal_next_changed_frame = next_val_candidate;
     
     // --- MODIFIED hold_progress Calculation ---
     double scaled_last_changed = 0.0;
     double scaled_next_changed = 0.0;
     fpsr_tm_get_details(out.last_changed_frame, frame_multiplier, &scaled_last_changed, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0);
     fpsr_tm_get_details(out.next_changed_frame, frame_multiplier, &scaled_next_changed, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0);
 
     double hold_duration = scaled_next_changed - scaled_last_changed;
     if (hold_duration > 0.0) {
         out.hold_progress = (float)((scaled_frame_position - scaled_last_changed) / hold_duration);
     } else {
         out.hold_progress = 0.0f;
     }
 
     return out;
 }
 
 /**
  * ---- QS: Quantised Switching Wrapper with Details ----
  * @brief Wrapper for fpsr_qs that returns a detailed FPSR_Output struct.
  * @param frame (int64_t) The current frame or time input.
  * @param frame_multiplier (double) The time scaling factor. >1.0 is slow-mo, <1.0 is fast-mo.
  * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
  * @param baseWaveFreq (float) The base frequency for the modulation wave of stream 1.
  * @param stream2FreqMult (float) A multiplier for the second stream's frequency.
  * @param quantLevelsMinMax (const int[2]) An array for min and max quantisation levels.
  * @param streamsOffset (const int[2]) An array to offset the frame for each stream's sine wave.
  * @param quantOffsets (const int[2]) An array to offset random quantisation selection.
  * @param streamSwitchDur (int) The number of frames after which the streams switch.
  * @param stream1QuantDur (int) The duration for stream 1's random quantisation level hold.
  * @param stream2QuantDur (int) The duration for stream 2's random quantisation level hold.
  * @param finalRandSwitch (int) A flag that can turn off the final randomisation step.
  * @param sine_lod_level (int) Level of detail for sine wave generation (0: direct sin(), 1-4: LUTs).
  * @param lod (int) The level of detail to calculate.
  * @param max_search_frames (int) A safety limit for the backward/forward search.
  * @return FPSR_Output struct with metadata populated based on the LOD.
  */
 FPSR_Output fpsr_qs_get_details(
     int64_t frame, double frame_multiplier,
     double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
     float baseWaveFreq, float stream2FreqMult,
     const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
     int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch,
     int sine_lod_level,
     int lod, int max_search_frames)
 {
     FPSR_Output out = {0};
 
     // --- HIERARCHICAL COHERENCE LOGIC (LOD 0) ---
     double scaled_frame_position = (double)frame / (frame_multiplier == 0.0 ? 1.0 : frame_multiplier);
     if (p_scaled_frame_pos_out) {
         *p_scaled_frame_pos_out = scaled_frame_position;
     }
     int64_t master_frame = (int64_t)floor(scaled_frame_position);
     double sub_frame_fraction = scaled_frame_position - (double)master_frame;
 
     FPSR_Output base_qs_output;
     if (sub_frame_fraction == 0.0) {
         // --- MASTER FRAME ---
         base_qs_output = fpsr_qs_base(master_frame, (double)baseWaveFreq, (double)stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, sine_lod_level);
     } else {
         // --- GAP FRAME ---
         int64_t hierarchical_seed = _get_hierarchical_seed(master_frame, sub_frame_fraction);
         
         // For QS, we modify the 'quantOffsets' to inject the hierarchical seed.
         // This makes the quantization level ("the event") unique to this gap.
         int new_quantOffsets[2] = {
             quantOffsets[0] + (int)(hierarchical_seed & 0xFFFFFFFF), 
             quantOffsets[1] + (int)((hierarchical_seed >> 32) & 0xFFFFFFFF)
         };
         
         // We still use '0' as the frame, as the 'streamsOffset' would be wrong,
         // but the sine wave freq at frame 0 is fine as a "local" event.
         // The *real* uniqueness comes from the new quant offsets.
         base_qs_output = fpsr_qs_base(0, (double)baseWaveFreq, (double)stream2FreqMult, quantLevelsMinMax, streamsOffset, new_quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, sine_lod_level);
     }
     
     out.randVal = base_qs_output.randVal;
     out.randStreams[0] = base_qs_output.randStreams[0];
     out.randStreams[1] = base_qs_output.randStreams[1];
     out.selected_stream_idx = base_qs_output.selected_stream_idx;
 
     if (lod < 1) return out;
 
     // LOD 1
     FPSR_Output prev_out = fpsr_qs_get_details(frame - 1, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
     out.randVal_previous = prev_out.randVal;
     out.has_changed = (out.randVal != out.randVal_previous);
     
     if (lod < 2) return out;
 
     // --- LOD 2: MODIFIED Robust Search ---
     int64_t low_int, high_int, mid_int, result_int; 
     float next_val_candidate = 0.0f;
     int64_t step_int = 1;
 
     // --- Backwards Search for last_changed_frame ---
     if (out.has_changed) {
         out.last_changed_frame = (int)frame;
     } else {
         int64_t bound_low_int = frame;
         step_int = 1;
         while (frame - step_int > frame - max_search_frames) { 
             FPSR_Output probe_qs_output = fpsr_qs_get_details(frame - step_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
             if (probe_qs_output.randVal != out.randVal) {
                 bound_low_int = frame - step_int;
                 break;
             }
             bound_low_int = frame - step_int;
             step_int *= 2; 
         }
         low_int = bound_low_int;
         high_int = frame;
         result_int = frame - max_search_frames + 1;
         while(low_int <= high_int) {
             mid_int = low_int + (high_int - low_int) / 2; 
             FPSR_Output mid_qs_output = fpsr_qs_get_details(mid_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
             if (mid_qs_output.randVal == out.randVal) {
                 FPSR_Output mid_minus_step_qs_output = fpsr_qs_get_details(mid_int - 1, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
                 if (mid_minus_step_qs_output.randVal != out.randVal) {
                     result_int = mid_int; break;
                 }
                 high_int = mid_int - 1; 
             } else {
                 low_int = mid_int + 1; 
             }
         }
         out.last_changed_frame = (int)result_int;
     }
 
     // --- Forwards search ---
     int64_t bound_high_int = frame;
     step_int = 1;
     while (frame + step_int < frame + max_search_frames) { 
         FPSR_Output probe_qs_output = fpsr_qs_get_details(frame + step_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
         if (probe_qs_output.randVal != out.randVal) {
             bound_high_int = frame + step_int;
             next_val_candidate = probe_qs_output.randVal;
             break;
         }
         bound_high_int = frame + step_int;
         step_int *= 2; 
     }
     low_int = frame;
     high_int = bound_high_int;
     result_int = frame + max_search_frames;
     while(low_int <= high_int) {
         mid_int = low_int + (high_int - low_int) / 2; 
         FPSR_Output mid_qs_output = fpsr_qs_get_details(mid_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
         if (mid_qs_output.randVal != out.randVal) {
             result_int = mid_int;
             next_val_candidate = mid_qs_output.randVal;
             high_int = mid_int - 1; 
         } else {
             low_int = mid_int + 1; 
         }
     }
     out.next_changed_frame = (int)result_int;
     out.randVal_next_changed_frame = next_val_candidate; 
     
     // --- MODIFIED hold_progress Calculation ---
     double scaled_last_changed = 0.0;
     double scaled_next_changed = 0.0;
     fpsr_qs_get_details(out.last_changed_frame, frame_multiplier, &scaled_last_changed, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
     fpsr_qs_get_details(out.next_changed_frame, frame_multiplier, &scaled_next_changed, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, sine_lod_level, 0, 0);
 
     double hold_duration = scaled_next_changed - scaled_last_changed;
     if (hold_duration > 0.0) { 
         out.hold_progress = (float)((scaled_frame_position - scaled_last_changed) / hold_duration); 
     } else {
         out.hold_progress = 0.0f;
     }
 
     return out;
 }
 
 /**
  * ---- BD: Bitwise Decode Wrapper with Details ----
  * @brief Wrapper for fpsr_bd that returns a detailed FPSR_Output struct.
  * @param frame (int64_t) The current frame or time input.
  * @param frame_multiplier (double) The time scaling factor. >1.0 is slow-mo, <1.0 is fast-mo.
  * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
  * @param block_size (int) The size of the macro-rhythm in frames.
  * @param streams_number (int) The number of parallel bitstreams to generate.
  * @param streams_offset (int) The frame offset between each parallel stream's seed.
  * @param intra_op (const char*) The unary (intra-stream) operation ("none", "not", "lshift", etc.).
  * @param dynamic_shift_bits (int) For dynamic ops, the number of controller bits to read.
  * @param static_shift_amount (int) For static ops, the fixed number of bits to shift/rotate.
  * @param inter_op (const char*) The binary (inter-stream) operation ("xor", "or", "and").
  * @param value_seed_offset (int) An additional seed offset for the final value calculation.
  * @param lod (int) The level of detail to calculate.
  * @param max_search_frames (int) A safety limit for the backward/forward search.
  * @return FPSR_Output struct with metadata populated based on the LOD.
  */
 FPSR_Output fpsr_bd_get_details(
     int64_t frame, double frame_multiplier,
     double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
     int block_size,
     int streams_number,
     int streams_offset,
     const char* intra_op,
     int dynamic_shift_bits,
     int static_shift_amount,
     const char* inter_op,
     int value_seed_offset,
     int lod, int max_search_frames
 )
 {
     FPSR_Output out = {0};
     
     // --- HIERARCHICAL COHERENCE LOGIC (LOD 0) ---
     double scaled_frame_position = (double)frame / (frame_multiplier == 0.0 ? 1.0 : frame_multiplier);
     if (p_scaled_frame_pos_out) {
         *p_scaled_frame_pos_out = scaled_frame_position;
     }
     int64_t master_frame = (int64_t)floor(scaled_frame_position);
     double sub_frame_fraction = scaled_frame_position - (double)master_frame;
 
     if (sub_frame_fraction == 0.0) {
         // --- MASTER FRAME ---
         out.randVal = (float)fpsr_bd(
             master_frame, (int64_t)block_size, streams_number, (int64_t)streams_offset,
             intra_op, dynamic_shift_bits, static_shift_amount, inter_op, (int64_t)value_seed_offset
         );
     } else {
         // --- GAP FRAME ---
         int64_t hierarchical_seed = _get_hierarchical_seed(master_frame, sub_frame_fraction);
         
         // For BD, we replace the 'value_seed_offset' with the hierarchical seed.
         // We also pass '0' as the frame, since the 'master_frame' context
         // is now contained within the hierarchical seed.
         out.randVal = (float)fpsr_bd(
             0, (int64_t)block_size, streams_number, (int64_t)streams_offset,
             intra_op, dynamic_shift_bits, static_shift_amount, inter_op, hierarchical_seed
         );
     }
     
     if (lod < 1) return out;
 
     // LOD 1
     FPSR_Output prev_out = fpsr_bd_get_details(frame - 1, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0);
     out.randVal_previous = prev_out.randVal;
     out.has_changed = (out.randVal != out.randVal_previous);
 
     if (lod < 2) return out;
 
     // --- LOD 2: MODIFIED Robust Search ---
     int64_t low_int, high_int, mid_int, result_int; 
     float next_val_candidate = 0.0f;
     int64_t step_int = 1;
 
     // --- Backwards Search for last_changed_frame ---
     if (out.has_changed) {
         out.last_changed_frame = (int)frame;
     } else {
         int64_t bound_low_int = frame;
         step_int = 1;
         while (frame - step_int > frame - max_search_frames) {
             float val_at_probe = fpsr_bd_get_details(frame - step_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0).randVal;
             if (val_at_probe != out.randVal) {
                 bound_low_int = frame - step_int;
                 break;
             }
             bound_low_int = frame - step_int;
             step_int *= 2;
         }
         low_int = bound_low_int;
         high_int = frame;
         result_int = frame - max_search_frames + 1;
         while(low_int <= high_int) {
             mid_int = low_int + (high_int - low_int) / 2;
             float mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0).randVal;
             if (mid_val == out.randVal) {
                 float prev_mid_val = fpsr_bd_get_details(mid_int - 1, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0).randVal;
                 if (prev_mid_val != out.randVal) {
                     result_int = mid_int; break;
                 }
                 high_int = mid_int - 1; 
             } else {
                 low_int = mid_int + 1; 
             }
         }
         out.last_changed_frame = (int)result_int;
     }
 
     // --- Forwards search ---
     int64_t bound_high_int = frame;
     step_int = 1;
     while (frame + step_int < frame + max_search_frames) {
         float val_at_probe = fpsr_bd_get_details(frame + step_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0).randVal;
         if (val_at_probe != out.randVal) {
             bound_high_int = frame + step_int;
             next_val_candidate = val_at_probe;
             break;
         }
         bound_high_int = frame + step_int;
         step_int *= 2;
     }
     low_int = frame;
     high_int = bound_high_int;
     result_int = frame + max_search_frames;
     while(low_int <= high_int) {
         mid_int = low_int + (high_int - low_int) / 2;
         float mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0).randVal;
         if (mid_val != out.randVal) {
             result_int = mid_int;
             next_val_candidate = mid_val;
             high_int = mid_int - 1;
         } else {
             low_int = mid_int + 1;
         }
     }
     out.next_changed_frame = (int)result_int;
     out.randVal_next_changed_frame = next_val_candidate;
     
     // --- MODIFIED hold_progress Calculation ---
     double scaled_last_changed = 0.0;
     double scaled_next_changed = 0.0;
     fpsr_bd_get_details(out.last_changed_frame, frame_multiplier, &scaled_last_changed, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0);
     fpsr_bd_get_details(out.next_changed_frame, frame_multiplier, &scaled_next_changed, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0);
 
     double hold_duration = scaled_next_changed - scaled_last_changed;
     if (hold_duration > 0.0) {
         out.hold_progress = (float)((scaled_frame_position - scaled_last_changed) / hold_duration);
     } else {
         out.hold_progress = 0.0f;
     }
 
     return out;
 }
 
 
 int main() {
     // Example usage of the FPSR algorithms with detailed output
     
     // Report OS type
     if (os == 0) {
         printf("Windows OS.\n");
     } else if (os == 1) {
         printf("POSIX OS.\n");
     } else {
         printf("Unknown OS.\n");
     }
     
     // Algorithms: 0 - SM, 1 - TM, 2 - QS, 3 - BD
     int algo = 3; // Change this value to 0, 1, 2, or 3 to test different algorithms
     char algo_name[][4] = {"SM", "TM", "QS", "BD"}; // Names for the algorithms
     printf("Using algorithm FPS-R: %s\n", algo_name[algo]);
 
     int start_frames[] = {90, 100, 103, 100}; // Starting frames for each algorithm
     int num_frames = 30; // Run a loop of 30 frames to demonstrate changes
     int lod = 2; // Level of detail (0, 1, or 2) for rich output
 
     for (int loop_frame = 0; loop_frame < num_frames; loop_frame++) {
         int64_t frame = (int64_t)loop_frame + (int64_t)start_frames[algo]; // Use int64_t for frame
         double frame_multiplier = 1.0; // Use double for multiplier
         FPSR_Output output = {0};
         
         if (algo == 0) {
             // Parameters for FPS-R:SM
             int minHoldFrames = 7;      // Minimum hold duration
             int maxHoldFrames = 9;      // Maximum hold duration
             int reseedFrames = 6;       // Reseed interval
             int offsetInner = -41;      // Inner seed offset
             int offsetOuter = 23;       // Outer seed offset
             int finalRandSwitch = 1;    // Final randomisation switch
             int max_search_frames = 50; // Safety limit for search
 
             // Call fpsr_sm_get_details
             output = fpsr_sm_get_details(frame, frame_multiplier, NULL, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch, lod, max_search_frames);
         } else if (algo == 1) {
             // Parameters for FPS-R:TM
             int periodA = 8;            // First hold duration
             int periodB = 5;            // Second hold duration
             int periodSwitch = 6;       // Period switch interval
             int offsetInner = 15;       // Inner seed offset
             int offsetOuter = 0;        // Outer seed offset
             int finalRandSwitch = 1;    // Final randomisation switch
             int max_search_frames = 50; // Safety limit for search
 
             // Call fpsr_tm_get_details
             output = fpsr_tm_get_details(frame, frame_multiplier, NULL,
                 periodA, periodB, periodSwitch, offsetInner, offsetOuter, 
                 finalRandSwitch, lod, max_search_frames);
         } else if (algo == 2) {
             // Parameters for FPS-R:QS
             float baseWaveFreq = 0.012f;    // Base wave frequency for stream 1
             float stream2FreqMult = 3.1f;   // Frequency multiplier for stream 2
             int quantLevelsMinMax[2] = {4, 12}; // Min and max quantisation levels
             int streamsOffset[2] = {0, 76}; // Frame offsets for each stream
             int quantOffsets[2] = {10, 81}; // Quantisation level offsets
             int streamSwitchDur = 8;        // Duration after which streams switch
             int stream1QuantDur = 10;       // Duration for stream 1 quantisation hold
             int stream2QuantDur = 13;       // Duration for stream 2 quantisation hold
             int finalRandSwitch = 1;        // Final randomisation switch
             int sine_lod_level = 4;         // Sine wave LOD level (0-4)
             int max_search_frames = 50;     // Safety limit for search
 
             // Call fpsr_qs_get_details
             output = fpsr_qs_get_details(frame, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, 
                 quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, 
                 stream1QuantDur, stream2QuantDur, finalRandSwitch, 
                 sine_lod_level, lod, max_search_frames);
         } else if (algo == 3) {
             // Parameters for FPS-R:BD
             int p_block_size = 64;           // Size of the macro-rhythm block
             int p_streams_number = 2;        // Number of parallel bitstreams
             int p_streams_offset = 10;       // Frame offset between each stream's seed
             // Intra-stream operation operates on each stream individually
             //      Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
             //      Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
             const char* p_intra_op = "rotl_dynamic"; // Intra-stream operation on each stream
             int p_dynamic_shift_bits = 6;    // Dynamic shift bits for intra-op
             int p_static_shift_amount = 1;   // Static shift amount for intra-op
             const char* p_inter_op = "xor";  // Inter-stream operation to combine transformed streams
                                             //     Options: "xor", "or", "and".
             int p_value_seed_offset = 78901; // Additional seed offset for final value
             int max_search_frames = 100; // BD blocks can be large
 
             output = fpsr_bd_get_details(
                 frame, frame_multiplier, NULL, p_block_size, p_streams_number, p_streams_offset,
                 p_intra_op, p_dynamic_shift_bits, p_static_shift_amount,
                 p_inter_op, p_value_seed_offset, lod, max_search_frames
             );
         }
 
 
         // Print the output for the current frame
         // Use %lld for int64_t frame
         printf("Frame %lld: randVal %.6f, prevVal %.6f, changed %d, progress %.3f, last %d, next %d ",
             frame, output.randVal, output.randVal_previous, output.has_changed, output.hold_progress, output.last_changed_frame, output.next_changed_frame);
         if (algo == 2) {
             printf("| s_idx %d, s[0] %.3f, s[1] %.3f ", output.selected_stream_idx, output.randStreams[0], output.randStreams[1]);
         }
         if (output.has_changed) printf("(jumped)");
         printf("\n");
         
     }
 
     return 0;
 }
 