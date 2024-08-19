
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

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Compress a binary file using zlib with maximum compression.')
    parser.add_argument('input_file', type=str, help='The path to the input binary file.')
    parser.add_argument('output_file', type=str, help='The path to the output compressed file.')

    args = parser.parse_args()

    compress_binary(args.input_file, args.output_file)
