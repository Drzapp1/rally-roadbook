# Authentic rally-roadbook TURN TULIPS as crisp image assets (supersampled PIL).
# These replace the live quad-drawing in the renderer: one image per turn class,
# styled like a real roadbook tulip — a white-cased route line with a cyan
# "piste" highlight, a solid entry ball (where you come from) and a filled
# arrowhead (where to exit), plus faint stubs for the roads you DON'T take.
# Exports PNG (web/tulips) + 32-bit TGA (data/icons/rb_tulip_<code>.tga).
import os, struct, math
from PIL import Image, ImageDraw

ROOT = r'D:\BikesRoadbook'
WEB  = os.path.join(ROOT, 'web', 'tulips'); os.makedirs(WEB, exist_ok=True)
ICON = os.path.join(ROOT, 'data', 'icons')
OUT  = os.path.join(ROOT, 'tools', 'iconpack');
SS   = 4                      # supersample factor
N    = 256                    # final px
C    = N * SS

# colours (RGBA)
CASE  = (248, 248, 244, 255)  # white casing so the tulip reads on any background
PISTE = (40, 162, 226, 255)   # cyan route highlight (the "piste" you ride)
CORE  = (24, 30, 42, 255)     # dark route core
BALL  = (24, 30, 42, 255)     # entry ball
STUB  = (120, 126, 134, 255)  # roads you don't take
STUBC = (236, 236, 232, 230)  # stub casing

def lerp(a, b, t): return (a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t)

# Each tulip: a route polyline (entry at bottom-centre -> exit), optional stub
# polylines (other roads), and the exit heading is taken from the last segment.
# Coordinates are unit square, y DOWN (0 top, 1 bottom). Entry ball at route[0].
TULIPS = {
    'straight': dict(route=[(.5,.90),(.5,.12)]),
    'slightL':  dict(route=[(.5,.90),(.5,.46),(.34,.13)]),
    'slightR':  dict(route=[(.5,.90),(.5,.46),(.66,.13)]),
    'left':     dict(route=[(.5,.90),(.5,.44),(.12,.44)], stub=[[(.5,.44),(.5,.16)]]),
    'right':    dict(route=[(.5,.90),(.5,.44),(.88,.44)], stub=[[(.5,.44),(.5,.16)]]),
    'sharpL':   dict(route=[(.5,.90),(.5,.40),(.24,.62)], stub=[[(.5,.40),(.62,.18)]]),
    'sharpR':   dict(route=[(.5,.90),(.5,.40),(.76,.62)], stub=[[(.5,.40),(.38,.18)]]),
    'hairpinL': dict(route=[(.5,.90),(.5,.30),(.30,.30),(.30,.66)]),
    'hairpinR': dict(route=[(.5,.90),(.5,.30),(.70,.30),(.70,.66)]),
    'forkL':    dict(route=[(.5,.90),(.5,.52),(.30,.16)], stub=[[(.5,.52),(.72,.16)]]),
    'forkR':    dict(route=[(.5,.90),(.5,.52),(.70,.16)], stub=[[(.5,.52),(.28,.16)]]),
    'cross':    dict(route=[(.5,.90),(.5,.12)], stub=[[(.14,.46),(.86,.46)]]),
    'start':    dict(route=[(.5,.90),(.5,.20)], flag='start'),
    'finish':   dict(route=[(.5,.90),(.5,.22)], flag='finish'),
}

def px(p): return (p[0]*C, p[1]*C)

def wline(d, pts, col, w):
    xy = [px(p) for p in pts]
    d.line(xy, fill=col, width=int(w), joint='curve')
    r = w/2.0
    for p in xy:  # round caps/joints
        d.ellipse([p[0]-r, p[1]-r, p[0]+r, p[1]+r], fill=col)

def arrowhead(d, tip, frm, col, size):
    ang = math.atan2(tip[1]-frm[1], tip[0]-frm[0])
    t = px(tip)
    L, Wd = size*C, size*C*0.62
    back = (t[0]-math.cos(ang)*L, t[1]-math.sin(ang)*L)
    nx, ny = -math.sin(ang), math.cos(ang)
    p1 = (back[0]+nx*Wd, back[1]+ny*Wd); p2 = (back[0]-nx*Wd, back[1]-ny*Wd)
    d.polygon([t, p1, p2], fill=col)

def draw(code, spec):
    im = Image.new('RGBA', (C, C), (0,0,0,0)); d = ImageDraw.Draw(im)
    route = spec['route']
    # stubs (roads not taken) first, under the route
    for s in spec.get('stub', []):
        wline(d, s, STUBC, 26*SS); wline(d, s, STUB, 13*SS)
    # route: white casing -> cyan piste -> dark core
    wline(d, route, CASE, 30*SS)
    wline(d, route, PISTE, 21*SS)
    wline(d, route, CORE, 9*SS)
    # entry ball (white-cased dark dot)
    b = px(route[0]); rr = 13*SS
    d.ellipse([b[0]-rr-3*SS, b[1]-rr-3*SS, b[0]+rr+3*SS, b[1]+rr+3*SS], fill=CASE)
    d.ellipse([b[0]-rr, b[1]-rr, b[0]+rr, b[1]+rr], fill=BALL)
    # exit marker
    flag = spec.get('flag')
    if flag == 'finish':
        t = px(route[-1])                       # checkered finish bar
        sq = 7*SS
        for r_ in range(2):
            for c_ in range(6):
                col = CORE if (r_+c_) % 2 == 0 else CASE
                x0 = t[0]-3*sq + c_*sq; y0 = t[1]-sq + r_*sq
                d.rectangle([x0, y0, x0+sq, y0+sq], fill=col)
    elif flag == 'start':
        arrowhead(d, route[-1], route[-2], CASE, 0.16)
        arrowhead(d, route[-1], route[-2], CORE, 0.13)
    else:                                        # arrowhead at exit
        arrowhead(d, route[-1], route[-2], CASE, 0.17)
        arrowhead(d, route[-1], route[-2], PISTE, 0.145)
        arrowhead(d, route[-1], route[-2], CORE, 0.075)
    return im.resize((N, N), Image.LANCZOS)

def to_tga(img, path):
    arr = img.convert('RGBA'); px_ = arr.load()
    hdr = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH', N, N) + bytes([32, 0x08])
    data = bytearray()
    for y in range(N-1, -1, -1):
        for x in range(N):
            r,g,b,a = px_[x,y]; data += bytes([b,g,r,a])
    open(path, 'wb').write(hdr + bytes(data))

made = []
for code, spec in TULIPS.items():
    im = draw(code, spec)
    im.save(os.path.join(WEB, code + '.png'))
    to_tga(im, os.path.join(ICON, f'rb_tulip_{code}.tga'))
    made.append(code)

# contact sheets on dark + light to check legibility
for bg, tag in [((22,25,32), 'dark'), ((238,236,228), 'light')]:
    cols=7; cell=140; rows=(len(made)+cols-1)//cols
    sheet=Image.new('RGB',(cols*cell, rows*cell+20), bg); d=ImageDraw.Draw(sheet)
    for i,code in enumerate(made):
        im=Image.open(os.path.join(WEB,code+'.png')).convert('RGBA').resize((cell-20,cell-20))
        x=(i%cols)*cell; y=(i//cols)*cell; sheet.paste(im,(x+10,y+6),im)
        d.text((x+8,y+cell-14), code, fill=(255-bg[0],255-bg[1],255-bg[2]))
    sheet.save(os.path.join(OUT, f'_tulips_{tag}.png'))
print('wrote', len(made), 'tulips:', ', '.join(made))
