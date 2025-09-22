# SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

"""
file: test_parity.py
brief: Python parity test runner for FPS-R algorithms (SM, TM, QS).

This script imports the Python implementation and prints the 64-bit IEEE-754
bit patterns of outputs across a sweep of frames. A companion C test program
prints the same for the C implementation. Comparing the logs enables bit-for-bit
parity checks between Python and C.

Why print float bit patterns?
- Textual float formatting can hide tiny differences; printing the exact 64-bit
  representation (as hex) guarantees we catch any mismatch.

Determinism notes:
- We use the same helpers/PRNG as the C reference (SplitMix64 + top-53-bit map),
  integer seeds with explicit uint64 wrap-around via masking, and math.floor for
  float->int boundaries, mirroring C double semantics.
- Python int is arbitrary-precision, which is fine; we only mask when emulating
  uint64 behavior at PRNG boundaries.
"""

import struct
from fpsr_algorithms import (
    fpsr_sm,
    fpsr_tm,
    fpsr_qs,
)

# Helper to get the exact 64-bit pattern of a Python float (IEEE-754 double)
def f64_bits_hex(x: float) -> str:
    """Return the 64-bit IEEE-754 bit pattern of the float as 0xXXXXXXXXXXXXXXXX hex.
    Using big-endian to match human-readable order; comparison only relies on
    equality, not endianness.
    """
    b = struct.pack('>d', float(x))  # pack as big-endian double
    u = int.from_bytes(b, 'big')
    return f"0x{u:016X}"


def run_sweep():
    # Frame sweep range chosen to include negatives for modulo semantics testing
    start_frame = -10
    count = 40  # total frames

    # Use the same default parameters as the C reference demo for parity
    # SM parameters
    sm_minHold = 12
    sm_maxHold = 21
    sm_reseed = 7
    sm_seedInner = -41
    sm_seedOuter = 23
    sm_final = True

    # TM parameters
    tm_periodA = 6
    tm_periodB = 8
    tm_periodSwitch = 10
    tm_seedInner = 15
    tm_seedOuter = 0
    tm_final = True

    # QS parameters
    qs_baseWaveFreq = 0.012
    qs_stream2freqMult = 3.1
    qs_quantLevelsMinMax = [4, 12]
    qs_streamsOffset = [0, 76]
    qs_quantOffsets = [10, 81]
    qs_streamSwitchDur = 14
    qs_stream1QuantDur = 6
    qs_stream2QuantDur = 9
    qs_final = True

    print("# === Python FPS-R parity sweep (float64 bit patterns) ===")

    for i in range(count):
        frame = start_frame + i

        # SM
        sm_val = fpsr_sm(frame, sm_minHold, sm_maxHold, sm_reseed, sm_seedInner, sm_seedOuter, sm_final)
        print(f"SM frame={frame:4d} val={sm_val:.17g} bits={f64_bits_hex(sm_val)}")

        # TM
        tm_val = fpsr_tm(frame, tm_periodA, tm_periodB, tm_periodSwitch, tm_seedInner, tm_seedOuter, tm_final)
        print(f"TM frame={frame:4d} val={tm_val:.17g} bits={f64_bits_hex(tm_val)}")

        # QS
        qs_val = fpsr_qs(frame, qs_baseWaveFreq, qs_stream2freqMult, qs_quantLevelsMinMax,
                         qs_streamsOffset, qs_quantOffsets,
                         qs_streamSwitchDur, qs_stream1QuantDur, qs_stream2QuantDur, qs_final)
        print(f"QS frame={frame:4d} val={qs_val:.17g} bits={f64_bits_hex(qs_val)}")

    print("# === End sweep ===")


if __name__ == "__main__":
    run_sweep()
