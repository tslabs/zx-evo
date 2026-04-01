#!/usr/bin/env python3
import argparse
import sys
import time
from typing import Optional

import serial

SOH = 0x01
EOT = 0x04
ACK = 0x06
NAK = 0x15
CAN = 0x18
CRCCHR = ord("C")

BLOCK_SIZE = 128


def crc16_xmodem(data: bytes) -> int:
  crc = 0
  for b in data:
    crc ^= (b << 8)
    for _ in range(8):
      if crc & 0x8000:
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF
      else:
        crc = (crc << 1) & 0xFFFF
  return crc


def read_byte(ser: serial.Serial, timeout_s: float) -> Optional[int]:
  end = time.time() + timeout_s
  while time.time() < end:
    b = ser.read(1)
    if b:
      return b[0]
  return None


def write_bytes(ser: serial.Serial, data: bytes) -> None:
  ser.write(data)
  ser.flush()


def format_byte(ch: int) -> str:
  if 32 <= ch <= 126:
    return f"0x{ch:02X} ('{chr(ch)}')"
  return f"0x{ch:02X}"


def wait_for_protocol_byte(
  ser: serial.Serial,
  timeout_s: float,
  allowed: tuple[int, ...],
  verbose: bool,
  context: str,
) -> Optional[int]:
  end = time.time() + timeout_s
  while time.time() < end:
    ch = read_byte(ser, min(0.2, max(0.01, end - time.time())))
    if ch is None:
      continue
    if ch in allowed:
      return ch
    if verbose:
      print(f"Ignoring junk {format_byte(ch)} while waiting for {context}.", file=sys.stderr)
  return None


def wait_receiver_ready(ser: serial.Serial, timeout_s: float, verbose: bool) -> None:
  ch = wait_for_protocol_byte(ser, timeout_s, (CRCCHR, NAK, CAN), verbose, "receiver ready")
  if ch == CRCCHR:
    if verbose:
      print("Receiver requested CRC (got 'C').", file=sys.stderr)
    return
  if ch == NAK:
    if verbose:
      print("Receiver sent NAK (checksum mode request). Using CRC anyway.", file=sys.stderr)
    return
  if ch == CAN:
    raise RuntimeError("Receiver cancelled (CAN).")
  raise TimeoutError("Timeout waiting for receiver ('C' or NAK).")


def send_block(ser: serial.Serial, blkno: int, payload: bytes) -> None:
  assert len(payload) == BLOCK_SIZE
  crc = crc16_xmodem(payload)
  pkt = bytes([
    SOH,
    blkno & 0xFF,
    (0xFF - (blkno & 0xFF)) & 0xFF,
  ]) + payload + bytes([(crc >> 8) & 0xFF, crc & 0xFF])
  write_bytes(ser, pkt)


def send_file_xmodem_crc(
  port: str,
  baud: int,
  filename: str,
  timeout_s: float,
  retries: int,
  inter_byte_delay_ms: float,
  verbose: bool,
) -> None:
  with serial.Serial(port=port, baudrate=baud, timeout=0, write_timeout=2) as ser:
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    wait_receiver_ready(ser, timeout_s=timeout_s, verbose=verbose)

    blkno = 1

    with open(filename, "rb") as f:
      while True:
        chunk = f.read(BLOCK_SIZE)
        if not chunk:
          break

        if len(chunk) < BLOCK_SIZE:
          chunk += b"\x1A" * (BLOCK_SIZE - len(chunk))  # SUB padding

        attempt = 0
        while True:
          if verbose:
            print(f"Sending block {blkno}...", file=sys.stderr)

          if inter_byte_delay_ms > 0:
            crc = crc16_xmodem(chunk)
            pkt = bytearray()
            pkt.extend([SOH, blkno & 0xFF, (0xFF - (blkno & 0xFF)) & 0xFF])
            pkt.extend(chunk)
            pkt.extend([(crc >> 8) & 0xFF, crc & 0xFF])
            for b in pkt:
              write_bytes(ser, bytes([b]))
              time.sleep(inter_byte_delay_ms / 1000.0)
          else:
            send_block(ser, blkno, chunk)

          resp = wait_for_protocol_byte(ser, timeout_s, (ACK, NAK, CAN), verbose, f"response to block {blkno}")
          if resp == ACK:
            blkno = (blkno + 1) & 0xFF
            break
          if resp == NAK or resp is None:
            attempt += 1
            if attempt > retries:
              raise RuntimeError(f"Block {blkno} failed after {retries} retries.")
            if verbose:
              print(f"Retry block {blkno} (resp={resp}).", file=sys.stderr)
            continue
          if resp == CAN:
            raise RuntimeError("Receiver cancelled (CAN).")

          attempt += 1
          if attempt > retries:
            raise RuntimeError(f"Block {blkno} failed (unexpected resp={resp}) after {retries} retries.")

    attempt = 0
    while True:
      if verbose:
        print("Sending EOT...", file=sys.stderr)
      write_bytes(ser, bytes([EOT]))
      resp = wait_for_protocol_byte(ser, timeout_s, (ACK, NAK, CAN), verbose, "response to EOT")
      if resp == ACK:
        if verbose:
          print("Done (ACK after EOT).", file=sys.stderr)
        return
      if resp == CAN:
        raise RuntimeError("Receiver cancelled (CAN).")
      attempt += 1
      if attempt > retries:
        raise RuntimeError("EOT not acknowledged.")


def main() -> int:
  ap = argparse.ArgumentParser(description="Send file over serial using X-MODEM CRC (128-byte blocks).")
  ap.add_argument("port", help="Serial port, e.g. COM5 or /dev/ttyUSB0")
  ap.add_argument("file", help="File to send")
  ap.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
  ap.add_argument("-t", "--timeout", type=float, default=10.0, help="Timeout seconds waiting for receiver/ACK (default: 10)")
  ap.add_argument("-r", "--retries", type=int, default=16, help="Retries per block (default: 16)")
  ap.add_argument("--inter-byte-delay-ms", type=float, default=0.0, help="Optional delay between bytes (slow receivers)")
  ap.add_argument("-v", "--verbose", action="store_true", help="Verbose logging to stderr")
  args = ap.parse_args()

  try:
    send_file_xmodem_crc(
      port=args.port,
      baud=args.baud,
      filename=args.file,
      timeout_s=args.timeout,
      retries=args.retries,
      inter_byte_delay_ms=args.inter_byte_delay_ms,
      verbose=args.verbose,
    )
    return 0
  except Exception as e:
    print(f"ERROR: {e}", file=sys.stderr)
    return 2


if __name__ == "__main__":
  raise SystemExit(main())
