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

# Bit-width used for chunked bit operations (avoids magic 63/64).
# Naming: "chunk_bits" emphasizes this is the width of each chunk/word of the
# bitstream used for chunking, rotations, and bit indexing. It must remain 64
# for deterministic compatibility with SplitMix64 and the 64-bit masking below.
chunk_bits = 64

# --- Deterministic Helpers (Copied from fpsr_algorithms.py for consistency) ---
# These functions are essential for ensuring the output is identical to a C implementation.

_UINT64_MASK = (1 << chunk_bits) - 1

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

# --- Bitwise Rotation Helpers ---

def _circular_left_shift(value: int, shift: int) -> int:
    """Performs a chunk_bits-wide circular left shift (rotate left)."""
    # The rotation distance wraps at chunk_bits to keep it within the word.
    shift %= chunk_bits
    return _to_uint64((value << shift) | (value >> (chunk_bits - shift)))

def _circular_right_shift(value: int, shift: int) -> int:
    """Performs a chunk_bits-wide circular right shift (rotate right)."""
    # The rotation distance wraps at chunk_bits to keep it within the word.
    shift %= chunk_bits
    return _to_uint64((value >> shift) | (value << (chunk_bits - shift)))


# --- FPS-R: Bitwise Decode (BD) Implementation ---

def fpsr_bd(
    frame: int,
    block_size: int,
    streams_number: int = 1,
    streams_offset: int = 0,
    intra_op: str = "none",
    shift_amount: int = 1,
    inter_op: str = "xor",
    value_seed_offset: int = 0
):
    """
    Generates a phrased random value by decoding a deterministically generated bitstream.

    This algorithm is stateless. For any given frame, it calculates its state by:
    1. Finding the start of its macro-block (`outer_anchor`).
    2. Generating one or more raw bitstreams for the block.
    3. Applying transformations (intra-stream op) to each stream, possibly in pairs for dynamic ops.
    4. Combining the transformed streams (inter-stream op).
    5. Decoding the final bitstream to produce phrased holds and jumps.

    Args:
        frame (int): The current frame or time input.
        block_size (int): The size of the macro-rhythm in frames. Must be > 0.
        streams_number (int): The number of parallel bitstreams to generate.
        streams_offset (int): The frame offset between each parallel stream's seed.
        intra_op (str): The unary (intra-stream) operation.
                        Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
                        Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
        shift_amount (int): For static ops, the fixed number of bits to shift/rotate.
                            For dynamic ops, the number of bits to read from the controller
                            stream to determine the shift/rotate amount (1-6 when chunk_bits=64).
        inter_op (str): The binary (inter-stream) operation to combine multiple
                        transformed streams. Options: "xor", "or", "and".
        value_seed_offset (int): An additional seed offset for the final value calculation.
    Returns:
        float: A deterministic, phrased pseudo-random float between 0.0 and 1.0.
    """
    if block_size <= 0:
        block_size = 1

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the raw bitstream(s) for the entire block ---
    # Use ceiling division to count how many chunk_bits-wide words are needed to cover block_size.
    # Previously 63/64 were magic; here we generalize to (block_size + chunk_bits - 1) // chunk_bits.
    num_chunks = (block_size + (chunk_bits - 1)) // chunk_bits
    raw_streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
        chunks = [_splitmix64(_to_uint64(stream_seed + j)) for j in range(num_chunks)]
        raw_streams.append(chunks)

    # --- Step 3: Apply Intra-Stream Transformations ---
    transformed_streams = []
    unary_op = intra_op.lower()
    
    dynamic_ops = ["lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic"]

    if unary_op in dynamic_ops:
        # Process streams in pairs (data, controller)
        for i in range(0, streams_number // 2):
            data_stream = raw_streams[i * 2]
            controller_stream = raw_streams[i * 2 + 1]
            
            # Determine how many controller bits to read so shifts/rotations cover [0, chunk_bits-1].
            # With chunk_bits=64 this is 6 bits (0..63). Clamp to the user's shift_amount.
            max_bits_for_shift = max(1, math.ceil(math.log2(chunk_bits)) if chunk_bits > 1 else 1)
            bit_mask_size = max(1, min(max_bits_for_shift, shift_amount))
            bit_mask = (1 << bit_mask_size) - 1
            
            transformed_chunks = []
            for j in range(num_chunks):
                data_chunk = data_stream[j]
                controller_chunk = controller_stream[j]
                # Bound the shift to the word width so operations remain well-defined.
                dynamic_shift = (controller_chunk & bit_mask) % chunk_bits
                
                if unary_op == "lshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk << dynamic_shift))
                elif unary_op == "rshift_dynamic":
                    transformed_chunks.append(_to_uint64(data_chunk >> dynamic_shift))
                elif unary_op == "rotl_dynamic":
                    transformed_chunks.append(_circular_left_shift(data_chunk, dynamic_shift))
                elif unary_op == "rotr_dynamic":
                    transformed_chunks.append(_circular_right_shift(data_chunk, dynamic_shift))
            transformed_streams.append(transformed_chunks)
        
        # If there's an odd number of streams, the last one passes through untransformed
        if streams_number % 2 != 0:
            transformed_streams.append(raw_streams[-1])

    else: # Apply static operations to all streams
        for stream_chunks in raw_streams:
            if unary_op == "not":
                transformed_chunks = [_to_uint64(~chunk) for chunk in stream_chunks]
            elif unary_op == "lshift":
                transformed_chunks = [_to_uint64(chunk << shift_amount) for chunk in stream_chunks]
            elif unary_op == "rshift":
                transformed_chunks = [_to_uint64(chunk >> shift_amount) for chunk in stream_chunks]
            elif unary_op == "rotl":
                transformed_chunks = [_circular_left_shift(chunk, shift_amount) for chunk in stream_chunks]
            elif unary_op == "rotr":
                transformed_chunks = [_circular_right_shift(chunk, shift_amount) for chunk in stream_chunks]
            else: # "none" or any other value
                transformed_chunks = stream_chunks
            transformed_streams.append(transformed_chunks)
        
    # --- Step 4: Combine Streams with Inter-Stream (Binary) Operation ---
    if len(transformed_streams) > 1:
        op_map = { "xor": (lambda a, b: a ^ b), "or": (lambda a, b: a | b), "and": (lambda a, b: a & b) }
        chosen_op = op_map.get(inter_op.lower(), lambda a, b: a ^ b) # Default to xor
        
        combined_chunks = []
        for chunk_idx in range(num_chunks):
            chunks_to_combine = [stream[chunk_idx] for stream in transformed_streams]
            combined_chunk = functools.reduce(chosen_op, chunks_to_combine)
            combined_chunks.append(combined_chunk)
        final_chunks = combined_chunks
    elif transformed_streams:
        final_chunks = transformed_streams[0]
    else: # Should not happen if streams_number > 0
        final_chunks = [0] * num_chunks

    # --- Step 5: Decode the final bitstream ---
    def get_bit(n):
        if not (0 <= n < block_size): return 0
        # Index into the chunked bitstream using chunk_bits for clarity and portability.
        chunk_index, bit_index = n // chunk_bits, n % chunk_bits
        return (final_chunks[chunk_index] >> bit_index) & 1

    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0
    
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 6: Generate the final random value ---
    final_seed = _to_uint64(outer_anchor) + _to_uint64(last_flip_pos) + _to_uint64(value_seed_offset)
    
    return portable_rand_u64(final_seed)


# --- Example Usage ---
if __name__ == "__main__":
    print("--- FPS-R: Bitwise Decode (BD) Demo ---")

    # --- Parameters to play with ---
    start_frame = 100
    num_frames_to_run = 50
    
    # BD Parameters
    p_block_size = 64
    p_streams_number = 2 # Must be even for dynamic ops, or last one is ignored by intra_op
    p_streams_offset = 10
    p_intra_op = "rotl_dynamic" # Try "lshift_dynamic", "rotr_dynamic", etc.
    p_shift_amount = 6          # For dynamic ops and chunk_bits=64, this bitmask size reads 1-6 controller bits
    p_inter_op = "xor"          # "xor", "or", "and"
    p_value_seed_offset = 78901

    print(f"Parameters: block_size={p_block_size}, streams={p_streams_number}, intra_op='{p_intra_op}', inter_op='{p_inter_op}'\n")

    last_val = -1.0
    for i in range(num_frames_to_run):
        current_frame = start_frame + i
        
        rand_val = fpsr_bd(
            frame=current_frame,
            block_size=p_block_size,
            streams_number=p_streams_number,
            streams_offset=p_streams_offset,
            intra_op=p_intra_op,
            shift_amount=p_shift_amount,
            inter_op=p_inter_op,
            value_seed_offset=p_value_seed_offset
        )

        jumped = " (jump)" if rand_val != last_val else ""
        print(f"Frame {current_frame}: {rand_val:.6f}{jumped}")
        
        last_val = rand_val

