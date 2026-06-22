#!/usr/bin/env python3
"""Extract a real MX Bikes track's terrain for the web 3D viewer:
  - decode the PiBoSo .trh heightmap (TRH magic + w + h + w*h uint16)
  - PiBoSo pegs "outside the terrain" cells at exactly 0 or 65535 (no-data
    sentinels). Left in, those blocks render as towering walls / pits. So:
    mask-aware downsample (average only valid cells) -> inpaint the gaps from
    their neighbours -> light smooth. Result is clean real terrain, no cliffs.
  - downsampled little-endian uint16 -> web/tracks/<id>/height.bin
  - aerial track_map.tga -> aerial.jpg ; meta.json ; rebuild manifest.json

Usage:  python tools/extract_track.py "<track folder>" <id> ["Display Name"]
"""
import numpy as np, struct, json, os, sys
from PIL import Image

N = 256  # web heightmap grid (256x256 verts)

def decode_trh(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'TRH\x00', 'not a TRH file: ' + repr(d[:4])
    w = struct.unpack('<I', d[4:8])[0]; h = struct.unpack('<I', d[8:12])[0]
    return np.frombuffer(d[12:12 + w*h*2], dtype='<u2').reshape(h, w), w

def inpaint_nan(a):
    """Fill NaN cells from the mean of their valid 4-neighbours, iteratively."""
    a = a.astype(np.float32).copy()
    for _ in range(400):
        nan = np.isnan(a)
        if not nan.any(): break
        nb = np.zeros_like(a); cnt = np.zeros_like(a)
        for dy, dx in ((-1,0),(1,0),(0,-1),(0,1)):
            s = np.roll(np.where(nan, 0.0, a), (dy, dx), (0, 1))
            m = np.roll((~nan).astype(np.float32), (dy, dx), (0, 1))
            nb += s; cnt += m
        fill = nan & (cnt > 0)
        a[fill] = nb[fill] / cnt[fill]
        if not fill.any():
            a[nan] = np.nanmean(a); break
    return np.nan_to_num(a, nan=float(np.nanmean(a)))

def box_blur(a):
    p = np.pad(a, 1, mode='edge')
    return (p[:-2,:-2]+p[:-2,1:-1]+p[:-2,2:]
          + p[1:-1,:-2]+p[1:-1,1:-1]+p[1:-1,2:]
          + p[2:,:-2]+p[2:,1:-1]+p[2:,2:]) / 9.0

def process_heightmap(a):
    h, w = a.shape
    if min(w, h) >= N and max(w, h) <= 6000:           # mask-aware block downsample
        cw, ch = (w // N) * N, (h // N) * N
        ac = a[:ch, :cw].astype(np.float32)
        valid = ((ac > 0) & (ac < 65535)).astype(np.float32)
        by, bx = ch // N, cw // N
        num = (ac * valid).reshape(N, by, N, bx).sum(axis=(1, 3))
        den = valid.reshape(N, by, N, bx).sum(axis=(1, 3))
        ds = np.where(den > 0, num / np.maximum(den, 1.0), np.nan)
    else:                                              # huge / tiny: stride fallback
        step = max(1, w // N)
        s = a[::step, ::step][:N, :N].astype(np.float32)
        ds = np.where((s > 0) & (s < 65535), s, np.nan)
        if ds.shape != (N, N):
            ds = np.asarray(Image.fromarray(np.nan_to_num(ds, nan=float(np.nanmedian(ds))), 'F').resize((N, N), Image.LANCZOS))
    return box_blur(inpaint_nan(ds))

def read_tga(path):
    d = open(path, 'rb').read()
    w = int.from_bytes(d[12:14], 'little'); h = int.from_bytes(d[14:16], 'little'); bpp = d[16]
    off = 18 + d[0]; px = d[off:off + w*h*(bpp//8)]
    mode = 'RGBA' if bpp == 32 else 'RGB'; raw = 'BGRA' if bpp == 32 else 'BGR'
    im = Image.frombytes(mode, (w, h), px, 'raw', raw)
    if not (d[17] & 0x20):
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

    raw, raw_w = decode_trh(find(folder, '.trh'))
    ds = process_heightmap(raw)
    np.clip(ds, 0, 65535).astype('<u2').tofile(os.path.join(out, 'height.bin'))

    aer = read_tga(os.path.join(folder, 'track_map.tga'))
    aer.save(os.path.join(out, 'aerial.jpg'), quality=88)

    json.dump({'id': tid, 'name': name, 'size': N,
               'hmin': float(ds.min()), 'hmax': float(ds.max())},
              open(os.path.join(out, 'meta.json'), 'w'))
    print(f'{tid}: heightmap {N}x{N} (raw {raw_w}px, cleaned {int(ds.min())}..{int(ds.max())}), aerial {aer.size}')

    tracksdir = os.path.join('web', 'tracks'); man = []
    for t in sorted(os.listdir(tracksdir)):
        mp = os.path.join(tracksdir, t, 'meta.json')
        if os.path.isfile(mp):
            m = json.load(open(mp)); man.append({'id': m['id'], 'name': m.get('name', m['id'])})
    json.dump(man, open(os.path.join(tracksdir, 'manifest.json'), 'w'), indent=1)
    print('manifest:', ', '.join(t['id'] for t in man))

if __name__ == '__main__':
    main()
