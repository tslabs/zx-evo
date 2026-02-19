import sys
import os
import time
import serial
import struct
import argparse
from enum import Enum
from collections import deque

TRD_SZ = 256 * 16 * 255
BAUD = 115200
TOKEN1 = 0xAA
TOKEN2 = 0x55
ANS1 = 0xCC
ANS2 = 0xEE

# How often to poll the filesystem for external image changes (seconds)
IMG_POLL_INTERVAL = 0.5

# Serial read on Windows can occasionally throw transient errors (e.g. ClearCommError failed / Access denied)
# especially if the device resets or the driver glitches. We treat these as recoverable and try to reopen.
SERIAL_REOPEN_BACKOFF_S = 0.25
SERIAL_REOPEN_BACKOFF_MAX_S = 5.0


class OPCODE(Enum):
    OP_RD = 5
    OP_WR = 6

class STATE(Enum):
    ST_IDLE = 0
    ST_RECEIVE_START_TOKEN2 = 1
    ST_RECEIVE_HEADER = 2
    ST_RECEIVE_DATA = 3

class REQ:
    def __init__(self, data=None):
        if data:
            self.drv, self.op, self.trk, self.sec = struct.unpack('>BBBB', data[:4])
            self.crc = data[4] if len(data) > 4 else 0
        else:
            self.drv = self.op = self.trk = self.sec = self.crc = 0

class SECT:
    def __init__(self, data=None):
        if data:
            self.ack = data[:2]
            self.data = data[2:-1]
            self.crc = data[-1]
        else:
            self.ack = [0, 0]
            self.data = [0] * 256
            self.crc = 0

def update_xor(xor_val, data, num):
    for i in range(num):
        xor_val ^= data[i]
    return xor_val

def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog=os.path.basename(sys.argv[0]),
        description="RS-232 VDOS TRD image mounter by TS-Labs",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    parser.add_argument('-a', metavar='FILE.trd', type=str, help='TRD image to mount on drive A')
    parser.add_argument('-b', metavar='FILE.trd', type=str, help='TRD image to mount on drive B')
    parser.add_argument('-c', metavar='FILE.trd', type=str, help='TRD image to mount on drive C')
    parser.add_argument('-d', metavar='FILE.trd', type=str, help='TRD image to mount on drive D')

    parser.add_argument('-com', metavar='PORT', type=str, default='COM1',
                        help='Serial port name (default: COM1)')
    parser.add_argument('-baud', metavar='BAUD', type=int, default=BAUD,
                        help=f'UART baudrate (default: {BAUD})')

    parser.add_argument('-slowpoke', action='store_true',
                        help='Insert delays into transmit')
    parser.add_argument('-log', action='store_true',
                        help='Scroll log for disk operations')

    args = parser.parse_args(argv)

    global baud, cport, log, slow, trd, drvs
    baud = args.baud
    cport = args.com
    log = args.log
    slow = args.slowpoke
    trd = [args.a, args.b, args.c, args.d]
    drvs = sum(1 for t in trd if t)

    return parser, drvs

def _stat_sig(path):
    """Return a cheap signature that changes when file content is likely updated."""
    st = os.stat(path)
    # mtime_ns is best where available; size helps detect some edit patterns.
    mtime_ns = getattr(st, "st_mtime_ns", int(st.st_mtime * 1_000_000_000))
    return (st.st_size, mtime_ns)

def _load_trd_into(img, drv_index, path):
    with open(path, 'rb') as f:
        raw = f.read(TRD_SZ)
    # Keep behavior compatible with original: always serve TRD_SZ bytes.
    if len(raw) < TRD_SZ:
        raw = raw + (b'\x00' * (TRD_SZ - len(raw)))
    img[drv_index] = list(raw[:TRD_SZ])

def refresh_images_if_changed(img, trd_paths, last_sig, verbose=False):
    """Reload any mounted image whose on-disk file changed since last check."""
    for i, p in enumerate(trd_paths):
        if not p:
            continue
        try:
            sig = _stat_sig(p)
        except OSError:
            # File disappeared or inaccessible; keep serving old image.
            if verbose:
                print(f"Image check: can't stat {p} (drive {chr(ord('A') + i)})")
            continue

        if last_sig[i] is None:
            last_sig[i] = sig
            continue

        if sig != last_sig[i]:
            try:
                _load_trd_into(img, i, p)
                last_sig[i] = sig
                print(f"Reloaded {p} -> drive {chr(ord('A') + i)}")
            except OSError as e:
                # If the file is mid-write by another process, this can happen.
                # Keep serving old image; we'll retry next poll.
                if verbose:
                    print(f"Image reload failed for {p}: {e}")


def _open_serial_port():
    try:
        port = serial.Serial(cport, baud, timeout=0.001)
        print(f"{cport} opened successfully ({baud} baud)\n")
        return port
    except Exception:
        print(f"Can't open {cport}")
        return None


def _safe_port_read(port, nbytes):
    try:
        return port.read(nbytes)
    except serial.SerialException as e:
        # Recoverable path: close the port and let the caller reopen it.
        try:
            port.close()
        except Exception:
            pass
        if log:
            print(f"Serial read failed: {e}. Will try to reopen {cport}...")
        return None

def main():
    img = [[0] * TRD_SZ for _ in range(4)]
    fifo_in = deque(maxlen=512)
    state = STATE.ST_IDLE

    parser, drvs = parse_args()
    if not drvs:
        parser.print_help()
        return 1

    # Track image file signatures so we can hot-reload on external modifications.
    last_sig = [None, None, None, None]
    next_img_poll = time.monotonic()

    for i, t in enumerate(trd):
        if t:
            try:
                _load_trd_into(img, i, t)
                last_sig[i] = _stat_sig(t)
                print(f"{t} opened successfully")
            except Exception:
                print(f"Can't open: {t}")
                return 2

    port = _open_serial_port()
    if port is None:
        return 3

    while True:
        # Periodically check whether any mounted image changed on disk.
        now = time.monotonic()
        if now >= next_img_poll:
            refresh_images_if_changed(img, trd, last_sig, verbose=log)
            next_img_poll = now + IMG_POLL_INTERVAL

        if port is None:
            # Keep trying to reopen with backoff.
            if not hasattr(main, "_reopen_backoff"):
                main._reopen_backoff = SERIAL_REOPEN_BACKOFF_S
            time.sleep(main._reopen_backoff)
            port = _open_serial_port()
            if port is None:
                main._reopen_backoff = min(main._reopen_backoff * 2.0, SERIAL_REOPEN_BACKOFF_MAX_S)
                continue
            main._reopen_backoff = SERIAL_REOPEN_BACKOFF_S

        data = _safe_port_read(port, 512)
        if data is None:
            port = None
            continue
        if data:
            fifo_in.extend(list(data))

        while fifo_in:
            if state == STATE.ST_IDLE:
                if fifo_in[0] == TOKEN1:
                    fifo_in.popleft()
                    state = STATE.ST_RECEIVE_START_TOKEN2
                else:
                    fifo_in.popleft()

            elif state == STATE.ST_RECEIVE_START_TOKEN2:
                if fifo_in[0] == TOKEN2:
                    fifo_in.popleft()
                    state = STATE.ST_RECEIVE_HEADER
                else:
                    fifo_in.popleft()
                    state = STATE.ST_IDLE

            elif state == STATE.ST_RECEIVE_HEADER:
                if len(fifo_in) >= 5:
                    data = bytes([fifo_in.popleft() for _ in range(5)])
                    req = REQ(data)
                    print(f"Op: {req.op} Drv: {req.drv} Trk: {req.trk} Sec: {req.sec}    " + ("\n" if log else "\r"), end="")

                    if req.drv > 3:
                        if log:
                            print("Wrong drive!")
                    if req.sec > 15:
                        if log:
                            print("Wrong sector!")

                    disk_ptr = (req.trk * 16 + req.sec) * 256

                    if req.op == OPCODE.OP_RD.value:
                        sect = SECT()
                        sect.ack = [ANS1, ANS2]
                        sect.data = img[req.drv][disk_ptr:disk_ptr + 256]
                        sect.crc = update_xor(ANS1 ^ ANS2, sect.data, 256)
                        out_data = bytes(sect.ack + sect.data + [sect.crc])
                        port.write(out_data)
                        if slow:
                            time.sleep(0.003)
                        state = STATE.ST_IDLE

                    elif req.op == OPCODE.OP_WR.value:
                        state = STATE.ST_RECEIVE_DATA

                    else:
                        if log:
                            print("Wrong operation!")
                        state = STATE.ST_IDLE
                else:
                    break

            elif state == STATE.ST_RECEIVE_DATA:
                if len(fifo_in) >= 259:
                    data = bytes([fifo_in.popleft() for _ in range(259)])
                    sect = SECT(data)
                    disk_ptr = (req.trk * 16 + req.sec) * 256
                    img[req.drv][disk_ptr:disk_ptr + 256] = sect.data
                    state = STATE.ST_IDLE
                else:
                    break

if __name__ == "__main__":
    sys.exit(main())
