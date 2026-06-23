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
import os, sys, math, json, array, random, subprocess, heapq

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

def route_follow(H, size, rng):
    """Least-cost path that follows the real terrain — biased toward low ground
    (valleys/wadis, where rally-raid lines naturally run) and gentle slopes — so the
    turns come from the landscape, not a synthetic meander. Dijkstra on a coarse grid."""
    G = 104; cells = size / G
    ch = lambda gx, gz: sample(H, size, min(size - 1, (gx + 0.5) * cells), min(size - 1, (gz + 0.5) * cells))
    margin = 6
    sx = rng.randint(margin, G - margin); sz = 2
    gx = rng.randint(margin, G - margin); gz = G - 3
    start, goal = (sx, sz), (gx, gz)
    INF = float('inf'); dist = {start: 0.0}; prev = {}; pq = [(0.0, start)]
    nb = [(-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, 1), (-1, 1), (1, -1)]
    while pq:
        d, (cx, cz) = heapq.heappop(pq)
        if (cx, cz) == goal: break
        if d > dist.get((cx, cz), INF): continue
        h0 = ch(cx, cz)
        for dx, dz in nb:
            nx, nz = cx + dx, cz + dz
            if not (0 <= nx < G and 0 <= nz < G): continue
            h1 = ch(nx, nz); slope = abs(h1 - h0)
            stepc = 1.0 if (dx == 0 or dz == 0) else 1.4142
            cost = stepc * (0.35 + 2.6 * h1 + 7.0 * slope)     # low ground + gentle slope
            nd = d + cost
            if nd < dist.get((nx, nz), INF):
                dist[(nx, nz)] = nd; prev[(nx, nz)] = (cx, cz); heapq.heappush(pq, (nd, (nx, nz)))
    path = [goal]; cur = goal
    while cur in prev: cur = prev[cur]; path.append(cur)
    path.reverse()
    gcell = size * CELL / G
    return [((px + 0.5) * gcell, (pz + 0.5) * gcell) for px, pz in path]

def walk_path(pts, H, size):
    """Densify the terrain-following waypoints to a 3 m ride trace, smoothing out the
    grid staircase so the generator sees the terrain's real bends as turns."""
    STEP = 3.0; dense = []
    for i in range(len(pts) - 1):
        (x0, z0), (x1, z1) = pts[i], pts[i + 1]
        seg = math.hypot(x1 - x0, z1 - z0); n = max(1, int(seg / STEP))
        for k in range(n): t = k / n; dense.append((x0 + (x1 - x0) * t, z0 + (z1 - z0) * t))
    dense.append(pts[-1])
    sm = []
    for i in range(len(dense)):                                # moving-average smooth
        a, b = max(0, i - 4), min(len(dense), i + 5); seg = dense[a:b]
        sm.append((sum(p[0] for p in seg) / len(seg), sum(p[1] for p in seg) / len(seg)))
    rows = []; dist = 0.0
    for i, (x, z) in enumerate(sm):
        if i > 0:
            dx, dz = x - sm[i - 1][0], z - sm[i - 1][1]; dist += math.hypot(dx, dz)
            head = math.degrees(math.atan2(dx, dz)) % 360
        else: head = 0.0
        y = HAMP * sample(H, size, x / CELL, z / CELL)
        rows.append((dist / 22.0, x, y, z, head, 0, 0, 0, 22.0, 0, dist))
    return rows

def main():
    if not EXE: print('ERROR: RoadbookTests.exe not built'); return 1
    if len(sys.argv) < 2: print('usage: make_terrain_stage.py <track_id> [seed] [follow|drive]'); return 2
    tid = sys.argv[1]; seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    mode = sys.argv[3] if len(sys.argv) > 3 else 'follow'
    rng = random.Random(hash((tid, seed)) & 0x7fffffff)
    H, size, name = load_height(tid)
    scratch = os.path.join(ROOT, 'build'); os.makedirs(scratch, exist_ok=True)
    def run_gen(rows):
        csv = os.path.join(scratch, f'_terrain_{tid}.csv')
        with open(csv, 'w') as f:
            f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
            for r in rows: f.write(','.join(f'{v:.3f}' for v in r) + '\n')
        svg = os.path.join(scratch, f'_terrain_{tid}.svg')
        subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
        return json.load(open(os.path.splitext(svg)[0] + '.json', encoding='utf-8'))
    rb = run_gen(walk_path(route_follow(H, size, rng), H, size) if mode == 'follow' else drive(rng, H, size))
    if mode == 'follow' and sum(1 for b in rb.get('boxes', []) if b['type'] in ('turn', 'hairpin')) < 8:
        rb = run_gen(drive(rng, H, size))         # terrain too flat to navigate by -> add a meander
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
