#!/usr/bin/env python3
"""rbtool - rally roadbook CLI.

  rbtool gen <ride.csv> [out.json]        generate a roadbook from a recorded ride
  rbtool gpx <file.gpx> [name]            import a real-world GPX track as a roadbook
  rbtool validate <roadbook.json>         lint a roadbook (schema + sanity)
  rbtool stats <roadbook.json>            print roadbook statistics
  rbtool sheet <roadbook.json> [out.svg]  render a printable SVG roadbook sheet
  rbtool book <roadbook.json> [out.html]  render an A4-paginated printable book (print to PDF)
  rbtool batch <rides_dir> <out_dir>      generate roadbooks for every ride in a folder
  rbtool diff <a.json> <b.json>           compare two roadbooks (boxes, signs, edits)
  rbtool union <primary.csv> <other.csv>... [out.json]   multi-ride union (un-taken branches)
"""
import sys, os, json, subprocess, base64, math, glob

ROOT = r'D:\BikesRoadbook'
EXE  = os.path.join(ROOT, r'build\Debug\RoadbookTests.exe')
SIGNS = os.path.join(ROOT, r'tools\iconpack\png_tulip')

# stock tulip catalog (x:0..1 L->R, y:0..1 where 1=top; entry first) — clean shapes
# mapped per turn class instead of drawing the raw recorded path.
TULIP_STOCK = {
 'start': [(.5,.10),(.5,.84)], 'straight': [(.5,.08),(.5,.92)],
 'slightL': [(.5,.08),(.5,.52),(.30,.88)], 'slightR': [(.5,.08),(.5,.52),(.70,.88)],
 'left': [(.5,.08),(.5,.56),(.14,.56)], 'right': [(.5,.08),(.5,.56),(.86,.56)],
 'sharpL': [(.5,.08),(.5,.60),(.26,.36)], 'sharpR': [(.5,.08),(.5,.60),(.74,.36)],
 'hairpinL': [(.5,.08),(.5,.70),(.30,.80),(.30,.34)], 'hairpinR': [(.5,.08),(.5,.70),(.70,.80),(.70,.34)],
 'finish': [(.5,.08),(.5,.80)]}
def _tulip_bend(pts):
    if not pts or len(pts) < 2: return 90.0
    a, b, c, d = pts[0], pts[1], pts[-2], pts[-1]
    v1 = (b[0]-a[0], b[1]-a[1]); v2 = (d[0]-c[0], d[1]-c[1])
    m1 = math.hypot(*v1) or 1; m2 = math.hypot(*v2) or 1
    return math.degrees(math.acos(max(-1, min(1, (v1[0]*v2[0]+v1[1]*v2[1])/(m1*m2)))))
def tulip_code(b):
    t = b.get('type'); R = (b.get('turnDir') == 'right')
    if t == 'start': return 'start'
    if t == 'finish': return 'finish'
    if t == 'hairpin': return 'hairpinR' if R else 'hairpinL'
    if t == 'turn':
        m = _tulip_bend(b.get('tulip', {}).get('points', []))
        if m < 32: return 'slightR' if R else 'slightL'
        if m >= 100: return 'sharpR' if R else 'sharpL'
        return 'right' if R else 'left'
    return 'straight'

def load(p): return json.load(open(p, encoding='utf-8'))
def comma(v): return f'{v:.2f}'.replace('.', ',')
def total_km(rb):
    bx = rb.get('boxes', [])
    return rb.get('totalDistanceKm') or (bx[-1].get('distTotalKm', 0) if bx else 0)
def track_name(rb, fallback=''):
    return rb.get('trackName') or rb.get('trackId') or fallback or 'roadbook'

def cmd_gen(args):
    if not args: return err('gen needs <ride.csv>')
    out = args[1] if len(args) > 1 else os.path.splitext(args[0])[0] + '.json'
    svg = os.path.splitext(out)[0] + '_gen.svg'
    subprocess.run([EXE, args[0], svg], check=True)
    j = os.path.splitext(svg)[0] + '.json'
    if os.path.exists(j) and os.path.abspath(j) != os.path.abspath(out):
        os.replace(j, out)
    print('wrote', out); return 0

def cmd_gpx(args):
    if not args: return err('gpx needs <file.gpx>')
    return subprocess.call([sys.executable, os.path.join(ROOT, r'tools\gpx_to_roadbook.py')] + args)

def cmd_validate(args):
    if not args: return err('validate needs <roadbook.json>')
    rb = load(args[0]); issues = []; warns = []
    boxes = rb.get('boxes', [])
    if not boxes: issues.append('no boxes')
    prev = -1.0; prevType = None
    for i, b in enumerate(boxes):
        for k in ('distTotalKm', 'capDeg', 'type', 'tulip'):
            if k not in b: issues.append(f'box {i}: missing {k}')
        d = b.get('distTotalKm', 0); t = b.get('type')
        if d != d: issues.append(f'box {i}: distTotalKm is NaN')                       # NaN check
        if d < prev - 1e-6: issues.append(f'box {i}: distance goes backwards ({d} < {prev})')
        edge = t in ('start', 'finish') or prevType in ('start', 'finish')
        if not edge and prev >= 0 and (d - prev) * 1000 < 5: issues.append(f'box {i}: coincident with previous (<5m)')
        prev = d; prevType = t
        c = b.get('capDeg', 0)
        if not (0 <= c < 360): issues.append(f'box {i}: cap out of range ({c})')
        pts = b.get('tulip', {}).get('points', [])
        if len(pts) < 2: issues.append(f'box {i}: tulip < 2 points')
        if any(len(p) < 2 or not (0 <= p[0] <= 1 and 0 <= p[1] <= 1) for p in pts): issues.append(f'box {i}: tulip point out of [0,1]')
    km = boxes[-1].get('distTotalKm', 0) if boxes else 0
    if km > 0 and len(boxes) / km > 25: warns.append(f'high box density: {len(boxes)/km:.1f}/km')
    print(f'{os.path.basename(args[0])}: {len(boxes)} boxes, {len(issues)} issue(s), {len(warns)} warning(s)')
    for s in issues[:40]: print('  -', s)
    for s in warns: print('  ~', s)
    return 1 if issues else 0

def cmd_stats(args):
    if not args: return err('stats needs <roadbook.json>')
    rb = load(args[0]); bx = rb.get('boxes', [])
    from collections import Counter
    types = Counter(b.get('type') for b in bx)
    signs = Counter(s for b in bx for s in b.get('pictograms', []))
    dang = Counter(b.get('dangerLevel', 0) for b in bx)
    print(f'track     : {track_name(rb, os.path.splitext(os.path.basename(args[0]))[0])}')
    print(f'distance  : {total_km(rb):.2f} km   boxes: {len(bx)}')
    TH = {'jump': 1.0, 'water': 1.0, 'cliff': 1.0, 'dune': 0.6, 'dunes': 0.6, 'rocks': 0.5, 'sand': 0.3,
          'ditch': 0.5, 'hole': 0.6, 'wadi': 0.4, 'compression': 0.4, 'bumpy': 0.2, 'narrow': 0.4, 'collapse': 0.6}
    hz = sum(x.get('dangerLevel', 0) + (2 if x.get('type') == 'hairpin' else 0)
             + sum(TH.get(p, 0) for p in x.get('pictograms', [])) for x in bx)
    km0 = total_km(rb)
    if km0 > 0.1:
        sc = hz / km0; lab = 'Easy' if sc < 2 else 'Moderate' if sc < 5 else 'Hard' if sc < 9 else 'Expert'
        print(f'difficulty: {lab}  ({sc:.1f} hazard/km)')
    print(f'types     : ' + ', '.join(f'{k}={v}' for k, v in types.most_common()))
    print(f'danger    : ' + ', '.join(f'L{k}={v}' for k, v in sorted(dang.items()) if k))
    left = sum(1 for b in bx if b.get('turnDir') == 'left'); right = sum(1 for b in bx if b.get('turnDir') == 'right')
    print(f'turns     : {left} left / {right} right' + (f'   ({100*left//(left+right)}% L)' if left + right else ''))
    gaps = [(bx[i]['distTotalKm'] - bx[i-1]['distTotalKm']) * 1000 for i in range(1, len(bx))]
    if gaps:
        km = total_km(rb)
        print(f'spacing   : avg {sum(gaps)/len(gaps):.0f} m   min {min(gaps):.0f} m   density {len(bx)/km:.1f}/km' if km else '')
    print(f'signs     : ' + (', '.join(f'{k}={v}' for k, v in signs.most_common()) or '(none)'))
    rt = rb.get('route', [])
    if rt and len(rt[0]) >= 4:
        es = [r[3] for r in rt]; asc = sum(max(0, rt[i][3]-rt[i-1][3]) for i in range(1, len(rt)))
        print(f'elevation : {min(es):.0f}..{max(es):.0f} m   ascent ~{asc:.0f} m')
    return 0

def _b64(code):
    p = os.path.join(SIGNS, code + '.png')
    if not os.path.exists(p): return None
    return 'data:image/png;base64,' + base64.b64encode(open(p, 'rb').read()).decode()

FRENCH = False
def _abbr(b):
    t = b.get('type')
    if FRENCH:
        return 'DEP' if t == 'start' else 'ARR' if t == 'finish' else 'TD' if t == 'straight' else ('D' if b.get('turnDir') == 'right' else 'G')
    return 'S' if t == 'start' else 'F' if t == 'finish' else 'kpS' if t == 'straight' else ('R' if b.get('turnDir') == 'right' else 'L')

def _case_svg(b, idx, x, y, w, h):
    INK, PAPER, LINE, CYAN, PINK, DIM = '#18120f', '#f3f1ea', '#968c88', '#28b9eb', '#cd2620', '#78726a'
    lw, cw = w*0.30, w*0.42; z1, z2 = x+lw, x+lw+cw
    s = [f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{PAPER}" stroke="{LINE}"/>']
    s.append(f'<line x1="{z1}" y1="{y}" x2="{z1}" y2="{y+h}" stroke="{LINE}"/><line x1="{z2}" y1="{y}" x2="{z2}" y2="{y+h}" stroke="{LINE}"/>')
    s.append(f'<text x="{x+6}" y="{y+26}" font-family="monospace" font-weight="bold" font-size="22" fill="{INK}">{comma(b["distTotalKm"])}</text>')
    part = '0,00' if b.get('type') == 'start' else ('END' if b.get('type') == 'finish' else comma(b.get('distPartialM', 0)/1000))
    s.append(f'<rect x="{x+5}" y="{y+h-30}" width="{lw*0.62:.0f}" height="22" fill="none" stroke="{LINE}"/><text x="{x+9}" y="{y+h-14}" font-family="monospace" font-size="12" fill="{INK}">{part}</text>')
    s.append(f'<text x="{z1-16}" y="{y+h-12}" font-family="monospace" font-size="11" fill="{INK}" text-anchor="middle">{idx}</text>')
    code = tulip_code(b)
    pts = TULIP_STOCK.get(code) or b.get('tulip', {}).get('points', [])
    if len(pts) >= 2:
        m = 10; uw, uh = cw-2*m, h-2*m
        P = [(z1+m+p[0]*uw, y+m+(1-p[1])*uh) for p in pts]
        ps = ' '.join(f'{a:.1f},{c:.1f}' for a, c in P)
        s.append(f'<polyline points="{ps}" fill="none" stroke="{CYAN}" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>')
        s.append(f'<polyline points="{ps}" fill="none" stroke="{INK}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>')
        s.append(f'<circle cx="{P[0][0]:.1f}" cy="{P[0][1]:.1f}" r="3.5" fill="{INK}"/>')
        ax, ay = P[-1]
        if code == 'finish':
            for k in range(-2, 3):
                s.append(f'<rect x="{ax+k*7-3:.1f}" y="{ay-7:.1f}" width="7" height="9" fill="{INK if k % 2 else "#9a9a9a"}"/>')
        else:
            bx, by = P[-2]; dx, dy = ax-bx, ay-by; L = math.hypot(dx, dy) or 1; ux, uy = dx/L, dy/L; px, py = -uy, ux; sz = 10
            s.append(f'<polygon points="{ax+ux*sz:.1f},{ay+uy*sz:.1f} {ax+px*sz*0.6:.1f},{ay+py*sz*0.6:.1f} {ax-px*sz*0.6:.1f},{ay-py*sz*0.6:.1f}" fill="{INK}"/>')
    for bd in b.get('branchDeg', []):   # un-taken junction branches (faint dashed spokes)
        r = math.radians(bd); cx = z1 + cw / 2; cy = y + h * 0.5
        ex = cx + math.sin(r) * cw * 0.42; ey = cy - math.cos(r) * cw * 0.42
        s.append(f'<line x1="{cx:.1f}" y1="{cy:.1f}" x2="{ex:.1f}" y2="{ey:.1f}" stroke="#9a9088" stroke-width="2.5" stroke-dasharray="4,3"/>')
    sg = b.get('pictograms', [])[:4]
    for i, code in enumerate(sg):
        d = _b64(code)
        if d: s.append(f'<image href="{d}" x="{z1+6+i*30}" y="{y+h-30}" width="26" height="26"/>')
    rcx = z2+(x+w-z2)/2
    s.append(f'<text x="{rcx}" y="{y+30}" font-family="monospace" font-weight="bold" font-size="22" fill="{INK}" text-anchor="middle">{_abbr(b)}</text>')
    s.append(f'<text x="{rcx}" y="{y+46}" font-family="monospace" font-size="11" fill="{DIM}" text-anchor="middle">{b.get("capDeg",0):03d}</text>')
    if b.get('dangerLevel', 0): s.append(f'<text x="{rcx}" y="{y+h-10}" font-family="monospace" font-weight="bold" font-size="18" fill="{PINK}" text-anchor="middle">{"!"*b["dangerLevel"]}</text>')
    return ''.join(s)

def cmd_sheet(args):
    global FRENCH
    if '--french' in args: FRENCH = True; args = [a for a in args if a != '--french']
    if not args: return err('sheet needs <roadbook.json>')
    rb = load(args[0]); out = args[1] if len(args) > 1 else os.path.splitext(args[0])[0] + '.svg'
    bx = rb.get('boxes', []); cols = 3; cw, ch, gap = 250, 150, 8; pad = 20
    rows = (len(bx)+cols-1)//cols
    W = pad*2 + cols*cw + (cols-1)*gap; H = pad+40 + rows*(ch+gap)
    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
         f'<rect width="{W}" height="{H}" fill="#fff"/>',
         f'<text x="{pad}" y="26" font-family="sans-serif" font-size="18" font-weight="bold">ROADBOOK  {track_name(rb, os.path.splitext(os.path.basename(args[0]))[0])}  -  {total_km(rb):.2f} km, {len(bx)} boxes</text>']
    for i, b in enumerate(bx):
        c, r = i % cols, i // cols
        s.append(_case_svg(b, i, pad+c*(cw+gap), pad+40+r*(ch+gap), cw, ch))
    s.append('</svg>')
    open(out, 'w', encoding='utf-8').write(''.join(s))
    print('wrote', out, f'({len(bx)} cases)'); return 0

SIGN_NAMES = {'keepL':'keep left','keepR':'keep right','keepS':'keep straight','onL':'on the left','onR':'on the right','hairpin':'hairpin','roundaboutL':'roundabout L','roundaboutR':'roundabout R','keepMain':'keep on main',
 'caution':'caution','danger2':'danger 2','danger3':'danger 3','cutL':'cut left','cutR':'cut right','crestL':'crest left','crestR':'crest right','downhill':'steep descent','inclineL':'camber left','inclineR':'camber right','slowdown':'slow down','deadend':'dead end','lessVisible':'less visible','noentry':'no entry',
 'bump':'crest / bump','jump':'jump','dip':'dip','compression':'compression','bumpy':'bumpy','water':'water','dune':'dune','dunes':'dunes','rocks':'rocks','narrow':'narrows','ditch':'ditch','hole':'hole','ruts':'ruts','rough':'rough','sandpit':'sand pit','camelgrass':'camel grass','rocky':'rocky','chott':'salt flat','mountain':'mountain','hill':'hill','cliff':'cliff','collapse':'collapse',
 'river':'river','lake':'lake','pond':'pond','dryriver':'dry river','wadi':'wadi','bigwadi':'big wadi','canal':'canal',
 'fence':'fence','gate':'gate','bridge':'bridge','pole':'pole','antenna':'antenna','pipeline':'pipeline','railroad':'railroad','wall':'wall','barbwire':'barbed wire','cattleguard':'cattle guard','gatebar':'gate / barrier','hvline':'high-voltage','pump':'pumpjack','mine':'mine','cairn':'cairn','wellpost':'marker post','roadworks':'roadworks',
 'tree':'tree','bush':'bush','village':'village','house':'house','church':'church','cemetery':'cemetery','monument':'monument','ruins':'ruins','castle':'fort','camp':'camp','animals':'animals',
 'fuel':'fuel','fuelzone':'fuel zone','checkpoint':'checkpoint','mediapt':'media','photo':'photo','spectators':'spectators','police':'police','medical':'medical','helicopter':'helicopter','info':'info','north':'north','stop':'stop'}

def _signs_legend(rb):
    used = []
    for b in rb.get('boxes', []):
        for c in b.get('pictograms', []):
            if c not in used: used.append(c)
    items = []
    for c in used:
        d = _b64(c)
        if d: items.append(f'<span style="display:inline-flex;align-items:center;gap:3px;font-size:10px;color:#444;margin:0 8px 4px 0"><img src="{d}" style="width:16px;height:16px;object-fit:contain"> {SIGN_NAMES.get(c, c)}</span>')
    if not items: return ''
    return '<div style="margin:2px 0 8px"><b style="font-size:11px">Signs in this book:</b> ' + ''.join(items) + '</div>'

def _route_svg(rb, w, h):
    rt = rb.get('route', [])
    if len(rt) < 2: return ''
    xs = [r[0] for r in rt]; zs = [r[1] for r in rt]
    minx, maxx, minz, maxz = min(xs), max(xs), min(zs), max(zs)
    mg = 6.0; s = min((w - 2*mg) / max(1.0, maxx - minx), (h - 2*mg) / max(1.0, maxz - minz))
    ox = (w - (maxx - minx) * s) / 2; oz = (h - (maxz - minz) * s) / 2
    PX = lambda x: ox + (x - minx) * s; PY = lambda z: h - (oz + (z - minz) * s)
    pts = ' '.join(f'{PX(r[0]):.1f},{PY(r[1]):.1f}' for r in rt)
    out = [f'<svg viewBox="0 0 {w} {h}" width="{w}" height="{h}" style="background:#f3f1ea;border:1px solid #b8b0a8;border-radius:4px;flex:none">',
           f'<polyline points="{pts}" fill="none" stroke="#28b9eb" stroke-width="1.6" stroke-linejoin="round"/>']
    for b in rb.get('boxes', []):
        if b.get('dangerLevel', 0) > 0:
            d = b['distTotalKm'] * 1000; bx, bz = rt[-1][0], rt[-1][1]
            for r in rt:
                if r[2] >= d: bx, bz = r[0], r[1]; break
            out.append(f'<rect x="{PX(bx)-1.5:.1f}" y="{PY(bz)-1.5:.1f}" width="3" height="3" fill="#d89818"/>')
    out.append(f'<circle cx="{PX(rt[0][0]):.1f}" cy="{PY(rt[0][1]):.1f}" r="3.2" fill="#1f9d4e"/>')
    out.append(f'<circle cx="{PX(rt[-1][0]):.1f}" cy="{PY(rt[-1][1]):.1f}" r="3.2" fill="#cd2620"/></svg>')
    return ''.join(out)

def cmd_book(args):
    global FRENCH
    if '--french' in args: FRENCH = True; args = [a for a in args if a != '--french']
    if not args: return err('book needs <roadbook.json>')
    rb = load(args[0]); out = args[1] if len(args) > 1 else os.path.splitext(args[0])[0] + '.html'
    bx = rb.get('boxes', [])
    cases = ''.join(f'<div class="case"><svg viewBox="0 0 250 150" preserveAspectRatio="xMidYMid meet">{_case_svg(b, i, 0, 0, 250, 150)}</svg></div>' for i, b in enumerate(bx))
    title = f'{track_name(rb, os.path.splitext(os.path.basename(args[0]))[0])} — {total_km(rb):.2f} km, {len(bx)} boxes'
    html = ('<!DOCTYPE html><html><head><meta charset="utf-8"><title>Roadbook ' + title + '</title><style>'
            '@page{size:A4 portrait;margin:9mm} *{box-sizing:border-box}'
            'body{font-family:Segoe UI,Arial,sans-serif;margin:0;color:#18120f}'
            'h1{font-size:15px;margin:0 0 6px}'
            '.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:4px}'
            '.case{break-inside:avoid;border:1px solid #b8b0a8;border-radius:3px}'
            '.case svg{display:block;width:100%;height:auto}'
            '@media print{.noprint{display:none}}'
            '</style></head><body>'
            '<h1>ROADBOOK — ' + title + '</h1>'
            '<div style="display:flex;gap:12px;align-items:center;margin-bottom:8px">' + _route_svg(rb, 220, 120) +
            '<div style="font-size:11px;color:#555;line-height:1.5"><b>Stage overview</b><br>'
            '<span style="color:#1f9d4e">&#9679;</span> start &nbsp; <span style="color:#cd2620">&#9679;</span> finish'
            ' &nbsp; <span style="color:#d89818">&#9632;</span> danger</div></div>'
            + _signs_legend(rb) +
            '<p class="noprint" style="color:#777;font-size:12px">Print (Ctrl/Cmd-P) → Save as PDF for an A4 roadbook.</p>'
            '<div class="grid">' + cases + '</div></body></html>')
    open(out, 'w', encoding='utf-8').write(html)
    print('wrote', out, f'({len(bx)} cases, A4 print-ready)'); return 0

def cmd_diff(args):
    if len(args) < 2: return err('diff needs <a.json> <b.json>')
    from collections import Counter
    A = load(args[0]); B = load(args[1]); ba = A.get('boxes', []); bb = B.get('boxes', [])
    print(f'A: {len(ba)} boxes, {total_km(A):.2f} km    B: {len(bb)} boxes, {total_km(B):.2f} km')
    sa = Counter(s for x in ba for s in x.get('pictograms', [])); sb = Counter(s for x in bb for s in x.get('pictograms', []))
    deltas = [(k, sb[k] - sa[k]) for k in sorted(set(sa) | set(sb)) if sb[k] != sa[k]]
    if deltas: print('signs:', ', '.join(f'{k} {d:+d}' for k, d in deltas))
    diffs = 0
    for i in range(min(len(ba), len(bb))):
        x, y = ba[i], bb[i]; ch = []
        if x.get('type') != y.get('type'): ch.append(f"type {x.get('type')}->{y.get('type')}")
        if x.get('turnDir') != y.get('turnDir'): ch.append(f"dir {x.get('turnDir')}->{y.get('turnDir')}")
        if x.get('dangerLevel') != y.get('dangerLevel'): ch.append(f"danger {x.get('dangerLevel')}->{y.get('dangerLevel')}")
        if set(x.get('pictograms', [])) != set(y.get('pictograms', [])): ch.append('signs')
        if abs(x.get('distTotalKm', 0) - y.get('distTotalKm', 0)) > 0.005: ch.append(f"dist {x.get('distTotalKm',0):.2f}->{y.get('distTotalKm',0):.2f}")
        if ch: print(f'  box {i}: ' + '; '.join(ch)); diffs += 1
    if len(ba) != len(bb): print(f'  box count differs ({len(ba)} vs {len(bb)}); compared first {min(len(ba), len(bb))}')
    print(f'{diffs} box(es) differ' + (' + count' if len(ba) != len(bb) else ''))
    return 0

def cmd_batch(args):
    if len(args) < 2: return err('batch needs <rides_dir> <out_dir>')
    os.makedirs(args[1], exist_ok=True); n = 0
    for csv in glob.glob(os.path.join(args[0], '*.csv')):
        name = os.path.splitext(os.path.basename(csv))[0]
        cmd_gen([csv, os.path.join(args[1], name + '.json')]); n += 1
    print(f'batch: {n} roadbooks -> {args[1]}'); return 0

def cmd_union(args):
    if len(args) < 2: return err('union needs <primary.csv> <other.csv> [more.csv...] [out.json]')
    csvs = args[:]; out = csvs.pop() if csvs[-1].endswith('.json') else None
    if len(csvs) < 2: return err('union needs at least two ride CSVs')
    rc = subprocess.call([EXE, 'union'] + csvs)
    src = os.path.join(ROOT, r'build\union.json')
    if rc == 0 and out and os.path.exists(src): os.replace(src, out); print('wrote', out)
    return rc

def err(m): print('error:', m); return 2

CMDS = {'gen': cmd_gen, 'gpx': cmd_gpx, 'validate': cmd_validate, 'stats': cmd_stats, 'sheet': cmd_sheet, 'book': cmd_book, 'batch': cmd_batch, 'union': cmd_union, 'diff': cmd_diff}
if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in CMDS:
        print(__doc__); sys.exit(0 if len(sys.argv) < 2 else 2)
    sys.exit(CMDS[sys.argv[1]](sys.argv[2:]))
