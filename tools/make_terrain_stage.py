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
import os, sys, math, json, array, random, subprocess, heapq, bisect

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

def enrich_signs(rb):
    """Read the route's real elevation profile and tag boxes with the terrain signs
    the relief implies — crest / jump on a launch, dip / compression in a bowl, steep
    descent — so 'elevation -> crest/dip/jump signs' is delivered from the landscape."""
    route = rb.get('route', [])
    if len(route) < 12: return rb
    ds = [p[2] for p in route]; es = [p[3] for p in route]
    def elev_at(d):
        i = bisect.bisect_left(ds, d)
        if i <= 0: return es[0]
        if i >= len(ds): return es[-1]
        d0, d1 = ds[i - 1], ds[i]; t = (d - d0) / ((d1 - d0) or 1.0)
        return es[i - 1] * (1 - t) + es[i] * t
    boxes = rb.get('boxes', []); D = 70.0
    be = [elev_at(b['distTotalKm'] * 1000.0) for b in boxes]   # elevation at each box
    for i, b in enumerate(boxes):
        if b.get('type') in ('start', 'finish'): continue
        d = b['distTotalKm'] * 1000.0; e = be[i]
        e1 = elev_at(d + D); post = (e1 - e) / D                # gradient out of the box
        ep = be[i - 1] if i > 0 else elev_at(d - D)             # neighbour-box elevations
        en = be[i + 1] if i < len(boxes) - 1 else e1
        pg = b.setdefault('pictograms', [])
        def add(s):
            if s not in pg and len(pg) < 3: pg.append(s)
        if e > ep + 2.4 and e > en + 2.4:                       # higher than both neighbours -> crest
            add('jump' if post < -0.12 else ('crestR' if b.get('turnDir') == 'right' else 'crestL'))
        elif e < ep - 2.4 and e < en - 2.4:                     # lower than both -> dip / compression
            add('compression' if post > 0.12 else 'dip')
        elif post < -0.07: add('downhill')                      # descent
        elif post > 0.10: add('hill')                           # climb
    return rb

def surface_signs(rb, aerial_path, size):
    """Classify the surface from the track's aerial imagery and tag transitions —
    sandy -> dune, vegetation -> bush, rock/dark -> rocky, blue -> water — so the
    roadbook's terrain signs come from the imagery, not just the heightmap."""
    try:
        from PIL import Image
        im = Image.open(aerial_path).convert('RGB')
    except Exception:
        return rb
    W, Hh = im.size; px = im.load(); span = size * CELL
    route = rb.get('route', [])
    if len(route) < 5: return rb
    ds = [p[2] for p in route]
    def pos_at(dm):
        i = max(1, min(len(route) - 1, bisect.bisect_left(ds, dm))); return route[i][0], route[i][1]
    def classify(x, z):
        u = min(W - 1, max(0, int(x / span * W))); v = min(Hh - 1, max(0, int(z / span * Hh)))
        rs = gs = bs = n = 0
        for du in (-2, 0, 2):
            for dv in (-2, 0, 2):
                c = px[min(W - 1, max(0, u + du)), min(Hh - 1, max(0, v + dv))]; rs += c[0]; gs += c[1]; bs += c[2]; n += 1
        r, g, b = rs // n, gs // n, bs // n; mx = max(r, g, b); mn = min(r, g, b)
        if b > r + 15 and b > g + 15 and b > 100: return 'water'
        if g > r + 10 and g > b + 10: return 'bush'
        if r > 150 and g > 115 and (r - b) > 35: return 'dune'
        if mx < 115 or (mx > 200 and mx - mn < 28): return 'rocky'
        return None
    prev = None
    for b in rb.get('boxes', []):
        if b.get('type') in ('start', 'finish'): continue
        x, z = pos_at(b['distTotalKm'] * 1000.0); s = classify(x, z)
        if s and s != prev:                                  # mark only surface transitions
            pg = b.setdefault('pictograms', [])
            if s not in pg and len(pg) < 3: pg.append(s)
        if s: prev = s
    return rb

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
    enrich_signs(rb)                              # tag crest/dip/jump/descent from the real elevation
    surface_signs(rb, os.path.join('web', 'tracks', tid, 'aerial.jpg'), size)   # surface from the aerial imagery
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
