# This script generates a sine lookup table in C format. It creates a static array of double precision values representing the sine of angles from 0 to 2π, divided into a specified number of points (1024 in this case). The output is formatted for inclusion in C code, with each value printed to 16 decimal places.

import math

# Configuration
NUM_POINTS = 1024
TABLE_NAME = "fpsr_sine_lut_1024"

def generate_c_sine_lookup_table(num_points, table_name):
    '''
    Generates a C-style sine lookup table with the specified number of points.
    Each entry corresponds to sin(2 * PI * i / num_points) for i in range(num_points).
    
    num_points: The number of points in the lookup table.
    table_name: The name of the C array to be generated.
    
    Returns: multiline string containing the C code for the lookup table.
    '''
    return_string = []
    return_string.append(f"// Auto-generated {num_points}-point sine lookup table")
    return_string.append(f"// Maps normalized phase [0.0, 1.0) to sin(0 to 2*PI)")
    return_string.append(f"static const double {table_name}[{num_points}] = {{")
    
    for i in range(NUM_POINTS):
        # 1. Get our normalized percentage (0.0 to 0.999...)
        normalized_phase = i / NUM_POINTS
        
        # 2. Multiply by 2*PI to get the math angle
        angle_in_radians = normalized_phase * 2.0 * math.pi
        
        # 3. Calculate the sine value
        sine_value = math.sin(angle_in_radians)
        
        # 4. Print it formatted for C (using 16 decimal places for double precision)
        comma = "," if i < NUM_POINTS - 1 else ""
        return_string.append(f"    {sine_value:.16f}{comma}")

    return_string.append("};")
    return "\n".join(return_string)

def generate_python_sine_lookup_table(num_points):
    '''
    Generates a Python-style sine lookup table with the specified number of points.
    Each entry corresponds to sin(2 * PI * i / num_points) for i in range(num_points).
    
    num_points: The number of points in the lookup table.
    
    Returns: list of sine values.
    '''
    return [float(f'{math.sin(2.0 * math.pi * (i / num_points)):.16f}') for i in range(num_points)]

generated_c_code = generate_c_sine_lookup_table(NUM_POINTS, TABLE_NAME)
print(generated_c_code)
# Output the generated C code to a file

generated_python_table = generate_python_sine_lookup_table(NUM_POINTS)

print("\nPython Sine Lookup Table:")
print(generated_python_table)
# Output the generated Python table to a file