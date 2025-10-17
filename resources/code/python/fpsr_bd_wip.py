# SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

"""
file: fpsr_bitwise_decode.py
brief: Python implementation of the FPS-R: Bitwise Decode (BD) algorithm.
details:
    This file contains the standalone, stateless implementation for FPS-R: BD.
    It uses the same deterministic helpers as its sibling algorithms (SM, TM, QS)
    to ensure bit-for-bit reproducibility across platforms. The core mechanic
    involves generating a deterministic bitstream for a fixed block of time and
    interpreting bit-flips within that stream as triggers for new random values.
"""

import math
import functools

# --- Deterministic Helpers (Copied from fpsr_algorithms.py for consistency) ---
# These functions are essential for ensuring the output is identical to a C implementation.

_UINT64_MASK = (1 << 64) - 1

def _to_uint64(x: int) -> int:
    """Emulates C's uint64_t wraparound for cross-language determinism."""
    return x & _UINT64_MASK

def i64_floor_mod(a: int, m: int) -> int:
    """Floor-based modulo that matches Python's '%' for m>0."""
    return a % m

def i64_align_down(a: int, m: int) -> int:
    """Aligns a down to a multiple of m using floor-mod semantics."""
    return a - i64_floor_mod(a, m)

def _splitmix64(x: int) -> int:
    """A portable 64-bit pseudo-random number generator (PRNG)."""
    x = _to_uint64(x + 0x9E3779B97F4A7C15)
    x = _to_uint64((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9)
    x = _to_uint64((x ^ (x >> 27)) * 0x94D049BB133111EB)
    x = _to_uint64(x ^ (x >> 31))
    return x

def portable_rand_u64(seed: int) -> float:
    """Maps a 64-bit integer to a float in [0,1) deterministically."""
    r = _splitmix64(_to_uint64(seed))
    return float((r >> 11)) * (1.0 / 9007199254740992.0)

# --- FPS-R: Bitwise Decode (BD) Implementation ---

def fpsr_bd(
    frame: int,
    block_size: int,
    streams_number: int = 1,
    streams_offset: int = 0,
    streams_operation: str = "xor",
    shift_amount: int = 1,
    value_seed_offset: int = 0
):
    """
    Generates a phrased random value by decoding a deterministically generated bitstream.

    This algorithm is stateless. For any given frame, it calculates its state by:
    1. Finding the start of its macro-block (`outer_anchor`).
    2. Generating a bitstream ('micro-playbook') for the entire block based on that anchor.
    3. Tracing backwards within the playbook to find the last bit-flip.
    4. Using the position of that flip to seed the final random value.

    Args:
        frame (int): The current frame or time input.
        block_size (int): The size of the macro-rhythm in frames. A new bitstream is
                          generated for each block. Must be > 0.
        streams_number (int): The number of parallel bitstreams to generate and combine.
        streams_offset (int): The frame offset between each parallel stream's seed.
        streams_operation (str): The bitwise operation to apply.
                                 Combining ops: 'xor', 'or', 'and'.
                                 Single-stream ops: 'not', 'lshift', 'rshift'.
        shift_amount (int): The number of bits to shift for 'lshift' and 'rshift' operations.
        value_seed_offset (int): An additional seed offset for the final value calculation
                                 to de-correlate it from the bitstream generation.
    Returns:
        float: A deterministic, phrased pseudo-random float between 0.0 and 1.0.
    """
    if block_size <= 0:
        block_size = 1

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    # This determines the start of the current "micro-playbook".
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the bitstream(s) for the entire block ---
    # We generate 64 bits at a time using the PRNG.
    num_chunks = (block_size + 63) // 64  # Calculate how many 64-bit chunks are needed
    streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
        # Generate all 64-bit integer chunks for this stream
        chunks = [_splitmix64(_to_uint64(stream_seed + j)) for j in range(num_chunks)]
        streams.append(chunks)

    # --- Step 3: Combine or transform streams based on the chosen operation ---
    op = streams_operation.lower()
    
    # These operations apply to a single stream (the first one if multiple are generated)
    single_stream_ops = ["not", "lshift", "rshift"]

    if op in single_stream_ops:
        base_chunks = streams[0]
        if op == "not":
            # Apply bitwise NOT and mask to keep it a 64-bit unsigned integer
            final_chunks = [_to_uint64(~chunk) for chunk in base_chunks]
        elif op == "lshift":
            final_chunks = [_to_uint64(chunk << shift_amount) for chunk in base_chunks]
        elif op == "rshift":
            # Python's >> on positive integers is a logical right shift, matching C's uint64_t behavior.
            final_chunks = [_to_uint64(chunk >> shift_amount) for chunk in base_chunks]

    elif streams_number > 1:
        # These are combining operations for multiple streams
        op_map = {
            "xor": lambda a, b: a ^ b,
            "or": lambda a, b: a | b,
            "and": lambda a, b: a & b,
        }
        chosen_op = op_map.get(op, lambda a, b: a ^ b) # Default to xor
        
        # Combine the chunks for each position
        combined_chunks = []
        for chunk_idx in range(num_chunks):
            # Get the i-th chunk from each stream
            chunks_to_combine = [stream[chunk_idx] for stream in streams]
            # Apply the operation cumulatively
            combined_chunk = functools.reduce(chosen_op, chunks_to_combine)
            combined_chunks.append(combined_chunk)
        final_chunks = combined_chunks
    else:
        # If only one stream and no operation, just use the raw stream
        final_chunks = streams[0]

    # Helper to get a specific bit from our list of 64-bit chunks
    def get_bit(n):
        if not (0 <= n < block_size):
            return 0 # Out of bounds
        chunk_index = n // 64
        bit_index = n % 64
        return (final_chunks[chunk_index] >> bit_index) & 1

    # --- Step 4: Find the last bit-flip by walking backwards ---
    # This determines the start of the current "hold" period.
    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0 # Default to the start of the block
    
    # Scan backwards from the current position to find where the value last changed
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 5: Generate the final random value ---
    # The seed is a combination of the block's anchor, the flip position, and an offset.
    # This ensures the value is unique and deterministic for this specific hold.
    final_seed = _to_uint64(outer_anchor) + _to_uint64(last_flip_pos) + _to_uint64(value_seed_offset)
    
    return portable_rand_u64(final_seed)


# --- Example Usage ---
if __name__ == "__main__":
    print("--- FPS-R: Bitwise Decode (BD) Demo ---")

    # --- Parameters to play with ---
    start_frame = 100
    num_frames_to_run = 50
    
    # BD Parameters
    p_block_size = 64         # How often to get a new "playbook". Try small (16) and large (128) values.
    p_streams_number = 1          # Set to 1 to test a single-stream operator.
    p_streams_offset = 1          # Offset between stream seeds.
    p_streams_operation = "lshift"   # "xor", "or", "and", "not", "lshift", "rshift".
    p_shift_amount = 2            # Amount for shift operations.
    p_value_seed_offset = 78901   # A "salt" for the final value.

    print(f"Parameters: block_size={p_block_size}, streams={p_streams_number}, op='{p_streams_operation}', shift_amount={p_shift_amount}\n")

    last_val = -1.0
    for i in range(num_frames_to_run):
        current_frame = start_frame + i
        
        # Call the standalone BD function
        rand_val = fpsr_bd(
            frame=current_frame,
            block_size=p_block_size,
            streams_number=p_streams_number,
            streams_offset=p_streams_offset,
            streams_operation=p_streams_operation,
            shift_amount=p_shift_amount,
            value_seed_offset=p_value_seed_offset
        )

        jumped = " (jump)" if rand_val != last_val else ""
        print(f"Frame {current_frame}: {rand_val:.6f}{jumped}")
        
        last_val = rand_val

