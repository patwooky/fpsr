# SPDX-License-Identifier: MIT — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

# @file fpsr_expressions.py
# @brief Python expressions implementation of FPS-R algorithms: 
# Stacked Modulo (SM) and Toggled Modulo (TM).
# @details This file contains the one-line expression version of 2
# stateless, frame-persistent randomisation FPS-R algorithms.
# It uses a custom portable_rand() function to ensure  
# deterministic and consistent results across any platform.


# import math for sin() and floor()
import math

# A simple, portable pseudo-random number generator.
# Generates a deterministic float between 0.0 and 1.0 from an integer seed.
# seed: <int> An integer used to generate the random number.
# return: <float> A pseudo-random float between 0.0 and 1.0.
def portable_rand(seed):
    # A common technique for a simple hash-like random number.
    # The large prime numbers are used to create a chaotic, unpredictable result.
    val = float(seed) * 12.9898

    # --- FIX for float precision on GPUs and other platforms ---
    # On many platforms, math.sin() loses precision or returns 0 for large inputs.
    # This causes the random value to "saturate" and become constant over time.
    # By using the mathematical property sin(x) = sin(x mod 2π), we can wrap the
    # input to sin() into a high-precision range [0, 2π], ensuring the result
    # remains stable and correct indefinitely.
    TWO_PI = 6.28318530718
    val = math.fmod(val, TWO_PI)

    result = math.sin(val) * 43758.5453

    # The (result - math.floor(result)) emulates GLSL's fract().
    return result - math.floor(result)


# ------------------------------------------------------------------------------
# FPS-R: Stacked Modulo (SM)
# ------------------------------------------------------------------------------

# Generates a persistent random value that holds for a calculated duration.
# This function uses a two-step process. First, it determines a random
# "hold duration". Second, it generates a stable integer for that duration,
# which is then used as a seed to produce the final, held random value.
# NOTE: The expression calls the included portable_rand(),
#       but you are free to use random from the host application.
#
# frame: <int> The current frame or time input.
# minHoldFrames: <int> The minimum duration (in frames) for a value to hold.
# maxHoldFrames: <int> The maximum duration (in frames) for a value to hold.
# reseedFrames: <int> The fixed interval (number of frames) at which a new hold duration is calculated.
# seedInner: <int> An offset for the random duration calculation to create unique sequences.
# seedOuter: <int> An offset for the final value calculation to create unique sequences.
# return: <float> A float value between 0.0 and 1.0 that remains constant for the hold duration.
# Sample parameter values to call the FPS-R:SM expression with
frame = 100  # Replace with the current frame value
minHoldFrames = 16  # probable minimum held period
maxHoldFrames = 24  # maximum held period before cycling
reseedFrames = 9  # inner mod cycle timing
seedInner = -41  # offsets the inner frame
seedOuter = 23  # offsets the outer frame

# Call the FPS-R:SM expression
fpsr_sm_expression = portable_rand(
    (seedOuter + frame) - ((seedOuter + frame) % (
        minHoldFrames + int(
            portable_rand(
                (seedInner + frame) - ((seedInner + frame) % reseedFrames)
            ) * (maxHoldFrames - minHoldFrames)
        )
    ))
)

# ------------------------------------------------------------------------------
# FPS-R: Toggled Modulo (TM)
# ------------------------------------------------------------------------------

# Generates a persistent value that holds for a rhythmically toggled duration.
# This function uses a deterministic switch to toggle the hold duration
# between two fixed periods. This creates a predictable, rhythmic, or mechanical
# "move-and-hold" pattern, as opposed to the organic randomness of SM.
# NOTE: The expression calls the included portable_rand(), 
#       but you are free to use rand() from the host application.
#
# frame: <int> The current frame or time input.
# period_A: <int> The first hold duration (in frames).
# period_B: <int> The second hold duration (in frames).
# periodSwitch: <int> The fixed interval at which the hold duration is toggled.
# seedInner: <int> An offset for the toggle clock to de-sync it from the main clock.
# seedOuter: <int> An offset for the main clock to create unique sequences.
# return 
#   when finalRandSwitch is 0: 
#     An integer value representing the currently held frame state.
#   when finalRandSwitch is 1: 
#     A float value between 0.0 and 1.0 that holds for the toggled duration.

# Sample parameter values to call the FPS-R:TM expression with
frame = 100  # Replace with the current frame value
period_A = 10  # The first hold duration
period_B = 25  # The second hold duration
periodSwitch = 30  # The toggle happens every 30 frames
seedInner = 15  # offsets the inner (toggle) clock
seedOuter = 0  # offsets the outer (hold) clock

# Call the FPS-R:TM expression
fpsr_tm_expression = portable_rand(
    (seedOuter + frame) - ((seedOuter + frame) % (
        (period_A if (seedInner + frame) % periodSwitch < periodSwitch * 0.5 else period_B)
    ))
)

# ------------------------------------------------------------------------------
# FPS-R: Quantised Switching (QS)
# ------------------------------------------------------------------------------

# The nature of QS does not support a single line implementation.
# Instead, it requires a more complex function with multiple parameters.
