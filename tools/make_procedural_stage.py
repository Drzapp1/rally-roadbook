#!/usr/bin/env python3
"""Procedural rally-raid: synthesize a terrain (multi-octave value-noise heightmap
+ a shaded aerial) as a brand-new track under web/tracks/proc_<name>/, then drive a
rally stage across it with the same engine as make_terrain_stage. Output: a
3D-viewable track AND a validated roadbook — endless fresh stages, no real data.

Usage:  python tools/make_procedural_stage.py <name> [seed] [style]
        style = mixed (default) | dunes | mountains
        (run from repo root)
"""
import os, sys, json, math, random, subprocess
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_terrain_stage import drive, EXE, ROOT, HAMP  # reuse the route+roadbook engine

N = 256
STYLES = {  # octaves, persistence, base grid, peak sharpening exponent
    'mixed':     (6, 0.50, 2, 1.0),
    'dunes':     (4, 0.62, 2, 0.85),
    'mountains': (7, 0.54, 3, 1.5),
}

def gen_height(npr, octaves, persistence, base_g, sharp):
    h = np.zeros((N, N), float); amp = 1.0; tot = 0.0
    for o in range(octaves):
        g = base_g * (2 ** o)
        grid = (npr.random((g + 1, g + 1)) * 255).astype('uint8')
        up = np.asarray(Image.fromarray(grid).resize((N, N), Image.BILINEAR), float) / 255.0
        h += up * amp; tot += amp; amp *= persistence
    h /= tot
    h = (h - h.min()) / ((h.max() - h.min()) or 1.0)
    if sharp != 1.0: h = h ** sharp
    return h.astype(float)

def render_aerial(h):
    gy, gx = np.gradient(h * 180.0)
    slope = np.pi / 2 - np.arctan(np.hypot(gx, gy)); aspect = np.arctan2(-gx, gy)
    az, alt = math.radians(315), math.radians(45)
    shade = np.sin(alt) * np.sin(slope) + np.cos(alt) * np.cos(slope) * np.cos((az - np.pi / 2) - aspect)
    shade = (shade * 0.5 + 0.55).clip(0.25, 1.05)
    stops = np.array([0.0, 0.33, 0.58, 0.80, 1.001])
    cols = np.array([(196, 180, 140), (150, 140, 96), (120, 112, 98), (150, 150, 156), (242, 242, 247)], float)
    hi = h.clip(0, 1); idx = (np.searchsorted(stops, hi) - 1).clip(0, len(stops) - 2)
    t = ((hi - stops[idx]) / (stops[idx + 1] - stops[idx])).clip(0, 1)[..., None]
    c = cols[idx] * (1 - t) + cols[idx + 1] * t
    img = (c * shade[..., None]).clip(0, 255).astype('uint8')
    return Image.fromarray(img, 'RGB')

def main():
    if not EXE: print('ERROR: RoadbookTests.exe not built'); return 1
    if len(sys.argv) < 2: print('usage: make_procedural_stage.py <name> [seed] [style]'); return 2
    name = sys.argv[1]; seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    style = sys.argv[3] if len(sys.argv) > 3 else 'mixed'
    octaves, persistence, base_g, sharp = STYLES.get(style, STYLES['mixed'])
    npr = np.random.default_rng(seed)
    h = gen_height(npr, octaves, persistence, base_g, sharp)
    tid = f'proc_{name}'; title = f'{name.replace("_", " ").title()} (procedural)'
    tdir = os.path.join('web', 'tracks', tid); os.makedirs(tdir, exist_ok=True)
    (h * 65535).clip(0, 65535).astype('<u2').tofile(os.path.join(tdir, 'height.bin'))
    render_aerial(h).save(os.path.join(tdir, 'aerial.jpg'), quality=86)
    json.dump({'id': tid, 'name': title, 'size': N, 'hmin': 0, 'hmax': 65535, 'style': style},
              open(os.path.join(tdir, 'meta.json'), 'w'))
    # tracks manifest
    mp = os.path.join('web', 'tracks', 'manifest.json'); man = json.load(open(mp))
    if not any(m['id'] == tid for m in man):
        man.append({'id': tid, 'name': title}); man.sort(key=lambda m: m['id'])
        json.dump(man, open(mp, 'w'), indent=1)
    # roadbook via the shared engine (route across the generated terrain)
    rows = drive(random.Random(seed * 7919 + 3), h, N)
    scratch = os.path.join(ROOT, 'build'); os.makedirs(scratch, exist_ok=True)
    csv = os.path.join(scratch, f'_{tid}.csv')
    with open(csv, 'w') as f:
        f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
        for r in rows: f.write(','.join(f'{v:.3f}' for v in r) + '\n')
    svg = os.path.join(scratch, f'_{tid}.svg')
    subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
    rb = json.load(open(os.path.splitext(svg)[0] + '.json', encoding='utf-8'))
    rb['trackName'] = title; rb.setdefault('meta', {})['trackName'] = title
    rb['meta']['generatedBy'] = 'procedural'; rb['meta']['sourceTrack'] = tid
    json.dump(rb, open(os.path.join('web', 'roadbooks', f'roadbook_{tid}.json'), 'w', encoding='utf-8'), separators=(',', ':'))
    boxes = rb.get('boxes', []); turns = sum(1 for b in boxes if b['type'] in ('turn', 'hairpin'))
    km = boxes[-1]['distTotalKm'] if boxes else 0
    signs = sorted({p for b in boxes for p in b.get('pictograms', [])})
    print(f'{tid} [{style}]: {len(boxes)} boxes ({turns} turns), {km:.1f} km, signs: {", ".join(signs) or "none"}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
