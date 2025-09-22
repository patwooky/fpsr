// SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/fpsr

/*
file: test_parity.c
brief: C parity test runner for FPS-R algorithms (SM, TM, QS).

This program includes the canonical C reference implementation and prints the
64-bit IEEE-754 bit patterns of outputs across a sweep of frames. A companion
Python script prints the same for the Python implementation. Comparing the logs
enables bit-for-bit parity checks between Python and C.

Implementation notes for determinism:
- We directly include the reference C file but remap its `main` to avoid symbol
  collision. This guarantees the test uses the exact same logic and helpers
  (floor-mod, align-down, SplitMix64, and double math) as the reference.
- We print both textual values and the exact 64-bit bit pattern of each double
  to catch any subtle discrepancies that string formatting might hide.
*/

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

// Rename the reference file's main() so we can provide our own test main().
#define main fpsr_reference_main
#include "fpsr_algorithms_reference.c"
#undef main

// Return the raw 64-bit IEEE-754 bit pattern of a double as an integer.
// Using a union is a simple and portable way to access the bit representation.
static inline uint64_t f64_bits(double x) {
    union { double d; uint64_t u; } v;
    v.d = x;
    return v.u;
}

int main(void) {
    // Frame sweep range chosen to include negatives for modulo semantics testing
    int start_frame = -10;
    int count = 40; // total frames

    // Use the same default parameters as the Python parity test for 1:1 comparison
    // SM parameters
    int64_t sm_minHold = 12;
    int64_t sm_maxHold = 21;
    int64_t sm_reseed = 7;
    int64_t sm_seedInner = -41;
    int64_t sm_seedOuter = 23;
    int sm_final = 1;

    // TM parameters
    int64_t tm_periodA = 6;
    int64_t tm_periodB = 8;
    int64_t tm_periodSwitch = 10;
    int64_t tm_seedInner = 15;
    int64_t tm_seedOuter = 0;
    int tm_final = 1;

    // QS parameters
    double qs_baseWaveFreq = 0.012;
    double qs_stream2freqMult = 3.1;
    int qs_quantLevelsMinMax[2] = {4, 12};
    int qs_streamsOffset[2] = {0, 76};
    int qs_quantOffsets[2] = {10, 81};
    int64_t qs_streamSwitchDur = 14;
    int64_t qs_stream1QuantDur = 6;
    int64_t qs_stream2QuantDur = 9;
    int qs_final = 1;

    printf("# === C FPS-R parity sweep (float64 bit patterns) ===\n");

    for (int i = 0; i < count; ++i) {
        int64_t frame = (int64_t)(start_frame + i);

        // SM
        double sm_val = fpsr_sm(frame, sm_minHold, sm_maxHold, sm_reseed, sm_seedInner, sm_seedOuter, sm_final);
        printf("SM frame=%4lld val=%.17g bits=0x%016" PRIX64 "\n", (long long)frame, sm_val, f64_bits(sm_val));

        // TM
        double tm_val = fpsr_tm(frame, tm_periodA, tm_periodB, tm_periodSwitch, tm_seedInner, tm_seedOuter, tm_final);
        printf("TM frame=%4lld val=%.17g bits=0x%016" PRIX64 "\n", (long long)frame, tm_val, f64_bits(tm_val));

        // QS
        double qs_val = fpsr_qs(frame, qs_baseWaveFreq, qs_stream2freqMult,
                                qs_quantLevelsMinMax, qs_streamsOffset, qs_quantOffsets,
                                qs_streamSwitchDur, qs_stream1QuantDur, qs_stream2QuantDur, qs_final);
        printf("QS frame=%4lld val=%.17g bits=0x%016" PRIX64 "\n", (long long)frame, qs_val, f64_bits(qs_val));
    }

    printf("# === End sweep ===\n");
    return 0;
}
