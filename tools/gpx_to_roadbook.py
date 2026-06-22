#!/usr/bin/env python3
# Convert a real-world GPX track into a roadbook JSON (so a real rally route can
# be studied in-game). GPX lat/lon/ele are projected to a local metric plane and
# fed through the same C++ generator (RoadbookTests.exe), reusing all the tulip /
# waypoint / sign logic. Output is copied into the in-game import folder.
import sys, os, math, subprocess, shutil, xml.etree.ElementTree as ET

EXE    = r'D:\BikesRoadbook\build\Debug\RoadbookTests.exe'
IMPORT = r'C:\Users\rosta\Documents\PiBoSo\MX Bikes\Roadbook\import'

def localname(tag): return tag.split('}')[-1]

def parse_gpx(path):
    root = ET.parse(path).getroot(); pts = []
    for el in root.iter():
        if localname(el.tag) == 'trkpt':
            lat = float(el.get('lat')); lon = float(el.get('lon')); e = 0.0
            for c in el:
                if localname(c.tag) == 'ele' and c.text:
                    try: e = float(c.text)
                    except ValueError: pass
            pts.append((lat, lon, e))
    return pts

def write_csv(pts, csvpath):
    lat0 = sum(p[0] for p in pts) / len(pts)
    lon0 = sum(p[1] for p in pts) / len(pts)
    mlat = 111320.0; mlon = 111320.0 * math.cos(math.radians(lat0))
    with open(csvpath, 'w') as f:
        f.write('t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n')
        t = 0.0; dist = 0.0; px = pz = None
        for lat, lon, e in pts:
            x = (lon - lon0) * mlon; z = (lat - lat0) * mlat
            if px is not None: dist += math.hypot(x - px, z - pz)
            f.write(f'{t:.3f},{x:.3f},{e:.3f},{z:.3f},0,0,0,0,10,0,{dist:.3f}\n')
            px, pz = x, z; t += 0.1

def main():
    if len(sys.argv) < 2:
        print('usage: gpx_to_roadbook.py <file.gpx> [name]'); return 1
    gpx = sys.argv[1]
    name = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(os.path.basename(gpx))[0]
    pts = parse_gpx(gpx)
    if len(pts) < 10:
        print('too few GPX track points:', len(pts)); return 1
    base = os.path.dirname(os.path.abspath(gpx))
    csv = os.path.join(base, name + '_gpx.csv'); svg = os.path.join(base, name + '_gpx.svg')
    write_csv(pts, csv)
    subprocess.run([EXE, csv, svg], check=True)
    jsrc = svg[:-4] + '.json'
    if not os.path.exists(jsrc): jsrc = os.path.splitext(svg)[0] + '.json'
    if not os.path.exists(jsrc):
        print('generator did not produce JSON next to', svg); return 1
    if os.path.isdir(IMPORT):
        dst = os.path.join(IMPORT, f'roadbook_{name}.json'); shutil.copy(jsrc, dst)
        print('imported ->', dst, '(load via F4 -> Roadbooks -> import)')
    print('GPX points:', len(pts), '   roadbook JSON:', jsrc)
    return 0

if __name__ == '__main__':
    sys.exit(main())
