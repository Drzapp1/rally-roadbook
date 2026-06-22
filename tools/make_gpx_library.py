#!/usr/bin/env python3
"""Build a library of sample rally-stage roadbooks from synthesised GPX tracks.

These are ORIGINAL practice stages (not real-event data) generated as GPX
lat/lon/ele tracks, then run through the real GPX import pipeline (parse GPX ->
metric ride CSV -> RoadbookTests generator) so the import path is genuinely
exercised. Drop your own real .gpx in and `rbtool gpx file.gpx` does the same.
"""
import os, math, subprocess, json, xml.etree.ElementTree as ET

ROOT = r'D:\BikesRoadbook'
EXE  = os.path.join(ROOT, r'build\Debug\RoadbookTests.exe')
OUT  = os.path.join(ROOT, r'content\gpx_library')
WEB  = os.path.join(ROOT, r'web\roadbooks')
STEP = 4.0  # metres between GPX points (rally stages are coarse)

# move list: ('S', len, delev) straight | ('C', len, height) crest(+)/dip(-) | ('T', deg, arc) turn
STAGES = {
    'stage_desert_loop': dict(base=(31.05, 2.40), moves=[   # sweeping desert pistes
        ('S', 400, 0), ('T', 60, 80, 0), ('S', 300, 6), ('C', 60, 5, 0), ('T', -80, 90, 0),
        ('S', 500, 0), ('C', 70, -4, 0), ('T', 110, 70, 0), ('S', 350, -6), ('T', -50, 90, 0),
        ('S', 600, 0), ('C', 60, 6, 0), ('T', 95, 80, 0), ('S', 300, 0)]),
    'stage_mountain_pass': dict(base=(45.20, 6.80), moves=[ # climbing switchbacks
        ('S', 250, 12), ('T', 150, 40, 0), ('S', 180, 14), ('T', -160, 38, 0), ('S', 200, 13),
        ('T', 150, 40, 0), ('S', 220, 16), ('T', -150, 42, 0), ('S', 260, 12), ('T', 70, 70, 0),
        ('S', 300, 10), ('T', -60, 80, 0), ('S', 240, 8)]),
    'stage_coastal_sprint': dict(base=(36.50, -4.60), moves=[ # fast, gentle curves
        ('S', 700, 0), ('T', 35, 120, 0), ('S', 600, 4), ('T', -30, 140, 0), ('S', 800, 0),
        ('T', 40, 110, 0), ('S', 500, -4), ('T', -25, 150, 0), ('S', 650, 0)]),
    'stage_canyon_run': dict(base=(34.10, -6.80), moves=[   # twisty + water crossings
        ('S', 300, 0), ('T', 80, 50, 0), ('C', 50, -4, 0), ('T', -90, 45, 0), ('S', 250, 0),
        ('T', 120, 40, 0), ('S', 200, 6), ('C', 50, -5, 0), ('T', -70, 55, 0), ('S', 300, 0),
        ('T', 100, 45, 0), ('C', 60, 5, 0), ('T', -110, 42, 0), ('S', 280, 0)]),
    'stage_river_delta': dict(base=(16.30, -16.00), moves=[ # flat, many water crossings + forks
        ('S', 400, 0), ('C', 50, -5, 0), ('T', 40, 90, 0), ('S', 300, 0), ('C', 55, -5, 0),
        ('T', -50, 100, 0), ('S', 350, 0), ('C', 50, -4, 0), ('T', 60, 80, 0), ('S', 400, 0), ('C', 50, -5, 0)]),
    'stage_high_plateau': dict(base=(33.80, 7.20), moves=[  # long fast straights, big crests
        ('S', 900, 8), ('C', 70, 6, 0), ('T', 30, 160, 0), ('S', 800, -6), ('C', 70, 7, 0),
        ('T', -25, 180, 0), ('S', 1000, 0), ('C', 60, 6, 0), ('T', 35, 150, 0), ('S', 700, 0)]),
    'stage_forest_track': dict(base=(52.10, -3.80), moves=[ # tight wooded twists, undulating
        ('S', 150, 4), ('T', 70, 35, 0), ('S', 90, -4, 0), ('T', -80, 32, 0), ('C', 40, 3, 0),
        ('S', 120, 5), ('T', 100, 30, 0), ('S', 80, -3, 0), ('T', -60, 38, 0), ('C', 40, -3, 0),
        ('S', 130, 0), ('T', 85, 34, 0), ('S', 100, 0)]),
    'stage_big_dunes': dict(base=(23.40, 12.10), moves=[    # erg crossing, large crests/drops
        ('S', 250, 0), ('C', 80, 8, 0), ('S', 200, 0), ('C', 80, -7, 0), ('T', 50, 90, 0),
        ('S', 220, 0), ('C', 90, 9, 0), ('S', 180, 0), ('C', 80, -8, 0), ('T', -40, 100, 0),
        ('S', 260, 0), ('C', 80, 7, 0), ('S', 200, 0)]),
}

def build_track(moves):
    x = z = 0.0; y = 200.0; head = 0.0; pts = []
    def emit(): pts.append((x, z, y))
    emit()
    for mv in moves:
        kind = mv[0]
        if kind in ('S', 'C'):
            length, param = mv[1], mv[2]; n = max(1, int(length / STEP))
            for i in range(n):
                if kind == 'C': y = 200.0 + param * math.sin(math.pi * (i + 1) / n)
                else: y += param * STEP / length if length else 0
                x += STEP * math.sin(head); z += STEP * math.cos(head); emit()
            if kind == 'C': y = 200.0
        else:
            angle, arc = math.radians(mv[1]), mv[2]; n = max(1, int(arc / STEP)); dh = angle / n
            for _ in range(n):
                head += dh; x += STEP * math.sin(head); z += STEP * math.cos(head); emit()
    return pts

def to_gpx(track, base, path, name):
    lat0, lon0 = base; mlat = 111320.0; mlon = 111320.0 * math.cos(math.radians(lat0))
    seg = ''.join(f'<trkpt lat="{lat0 + z / mlat:.7f}" lon="{lon0 + x / mlon:.7f}"><ele>{y:.1f}</ele></trkpt>\n'
                  for x, z, y in track)
    open(path, 'w').write(
        f'<?xml version="1.0" encoding="UTF-8"?>\n<gpx version="1.1" creator="RallyRoadbook">\n'
        f'<trk><name>{name}</name><trkseg>\n{seg}</trkseg></trk>\n</gpx>\n')

def gpx_to_csv(gpxpath, csvpath):
    root = ET.parse(gpxpath).getroot(); pts = []
    for el in root.iter():
        if el.tag.split('}')[-1] == 'trkpt':
            e = 0.0
            for c in el:
                if c.tag.split('}')[-1] == 'ele' and c.text:
                    try: e = float(c.text)
                    except ValueError: pass
            pts.append((float(el.get('lat')), float(el.get('lon')), e))
    lat0 = sum(p[0] for p in pts) / len(pts); lon0 = sum(p[1] for p in pts) / len(pts)
    mlat = 111320.0; mlon = 111320.0 * math.cos(math.radians(lat0))
    with open(csvpath, 'w') as f:
        f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
        t = dist = 0.0; px = pz = None
        for lat, lon, e in pts:
            x = (lon - lon0) * mlon; z = (lat - lat0) * mlat
            if px is not None: dist += math.hypot(x - px, z - pz)
            f.write(f'{t:.3f},{x:.3f},{e:.3f},{z:.3f},0,0,0,0,11,0,{dist:.3f}\n')
            px, pz = x, z; t += 0.4

def main():
    os.makedirs(OUT, exist_ok=True); os.makedirs(WEB, exist_ok=True)
    index = []
    for name, spec in STAGES.items():
        track = build_track(spec['moves'])
        gpx = os.path.join(OUT, name + '.gpx'); csv = os.path.join(OUT, name + '.csv'); svg = os.path.join(OUT, name + '.svg')
        to_gpx(track, spec['base'], gpx, name)
        gpx_to_csv(gpx, csv)
        subprocess.run([EXE, csv, svg], check=True, stdout=subprocess.DEVNULL)
        rb = json.load(open(os.path.splitext(svg)[0] + '.json', encoding='utf-8'))
        rb['trackName'] = name.replace('stage_', '').replace('_', ' ').title()
        out = os.path.join(OUT, 'roadbook_' + name + '.json')
        json.dump(rb, open(out, 'w', encoding='utf-8'), indent=1)
        json.dump(rb, open(os.path.join(WEB, 'roadbook_' + name + '.json'), 'w', encoding='utf-8'))
        bx = rb.get('boxes', []); km = bx[-1]['distTotalKm'] if bx else 0
        signs = sorted({s for b in bx for s in b.get('pictograms', [])})
        index.append({'name': rb['trackName'], 'file': 'roadbook_' + name + '.json', 'km': round(km, 2), 'boxes': len(bx), 'signs': signs})
        print(f'  {rb["trackName"]:18s} {km:5.2f} km  {len(bx):3d} boxes  signs: {",".join(signs) or "-"}')
    json.dump(index, open(os.path.join(OUT, 'index.json'), 'w', encoding='utf-8'), indent=1)
    print(f'\n{len(index)} GPX stages -> {OUT}')

if __name__ == '__main__':
    main()
