
import sys

def binary_to_header(binary_filename, header_filename, array_name):
    with open(binary_filename, 'rb') as bin_file:
        binary_data = bin_file.read()

    header_lines = [
        f"const unsigned char {array_name}[{len(binary_data)}] = {{",
    ]

    # Process the binary data 4 bytes at a time
    for i in range(0, len(binary_data), 4):
        # Extract up to 4 bytes from the current position
        chunk = binary_data[i:i+4]

        # Convert the chunk to a hex string
        line = ", ".join(f"0x{byte:02x}" for byte in chunk)

        # Add a comment with the hex offset
        offset_comment = f"/* 0x{i:08x} */"

        # Add the formatted line to the list of header lines
        header_lines.append(f"    {line},{' ' if len(chunk) < 4 else ''} {offset_comment}")

    header_lines.append("};")

    # Write the formatted header lines to the output file
    with open(header_filename, 'w') as header_file:
        header_file.write("\n".join(header_lines))
        header_file.write("\n")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python script.py <binary_file> <header_file> <array_name>")
        sys.exit(1)
    
    binary_file = sys.argv[1]
    header_file = sys.argv[2]
    array_name = sys.argv[3]
    
    binary_to_header(binary_file, header_file, array_name)
