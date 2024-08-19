
import sys

def binary_to_header(binary_filename, header_filename, array_name):
    with open(binary_filename, 'rb') as bin_file:
        binary_data = bin_file.read()

    header_lines = [
        f"const unsigned char {array_name}[{len(binary_data)}] = {{",
    ]
    
    line = "    "
    for i, byte in enumerate(binary_data):
        if i % 16 == 0 and i != 0:
            header_lines.append(line)
            line = "    "
        line += f"0x{byte:02x}, "
    
    if line.strip():
        header_lines.append(line)
    
    header_lines.append("};")
    
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
