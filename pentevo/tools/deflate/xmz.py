import zlib
import os
import struct
import argparse

def compress_binary(input_file: str, output_file: str):
    # Read the binary data from the input file
    with open(input_file, 'rb') as f_in:
        data = f_in.read()

    # Get the length of the uncompressed data
    uncompressed_length = len(data)

    # Compress the binary data with maximum compression level (9)
    compressed_data = zlib.compress(data, level=9)

    # Pack the uncompressed length as a 4-byte little-endian integer
    length_prefix = struct.pack('<I', uncompressed_length)

    # Write the length prefix followed by the compressed data to the output file
    with open(output_file, 'wb') as f_out:
        f_out.write(length_prefix)
        f_out.write(compressed_data)

def compress_all_xm_files(directory: str):
    # Iterate over all files in the directory
    for filename in os.listdir(directory):
        # Check if the file has a .xm extension
        if filename.endswith('.xm'):
            input_file = os.path.join(directory, filename)
            output_file = os.path.join(directory, f"{os.path.splitext(filename)[0]}.xmz")
            
            print(f"Compressing {input_file} to {output_file}...")
            compress_binary(input_file, output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Compress all .xm files in a directory to .xmz using zlib.')
    parser.add_argument('directory', type=str, help='The path to the directory containing .xm files.')

    args = parser.parse_args()

    compress_all_xm_files(args.directory)
