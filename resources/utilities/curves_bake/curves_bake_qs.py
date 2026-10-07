# This script generates a sine lookup table in C and Python formats.
# It computes a table of double precision values representing the sine of angles from 0 to 2π,
# and formats the output into configurable column widths.

import math

# Configuration
NUM_POINTS = 1024
TABLE_NAME = "fpsr_sine_lut_1024"
COLS_PER_ROW = 5


def compute_sine_lookup_table(num_points: int, bipolar: bool = True) -> list[float]:
    """Computes a pure list of sine table samples.
    
    num_points: Number of points in the lookup table.
    bipolar: If True, values in range [-1.0, 1.0]. If False (unipolar), values in range [0.0, 1.0].
    
    Returns:
        list[float]: The generated sine values.
    """
    table = []
    for i in range(num_points):
        sine_val = math.sin(2.0 * math.pi * (i / num_points))
        val = sine_val if bipolar else 0.5 * (sine_val + 1.0)
        table.append(val)
    return table


def _format_table_rows(values: list[float], cols: int, indent: str = "    ") -> list[str]:
    """Helper to format a flat list of floats into grouped rows."""
    if cols < 1:
        cols = 1
    lines = []
    total = len(values)

    for i in range(0, total, cols):
        chunk = values[i : i + cols]
        row_tokens = []
        for idx, val in enumerate(chunk):
            global_idx = i + idx
            comma = "," if global_idx < total - 1 else ""
            row_tokens.append(f"{val:.16f}{comma}")
        lines.append(f"{indent}" + " ".join(row_tokens))

    return lines


def generate_c_sine_lookup_table(values: list[float], table_name: str, bipolar: bool = True, cols: int = 4) -> str:
    """Formats a list of floats as a static const C array."""
    num_points = len(values)
    range_desc = "[-1.0, 1.0] (bipolar)" if bipolar else "[0.0, 1.0] (unipolar)"

    lines = [
        f"// Auto-generated {num_points}-point sine lookup table",
        f"// Maps normalized phase [0.0, 1.0) to {range_desc}",
        f"static const double {table_name}[{num_points}] = {{",
    ]
    lines.extend(_format_table_rows(values, cols=cols, indent="    "))
    lines.append("};")
    return "\n".join(lines)


def generate_python_sine_lookup_table(values: list[float], table_name: str | None = None, bipolar: bool = True, cols: int = 4) -> str:
    """Formats a list of floats as a Python list."""
    num_points = len(values)
    range_desc = "[-1.0, 1.0] (bipolar)" if bipolar else "[0.0, 1.0] (unipolar)"

    lines = [
        f"# Auto-generated {num_points}-point sine lookup table",
        f"# Maps normalized phase [0.0, 1.0) to {range_desc}",
    ]
    prefix = f"{table_name} = [" if table_name else "["
    lines.append(prefix)
    lines.extend(_format_table_rows(values, cols=cols, indent="    "))
    lines.append("]")
    return "\n".join(lines)


if __name__ == "__main__":
    # 1. Compute table values
    unipolar_table = compute_sine_lookup_table(NUM_POINTS, bipolar=False)

    # 2. Format as C code
    generated_c_code = generate_c_sine_lookup_table(
        unipolar_table, table_name=TABLE_NAME, bipolar=False, cols=COLS_PER_ROW
    )
    print("// C Sine Lookup Table:")
    print(generated_c_code)

    # 3. Format as Python code
    generated_python_table = generate_python_sine_lookup_table(
        unipolar_table, table_name=TABLE_NAME, bipolar=False, cols=COLS_PER_ROW
    )
    print("\n# Python Sine Lookup Table:")
    print(generated_python_table)