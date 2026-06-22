#!/usr/bin/env python3
"""Generate procedural practice-roadbook scenarios.

Synthesises ride traces (junction drills, hairpins, crest/dip, slalom, maze) in the
recorder CSV format, runs them through the real RoadbookTests generator, and writes
training roadbooks to web/roadbooks and a scenarios bundle.
"""
import os, math, subprocess, json, sys

ROOT = r'D:\BikesRoadbook'
EXE  = os.path.join(ROOT, r'build\Debug\RoadbookTests.exe')
OUT  = os.path.join(ROOT, r'content\scenarios')
WEB  = os.path.join(ROOT, r'web\roadbooks')
STEP = 2.0  # metres between samples

# A scenario is a list of moves:
#   ('S', length_m, elev_delta_m, speed)         straight (elev rises/falls linearly)
#   ('C', length_m, crest_height_m, speed)        straight with a symmetric crest (+) or dip (-)
#   ('T', angle_deg, arc_len_m, speed)            turn (heading sweeps by angle over arc)
SCENARIOS = {
    'drill_junctions': [   # navigation: spaced turns of varied angle
        ('S', 120, 0, 16), ('T', 90, 18, 9), ('S', 90, 0, 16), ('T', -75, 16, 9),
        ('S', 110, 0, 16), ('T', 110, 20, 8), ('S', 80, 0, 16), ('T', -60, 14, 10),
        ('S', 100, 0, 16), ('T', 95, 18, 9), ('S', 90, 0, 16)],
    'drill_hairpins': [    # five switchbacks
        ('S', 70, 6, 14)] + sum(([('T', 170 if i % 2 == 0 else -170, 14, 5), ('S', 55, -3 if i % 2 else 4, 13)]
                                 for i in range(5)), []),
    'drill_crest_dip': [   # elevation reading: crests (bump) and slow dips (water)
        ('S', 60, 0, 16), ('C', 44, 4.5, 16), ('S', 50, 0, 16), ('C', 40, -4.0, 6),
        ('S', 55, 0, 16), ('C', 46, 5.0, 16), ('S', 50, 0, 16), ('C', 38, -3.5, 6), ('S', 60, 0, 16)],
    'drill_slalom': [      # quick alternating chicane
        ('S', 60, 0, 15)] + sum(([('T', 48 if i % 2 == 0 else -48, 10, 11), ('S', 34, 0, 14)]
                                 for i in range(8)), []),
    'drill_mixed': [       # mixed bag: sweeper, hairpin, crest, junction
        ('S', 90, 0, 16), ('T', 55, 22, 11), ('S', 70, 3, 15), ('C', 42, 4.0, 16),
        ('T', -160, 13, 5), ('S', 80, -3, 14), ('T', 80, 16, 9), ('S', 60, 0, 16),
        ('C', 40, -3.8, 6), ('S', 70, 0, 16), ('T', -90, 18, 9), ('S', 80, 0, 16)],
    'drill_esses': [       # flowing S-bends: rhythm + reading ahead
        ('S', 80, 0, 15)] + sum(([('T', 60 if i % 2 == 0 else -60, 30, 12), ('S', 40, 0, 14)]
                                 for i in range(7)), []),
    'drill_technical': [   # tight + elevation: hairpins, crests and dips together
        ('S', 70, 4, 13), ('T', 165, 13, 5), ('C', 40, 4.5, 14), ('S', 50, -3, 13),
        ('T', -150, 14, 5), ('S', 60, 0, 15), ('C', 40, -4.0, 6), ('T', 120, 16, 8),
        ('S', 55, 5, 14), ('T', -100, 18, 9), ('C', 44, 5.0, 15), ('S', 70, -4, 13)],
    'drill_long_mixed': [  # a longer varied stage to sustain concentration
        ('S', 200, 0, 17), ('T', 45, 60, 13), ('S', 150, 6, 16), ('C', 50, 5, 16),
        ('T', -70, 40, 10), ('S', 180, 0, 17), ('T', 160, 14, 5), ('S', 120, -5, 14),
        ('C', 44, -4, 6), ('T', 90, 50, 11), ('S', 200, 0, 17), ('T', -55, 45, 12),
        ('S', 160, 4, 16), ('C', 46, 5, 16), ('T', 110, 16, 8), ('S', 140, 0, 16)],
}

def build_csv(moves, path):
    x = z = 0.0; y = 100.0; head = 0.0; dist = 0.0; t = 0.0
    rows = ['# scenario synthetic ride']
    def emit(speed):
        nonlocal t
        rows.append(f'{t:.2f},{x:.3f},{y:.3f},{z:.3f},{math.degrees(head) % 360:.1f},0,0,{speed:.2f},0,{dist:.2f}')
        t += STEP / max(speed, 1.0)
    emit(moves[0][3] if moves else 8)
    for mv in moves:
        kind = mv[0]
        if kind in ('S', 'C'):
            length, param, speed = mv[1], mv[2], mv[3]
            n = max(1, int(length / STEP))
            for i in range(n):
                if kind == 'C':   # symmetric bump/dip via a sine hump
                    y_off = param * math.sin(math.pi * (i + 1) / n)
                    base = y; y = 100.0 + y_off  # crest relative to baseline 100
                else:
                    y += param * STEP / length if length else 0
                x += STEP * math.sin(head); z += STEP * math.cos(head); dist += STEP
                emit(speed)
            if kind == 'C': y = 100.0
        elif kind == 'T':
            angle, arc, speed = math.radians(mv[1]), mv[2], mv[3]
            n = max(1, int(arc / STEP)); dh = angle / n
            for _ in range(n):
                head += dh; x += STEP * math.sin(head); z += STEP * math.cos(head); dist += STEP
                emit(speed)
    open(path, 'w').write('\n'.join(rows) + '\n')

def main():
    os.makedirs(OUT, exist_ok=True); os.makedirs(WEB, exist_ok=True)
    made = []
    for name, moves in SCENARIOS.items():
        csv = os.path.join(OUT, name + '.csv'); svg = os.path.join(OUT, name + '.svg')
        build_csv(moves, csv)
        subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
        gen = os.path.splitext(svg)[0] + '.json'
        rb = json.load(open(gen, encoding='utf-8'))
        rb['trackName'] = 'Practice: ' + name.replace('drill_', '').replace('_', ' ')
        out = os.path.join(OUT, 'roadbook_' + name + '.json')
        json.dump(rb, open(out, 'w', encoding='utf-8'), indent=2)
        json.dump(rb, open(os.path.join(WEB, 'roadbook_' + name + '.json'), 'w', encoding='utf-8'))
        bx = rb.get('boxes', [])
        signs = sorted({s for b in bx for s in b.get('pictograms', [])})
        made.append(f'{name:18s} {len(bx):2d} boxes  signs: {",".join(signs) or "-"}')
    print('\n'.join(made)); print(f'\n{len(made)} scenarios -> {OUT}')

if __name__ == '__main__':
    main()
