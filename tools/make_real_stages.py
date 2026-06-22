#!/usr/bin/env python3
"""Build recognizable real-rally-stage roadbooks for the bundled library.

Each stage is a procedurally-synthesized ride trace characteristic of a real event
and its terrain (Empty Quarter dunes, Erzberg hillclimb, Baja desert washes, Finke
whoops, Hellas mountain gravel, Merzouga ergs), run through the *same* C++ generator
the plugin uses, then lightly flavoured with terrain signs. These are faithful
representations for study — not the official copyrighted roadbooks.

Usage:  python tools/make_real_stages.py
"""
import os, math, json, random, subprocess

ROOT = r'D:\BikesRoadbook'
OUT  = os.path.join(ROOT, 'web', 'roadbooks')
EXE  = next((p for p in (os.path.join(ROOT, 'build', c, 'RoadbookTests.exe')
                         for c in ('Release', 'Debug')) if os.path.exists(p)), None)

class Walker:
    """Walks a ground path emitting recorder-format rows (t,x,y,z,yaw,vx,vy,vz,sp,pos,dist)."""
    def __init__(self, speed0=20.0, y0=200.0):
        self.x = self.z = self.head = self.dist = self.t = 0.0
        self.y = y0; self.speed = speed0; self.rows = []
    def _emit(self):
        self.rows.append((self.t, self.x, self.y, self.z,
                          math.degrees(self.head) % 360, 0, 0, 0, self.speed, 0, self.dist))
    def seg(self, length, turn_deg=0.0, speed=None, dy=0.0, step=2.0):
        if speed is None: speed = self.speed
        n = max(1, int(length / step)); dh = math.radians(turn_deg) / n; dyy = dy / n
        for _ in range(n):
            self.head += dh; self.speed = speed
            self.x += step * math.sin(self.head); self.z += step * math.cos(self.head)
            self.y += dyy; self.dist += step; self.t += step / max(2.0, speed); self._emit()
    def dunes(self, length, amp, wl, speed, sharp=False, wander=1.2, step=2.0):
        n = max(1, int(length / step)); base = self.y; s = 0.0
        for _ in range(n):
            s += step; self.head += math.radians(random.uniform(-wander, wander))
            e = math.sin(2 * math.pi * s / wl)
            self.y = base + amp * (e ** 3 if sharp else e)
            self.speed = speed * random.uniform(0.9, 1.05)
            self.x += step * math.sin(self.head); self.z += step * math.cos(self.head)
            self.dist += step; self.t += step / max(2.0, self.speed); self._emit()
    def whoops(self, length, amp, wl, speed, step=1.5):
        n = max(1, int(length / step)); base = self.y; s = 0.0
        for _ in range(n):
            s += step; self.y = base + amp * math.sin(2 * math.pi * s / wl); self.speed = speed
            self.x += step * math.sin(self.head); self.z += step * math.cos(self.head)
            self.dist += step; self.t += step / max(2.0, speed); self._emit()
    def switchbacks(self, count, climb_each, speed, right_first=True):
        for i in range(count):
            self.seg(70, 8, speed, dy=climb_each * 0.4)
            self.seg(42, (170 if (i % 2 == 0) == right_first else -170), speed * 0.45, dy=climb_each * 0.2)
            self.seg(70, -8, speed, dy=climb_each * 0.4)
    def write_csv(self, path):
        with open(path, 'w') as f:
            f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
            for r in self.rows: f.write(','.join(f'{v:.3f}' for v in r) + '\n')

# ---- stage builders (characteristic of each real event) --------------------
def empty_quarter(w):
    w.seg(400, 0, 30); w.dunes(2600, 9, 170, 27)
    w.seg(220, 45, 22); w.dunes(1900, 12, 130, 24, sharp=True)
    w.seg(160, -65, 18); w.dunes(2400, 7, 190, 29)
    w.seg(320, 30, 26); w.dunes(1600, 11, 115, 23, sharp=True)
    w.seg(500, -25, 30); w.seg(260, 0, 28)

def merzouga(w):
    w.seg(300, 0, 24); w.dunes(1500, 13, 120, 21, sharp=True)
    w.seg(180, 70, 14); w.seg(900, 10, 26)            # rocky piste transition
    w.dunes(1700, 9, 150, 22); w.seg(220, -80, 12)
    w.seg(700, 15, 24); w.dunes(1200, 11, 110, 20, sharp=True); w.seg(300, 0, 24)

def erzberg(w):
    w.seg(250, 0, 16, dy=10)
    w.switchbacks(7, climb_each=45, speed=12)          # the iron-mountain hillclimb
    w.seg(120, 30, 10, dy=18); w.seg(90, -150, 7, dy=8)
    w.seg(180, 20, 12, dy=22); w.seg(140, 0, 14, dy=10)

def baja_sf(w):
    w.seg(700, 0, 32)                                  # fast graded road
    w.whoops(500, 1.4, 22, 24); w.seg(120, -55, 18)
    w.seg(900, 8, 33); w.dunes(600, 5, 40, 26, sharp=True)   # silt + small kickers
    w.seg(160, 70, 16); w.whoops(700, 1.7, 18, 22)
    w.seg(1100, -12, 34); w.seg(140, 60, 17); w.seg(900, 0, 31)
    w.dunes(500, 6, 45, 27, sharp=True); w.seg(400, -20, 30)

def finke(w):
    w.seg(500, 0, 33)
    for _ in range(5):
        w.whoops(420, 1.6, 17, 24); w.dunes(120, 5, 42, 26, sharp=True); w.seg(160, random.uniform(-25, 25), 30)
    w.seg(700, 0, 34)

def hellas_crete(w):
    w.seg(300, 0, 18, dy=6)
    w.seg(140, 80, 13, dy=8); w.seg(220, -50, 16, dy=10)
    w.switchbacks(4, climb_each=30, speed=13)
    w.seg(260, 35, 17, dy=-6); w.seg(120, -140, 9, dy=-4)
    w.seg(380, 20, 18, dy=-8); w.whoops(300, 1.1, 24, 15)
    w.seg(160, 95, 12, dy=-6); w.seg(300, -30, 17); w.seg(240, 60, 14, dy=-10)

STAGES = [
    ('rally_empty_quarter', 'Dakar — Empty Quarter (SA)', empty_quarter, 150.0, 1,
     {'dune': 0.40, 'sand': 0.18}),
    ('rally_merzouga',      'Merzouga — Erg Chebbi (MA)',  merzouga,     420.0, 2,
     {'dune': 0.32, 'sand': 0.14, 'rocks': 0.14, 'wadi': 0.08}),
    ('rally_erzberg',       'Erzbergrodeo Prologue (AT)',  erzberg,      720.0, 3,
     {'rocks': 0.30, 'cliff': 0.12}),
    ('rally_baja_sanfelipe','Baja 500 — San Felipe (MX)',  baja_sf,      30.0,  4,
     {'ditch': 0.16, 'wadi': 0.10, 'rocks': 0.06}),
    ('rally_finke',         'Finke Desert Race (AU)',      finke,        560.0, 5,
     {'sand': 0.10}),
    ('rally_hellas_crete',  'Hellas Rally — Crete (GR)',   hellas_crete, 380.0, 6,
     {'rocks': 0.22, 'cliff': 0.08}),
]

def flavour(rb, flav, seed):
    """Add a few terrain signs characteristic of the stage (no spam, deterministic)."""
    rnd = random.Random(seed * 7919)
    for b in rb.get('boxes', []):
        if b.get('type') in ('start', 'finish'):
            continue
        pg = b.setdefault('pictograms', [])
        for sign, prob in flav.items():
            # bias dunes/sand onto the crest/jump boxes, rocks onto turns
            boost = 1.6 if (sign in ('dune', 'sand') and ('bump' in pg or 'jump' in pg)) else \
                    1.5 if (sign in ('rocks', 'cliff') and b.get('type') in ('turn', 'hairpin')) else 1.0
            if sign not in pg and len(pg) < 3 and rnd.random() < prob * boost:
                pg.append(sign)
    return rb

def main():
    if not EXE:
        print('ERROR: RoadbookTests.exe not built'); return 1
    os.makedirs(OUT, exist_ok=True)
    made = []
    for fname, title, build, y0, seed, flav in STAGES:
        random.seed(seed)
        w = Walker(speed0=24.0, y0=y0); build(w)
        csv = os.path.join(ROOT, 'build', f'_{fname}.csv'); w.write_csv(csv)
        svg = os.path.join(ROOT, 'build', f'_{fname}.svg')
        subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
        gen = os.path.splitext(svg)[0] + '.json'
        rb = json.load(open(gen, encoding='utf-8'))
        rb['trackName'] = title
        rb.setdefault('meta', {})['trackName'] = title
        flavour(rb, flav, seed)
        out = os.path.join(OUT, f'roadbook_{fname}.json')
        json.dump(rb, open(out, 'w', encoding='utf-8'), separators=(',', ':'))
        km = (rb['boxes'][-1]['distTotalKm'] if rb.get('boxes') else 0)
        made.append((fname, title, len(rb.get('boxes', [])), round(km, 1)))
    print('built real stages:')
    for f, t, n, km in made: print(f'  {f:24} {t:30} {n:3} boxes  {km:5} km')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
