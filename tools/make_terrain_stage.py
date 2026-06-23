#!/usr/bin/env python3
"""Generate a rally-raid roadbook by driving an open-terrain route ACROSS a real
heightmap (web/tracks/<id>/height.bin) and feeding the resulting ride trace through
the same C++ generator the plugin uses (RoadbookTests.exe). Because rally-raid is
open-terrain navigation, no recorded ride or track racing-line is needed: the
route is a sequence of straights and corners (giving the tulip turns) and the real
elevation profile sampled along it gives the crest / dip / jump / descent signs —
for free, from the shared generator.

Usage:  python tools/make_terrain_stage.py <track_id> [seed]
        (run from repo root; reads web/tracks, writes web/roadbooks)
"""
import os, sys, math, json, array, random, subprocess

ROOT = r'D:\BikesRoadbook'
EXE  = next((p for p in (os.path.join(ROOT, 'build', c, 'RoadbookTests.exe')
                         for c in ('Release', 'Debug')) if os.path.exists(p)), None)
CELL = 25.0     # metres per heightmap cell  -> 256 cells = 6.4 km across
HAMP = 165.0    # metres of relief mapped onto the normalised 0..1 heightmap

def load_height(tid):
    d = os.path.join('web', 'tracks', tid)
    meta = json.load(open(os.path.join(d, 'meta.json'), encoding='utf-8'))
    size = meta['size']; hmin, hmax = meta['hmin'], meta['hmax']
    raw = array.array('H'); raw.frombytes(open(os.path.join(d, 'height.bin'), 'rb').read())
    span = (hmax - hmin) or 1.0
    H = [[(raw[z * size + x] - hmin) / span for x in range(size)] for z in range(size)]
    return H, size, meta.get('name', tid)

def sample(H, size, fx, fz):
    fx = min(size - 1.001, max(0.0, fx)); fz = min(size - 1.001, max(0.0, fz))
    x0, z0 = int(fx), int(fz); dx, dz = fx - x0, fz - z0
    a = H[z0][x0] * (1 - dx) + H[z0][x0 + 1] * dx
    b = H[z0 + 1][x0] * (1 - dx) + H[z0 + 1][x0 + 1] * dx
    return a * (1 - dz) + b * dz

def drive(rng, H, size):
    """Drive a straights-and-corners rally line across the terrain, sampling
    elevation from the heightmap. Stays inside the field by steering to centre."""
    W = size * CELL; m = W * 0.10; STEP = 3.0
    x = rng.uniform(m, W - m); z = m * 1.15
    head = math.radians(rng.uniform(-25, 25))
    target = rng.uniform(8000, 13000)
    rows = []; dist = 0.0
    def emit():
        y = HAMP * sample(H, size, x / CELL, z / CELL)
        rows.append((dist / 22.0, x, y, z, math.degrees(head) % 360, 0, 0, 0, 22.0, 0, dist))
    while dist < target:
        seg = rng.uniform(90, 360)
        n = max(1, int(seg / STEP))
        for _ in range(n):
            x += STEP * math.sin(head); z += STEP * math.cos(head); dist += STEP
            if x < m or x > W - m or z < m or z > W - m:          # near edge -> steer inward
                x = min(W - m, max(m, x)); z = min(W - m, max(m, z))
                head = math.atan2(W / 2 - x, W / 2 - z) + math.radians(rng.uniform(-20, 20))
            emit()
        # a corner: ~25..140 deg either way (occasionally a hairpin)
        mag = rng.uniform(0.45, 2.0)
        if rng.random() < 0.12: mag = rng.uniform(2.3, 2.9)
        head += (1 if rng.random() < 0.5 else -1) * mag
    return rows

def main():
    if not EXE: print('ERROR: RoadbookTests.exe not built'); return 1
    if len(sys.argv) < 2: print('usage: make_terrain_stage.py <track_id> [seed]'); return 2
    tid = sys.argv[1]; seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(hash((tid, seed)) & 0x7fffffff)
    H, size, name = load_height(tid)
    rows = drive(rng, H, size)
    scratch = os.path.join(ROOT, 'build'); os.makedirs(scratch, exist_ok=True)
    csv = os.path.join(scratch, f'_terrain_{tid}.csv')
    with open(csv, 'w') as f:
        f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
        for r in rows: f.write(','.join(f'{v:.3f}' for v in r) + '\n')
    svg = os.path.join(scratch, f'_terrain_{tid}.svg')
    subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
    rb = json.load(open(os.path.splitext(svg)[0] + '.json', encoding='utf-8'))
    title = f'{name} — terrain stage'
    rb['trackName'] = title; rb.setdefault('meta', {})['trackName'] = title
    rb['meta']['generatedBy'] = 'terrain'; rb['meta']['sourceTrack'] = tid
    out = os.path.join('web', 'roadbooks', f'roadbook_terrain_{tid}.json')
    json.dump(rb, open(out, 'w', encoding='utf-8'), separators=(',', ':'))
    boxes = rb.get('boxes', [])
    turns = sum(1 for b in boxes if b['type'] in ('turn', 'hairpin'))
    km = boxes[-1]['distTotalKm'] if boxes else 0
    signs = sorted({p for b in boxes for p in b.get('pictograms', [])})
    print(f'{tid}: {len(boxes)} boxes ({turns} turns), {km:.1f} km, signs: {", ".join(signs) or "none"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
