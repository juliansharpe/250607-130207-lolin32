import re
import sys

def swap_bytes(hex_str):
    # Extract the 4 hex digits
    hex_digits = hex_str[2:]
    # Swap the two bytes
    swapped = hex_digits[2:] + hex_digits[:2]
    return f"0x{swapped.upper()}"

def process_file(input_path, output_path):
    with open(input_path, 'r') as infile:
        content = infile.read()

    # Match hex values like 0xFF10
    pattern = r'0x[0-9A-Fa-f]{4}'

    # Replace with swapped bytes
    modified = re.sub(pattern, lambda m: swap_bytes(m.group()), content)

    with open(output_path, 'w') as outfile:
        outfile.write(modified)

    print(f"Processed file saved as: {output_path}")

# Example usage
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python swap_bytes.py input.c output.c")
    else:
        process_file(sys.argv[1], sys.argv[2])