#!/usr/bin/env python3
"""xm_scan.py <file.xm>...: header values of an XM which the loader has to bound."""
import struct, sys

def scan(path):
    d = open(path, 'rb').read()
    out = []
    o = 60
    hs, length, restart, nch, npat, nins = struct.unpack_from('<IHHHHH', d, o)
    if length > 256: out.append(f'song length {length}')
    o += hs
    for p in range(npat):
        phs, _, rows, packed = struct.unpack_from('<IBHH', d, o)
        if rows > 256: out.append(f'pattern {p} rows {rows}')
        o += phs + packed
    for i in range(nins):
        ihs = struct.unpack_from('<I', d, o)[0]
        ns = struct.unpack_from('<H', d, o + 27)[0]
        shs = 0
        if ns:
            shs = struct.unpack_from('<I', d, o + 29)[0]
            vp, pp = d[o + 225], d[o + 226]
            vs, vls, vle, ps, pls, ple = d[o + 227:o + 233]
            vf, pf = d[o + 233], d[o + 234]
            for name, n, s, ls, le, f in (('vol', vp, vs, vls, vle, vf), ('pan', pp, ps, pls, ple, pf)):
                if n > 12: out.append(f'ins {i} {name} env points {n} (on={f & 1})')
                if (f & 2) and s >= min(n, 12): out.append(f'ins {i} {name} sustain {s} of {n}')
                if (f & 4) and (ls >= min(n, 12) or le >= min(n, 12) or ls > le): out.append(f'ins {i} {name} loop {ls}..{le} of {n}')
        o += ihs
        total = 0
        for s in range(ns):
            ln, ls, ll = struct.unpack_from('<III', d, o)
            fl = d[o + 14]
            b16 = fl & 16
            if b16 and ln & 1: out.append(f'ins {i} smp {s} 16-bit odd length {ln}')
            if fl & 3:
                if ll == 0: out.append(f'ins {i} smp {s} loop length 0 (type {fl & 3})')
                elif ls + ll > ln: out.append(f'ins {i} smp {s} loop {ls}+{ll} > {ln}')
            total += ln
            o += shs
        o += total
    return out

for p in sys.argv[1:]:
    r = scan(p)
    print(p.split('/')[-1] + ': ' + ('; '.join(r) if r else 'clean'))
