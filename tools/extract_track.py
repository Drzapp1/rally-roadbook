#!/usr/bin/env python3
"""Extract a real MX Bikes track's terrain for the web 3D viewer:
  - decode the PiBoSo .trh heightmap (TRH<magic> + w + h + w*h uint16) -> downsampled
    little-endian uint16 binary (web/tracks/<id>/height.bin)
  - read the aerial track_map.tga -> aerial.jpg
  - meta.json (grid size, raw height min/max, name)

Usage:  python tools/extract_track.py "<track folder>" <id> ["Display Name"]
"""
import numpy as np, struct, json, os, sys
from PIL import Image

N = 256  # web heightmap grid (256x256 verts)

def decode_trh(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'TRH\x00', 'not a TRH file: ' + repr(d[:4])
    w = struct.unpack('<I', d[4:8])[0]; h = struct.unpack('<I', d[8:12])[0]
    a = np.frombuffer(d[12:12 + w*h*2], dtype='<u2').reshape(h, w)   # uint16 view (no copy)
    step = max(1, w // 512)                                          # stride huge maps down first
    small = np.ascontiguousarray(a[::step, ::step]).astype(np.float32)
    return small, w

def read_tga(path):
    d = open(path, 'rb').read()
    w = int.from_bytes(d[12:14], 'little'); h = int.from_bytes(d[14:16], 'little'); bpp = d[16]
    off = 18 + d[0]; px = d[off:off + w*h*(bpp//8)]
    mode = 'RGBA' if bpp == 32 else 'RGB'; raw = 'BGRA' if bpp == 32 else 'BGR'
    im = Image.frombytes(mode, (w, h), px, 'raw', raw)
    if not (d[17] & 0x20):  # bottom-origin -> flip to top-origin
        im = im.transpose(Image.FLIP_TOP_BOTTOM)
    return im.convert('RGB')

def find(folder, ext):
    for f in os.listdir(folder):
        if f.lower().endswith(ext): return os.path.join(folder, f)
    return None

def main():
    folder, tid = sys.argv[1], sys.argv[2]
    name = sys.argv[3] if len(sys.argv) > 3 else tid
    out = os.path.join('web', 'tracks', tid); os.makedirs(out, exist_ok=True)

    hm, raw_w = decode_trh(find(folder, '.trh'))
    ds = np.asarray(Image.fromarray(hm, 'F').resize((N, N), Image.LANCZOS), dtype=np.float32)
    np.clip(ds, 0, 65535).astype('<u2').tofile(os.path.join(out, 'height.bin'))

    aer = read_tga(os.path.join(folder, 'track_map.tga'))
    aer.save(os.path.join(out, 'aerial.jpg'), quality=88)

    json.dump({'id': tid, 'name': name, 'size': N,
               'hmin': float(hm.min()), 'hmax': float(hm.max())},
              open(os.path.join(out, 'meta.json'), 'w'))
    print(f'{tid}: heightmap {N}x{N} (raw {raw_w}px, {int(hm.min())}..{int(hm.max())}), aerial {aer.size}')

    # rebuild the tracks manifest from every extracted track
    tracksdir = os.path.join('web', 'tracks'); man = []
    for t in sorted(os.listdir(tracksdir)):
        mp = os.path.join(tracksdir, t, 'meta.json')
        if os.path.isfile(mp):
            m = json.load(open(mp)); man.append({'id': m['id'], 'name': m.get('name', m['id'])})
    json.dump(man, open(os.path.join(tracksdir, 'manifest.json'), 'w'), indent=1)
    print('manifest:', ', '.join(t['id'] for t in man))

if __name__ == '__main__':
    main()
