# TS-Labs SPI Flash File System.

## Objectives

- meet NOR flash constraints:
  - only zeroes can be written;
  - to change cell's value from 0 to 1 it must be erased;
  - erase is only possible for large blocks;
- extremely simple implementation, feasible for very constrained systems like AVR and Z80;
- configurable for different SPI Flash devices;
- a configurable area of the flash device can be used for the file system;
- fragmented file system (i.e. file's data may be scattered all over the bulk);
- reasonable wear levelling;
- directories are not supported;
- files can only be written sequentially and cannot be overwritten;
- files can be read randomly;
- files are truncated to chunks and chunk size is equal to erase block;

## Internals

Each chunk contains header and user data.
The first chunk also contains file size and name (only ASCII so far).
Each chunk header contains:

- 'magic' signature,
- next chunk number,
- chunk type (first, one-of).
