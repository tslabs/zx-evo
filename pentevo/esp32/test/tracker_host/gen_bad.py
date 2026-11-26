#!/usr/bin/env python3
"""gen_bad.py <dir>: small XM files, one per loader bug, and the clean ones they are compared with.

Every module plays instrument 2 (an 8-bit sine with a forward loop) on channel 0;
instrument 1 is not played unless a case says so.
"""
import math, struct, sys

def sample_data(values, bits):
    out, prev = bytearray(), 0
    for v in values:
        d = v - prev; prev = v
        out += struct.pack('<h', d) if bits == 16 else struct.pack('<b', ((d + 128) & 255) - 128)
    return bytes(out)

def sine(n, amp):
    return [int(amp * math.sin(2 * math.pi * i / 32)) for i in range(n)]

def sample(values=None, bits=8, loop=(0, 0), ltype=0, raw=None, length=None):
    data = raw if raw is not None else sample_data(values, bits)
    ln = len(data) if length is None else length
    flags = ltype | (16 if bits == 16 else 0)
    hdr = struct.pack('<IIIBbBBbB', ln, loop[0], loop[1], 64, 0, flags, 128, 0, 0) + b'smp'.ljust(22, b'\0')
    return hdr, data

def instrument(samples, env_points=0, env=(0, 0, 0), env_flags=0, keymap=None):
    hdr = struct.pack('<I', 263) + b'ins'.ljust(22, b'\0') + b'\0' + struct.pack('<H', len(samples))
    hdr += struct.pack('<I', 40) + bytes(keymap or [0] * 96)
    pts = b''.join(struct.pack('<HH', 4 * i, 64 - i) for i in range(12))
    hdr += pts + pts
    hdr += bytes([env_points, 0, env[0], env[1], env[2], 0, 0, 0, env_flags, 0, 0, 0, 0, 0]) + struct.pack('<H', 0)
    hdr += bytes(22)
    assert len(hdr) == 263
    return hdr + b''.join(h for h, d in samples) + b''.join(d for h, d in samples)

def pattern(rows=64, nch=2, hdr_rows=None, ins=2, extra=b''):
    data = bytearray()
    for r in range(rows):
        if r % 16 == 0: data += bytes([0x83, 49 + (r // 16), ins])
        else: data += b'\x80'
        data += b'\x80' * (nch - 1)
    data += extra
    return struct.pack('<IBHH', 9, 0, rows if hdr_rows is None else hdr_rows, len(data)) + bytes(data)

def module(patterns, instruments, length=1, orders=None, nch=2):
    h = b'Extended Module: ' + b'bad'.ljust(20, b'\0') + b'\x1a' + b'gen_bad.py'.ljust(20, b'\0') + struct.pack('<H', 0x0104)
    o = bytes(orders or [0] * length)[:256].ljust(256, b'\0')
    h += struct.pack('<IHHHHHHHH', 276, length, 0, nch, len(patterns), len(instruments), 1, 6, 125) + o
    return h + b''.join(patterns) + b''.join(instruments)

def base_instruments(i1=None, i2=None):
    s2 = sample(sine(256, 100), 8, loop=(0, 256), ltype=1)
    return [i1 or instrument([sample(sine(100, 3000), 16)]), i2 or instrument([s2])]

cases = {}
cases['base'] = module([pattern()], base_instruments())
# a 16-bit sample of an odd byte length: instrument 2 after it must load as in 'base'
odd = sample(raw=sample_data(sine(100, 3000), 16) + b'\x55', bits=16)
cases['odd16'] = module([pattern()], base_instruments(i1=instrument([odd])))
# a 16-bit sample right after an 8-bit one of odd length, the 16-bit one is played
i1 = instrument([sample(sine(101, 100), 8), sample(sine(300, 3000), 16, loop=(0, 600), ltype=1)], keymap=[1] * 96)
cases['align'] = module([pattern(ins=1)], base_instruments(i1=i1))
# song length 256: the order check counted with a uint8_t
cases['len256'] = module([pattern()], base_instruments(), length=256)
# song length 300: the order table has 256 entries
cases['len300'] = module([pattern()], base_instruments(), length=300)
# 300 rows (the row counter of the player is 8-bit): no finding
cases['rows300'] = module([pattern(rows=300)], base_instruments())
# packed data of 64 rows in a pattern of 2 rows (the stream parser rejects it)
cases['slots'] = module([pattern(hdr_rows=2)], base_instruments())
# 20 envelope points (points[] has 12), the envelope on
cases['env20'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(0, 256), ltype=1)], env_points=20, env_flags=1)))
# sustain point 200 of 6
cases['sustain'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(0, 256), ltype=1)], env_points=6, env=(200, 0, 3), env_flags=3)))
# envelope loop to point 50 of 6
cases['envloop'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(0, 256), ltype=1)], env_points=6, env=(0, 1, 50), env_flags=5)))
# sample loop 10 times longer than the sample
cases['loopbig'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(0, 2560), ltype=1)])))
# forward loop of length 0 at the end of the sample
cases['loop0'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(256, 0), ltype=1)])))
cases['loop0p'] = module([pattern()], base_instruments(i2=instrument([sample(sine(256, 100), 8, loop=(100, 0), ltype=2)])))

for name, data in cases.items():
    open(f'{sys.argv[1]}/{name}.xm', 'wb').write(data)
print(' '.join(cases))
