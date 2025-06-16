import sys
import os
from pathlib import Path

MAX_SPLIT = 100

def main():
    argc = len(sys.argv)
    if argc < 3 or argc > (MAX_SPLIT + 2):
        print("File splitter by TS-Labs")
        print("Usage: fsplit.py <input> <piece 1 size> .. <piece N size>")
        return 1

    input_file = sys.argv[1]
    try:
        with open(input_file, "rb") as f_in:
            # Get file size
            st = os.stat(input_file)
            sz = st.st_size
            buf = f_in.read(sz)  # Read entire file into memory
            if len(buf) != sz:
                print("Read file error!")
                return 4
    except IOError:
        print("Input file error!")
        return 1

    # Parse piece sizes
    ssz = [0] * MAX_SPLIT
    n = argc - 2
    for i in range(n):
        try:
            ssz[i] = min(sz, int(sys.argv[i + 2]))
            sz -= ssz[i]
            if sz == 0:
                break
        except ValueError:
            print("Invalid piece size!")
            return 1

    if sz:
        ssz[n] = sz
        n += 1

    # Split file into pieces
    offset = 0
    for i in range(n):
        output_file = f"{input_file}.{i}"
        try:
            with open(output_file, "wb") as f_out:
                bytes_to_write = ssz[i]
                if f_out.write(buf[offset:offset + bytes_to_write]) != bytes_to_write:
                    print("Write file error!")
                    return 5
                offset += bytes_to_write
        except IOError:
            print("Output file error!")
            return 2

    print("DONE!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
