#!/usr/bin/env python3
"""Trim leading/trailing silence from the co-driver voice clips so concatenated
pace-note sentences flow at a natural pace (SAPI leaves ~0.8s tails)."""
import struct, os, glob, array, sys

BASE = sys.argv[1] if len(sys.argv) > 1 else r'D:\BikesRoadbook\release\RallyRoadbook\Roadbook_data\audio\voice'

def load(p):
    b = open(p, 'rb').read(); o = 12; rate = bits = ch = 0; data = b''
    while o + 8 <= len(b):
        cid = b[o:o+4]; csz = struct.unpack('<I', b[o+4:o+8])[0]
        if cid == b'fmt ': ch, = struct.unpack('<H', b[o+10:o+12]); rate, = struct.unpack('<I', b[o+12:o+16]); bits, = struct.unpack('<H', b[o+22:o+24])
        elif cid == b'data': data = b[o+8:o+8+csz]
        o += 8 + csz + (csz & 1)
    return data, rate, bits, ch

def write(p, data, rate, bits, ch):
    hdr = b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVE' + b'fmt ' + struct.pack('<IHHIIHH', 16, 1, ch, rate, rate*ch*bits//8, ch*bits//8, bits) + b'data' + struct.pack('<I', len(data))
    open(p, 'wb').write(hdr + data)

def main():
    files = glob.glob(os.path.join(BASE, '*.wav'))
    before = after = 0.0; thr = 300
    for f in files:
        data, rate, bits, ch = load(f)
        if bits != 16: continue
        a = array.array('h'); a.frombytes(data); before += len(a)/max(rate,1)
        first = 0
        while first < len(a) and abs(a[first]) < thr: first += 1
        last = len(a) - 1
        while last > first and abs(a[last]) < thr: last -= 1
        if last <= first: continue
        pad = int(0.018 * rate)
        s = max(0, first - pad); e = min(len(a), last + 1 + pad)
        t = a[s:e]; write(f, t.tobytes(), rate, bits, ch); after += len(t)/rate
    print(f'trimmed {len(files)} clips: {before:.1f}s -> {after:.1f}s')

if __name__ == '__main__':
    main()
