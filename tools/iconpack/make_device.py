# Generates the rally-computer device body textures (rb_device0..N.tga) from
# REAL material photos (carbon-fibre weave, brushed aluminium, knurled metal —
# CC-licensed, in tools/iconpack/textures/) so every model looks like an actual
# device: the photo texture is modulated to the model's colour, then lit with a
# top-down gradient + bevel + recessed LCD/screen shadows. Layout (LCD windows,
# screen hole, buttons) is unchanged so drawDevice's fractions still line up.
import numpy as np, os, struct
from PIL import Image, ImageDraw, ImageFont, ImageOps, ImageFilter
HERE = r'D:\BikesRoadbook\tools\iconpack'
OUT  = os.path.join(HERE, 'device'); os.makedirs(OUT, exist_ok=True)
ICONS = r'D:\BikesRoadbook\data\icons'
TEX  = os.path.join(HERE, 'textures')

def F(sz, bold=True):
    for p in (r'C:\Windows\Fonts\consolab.ttf' if bold else r'C:\Windows\Fonts\consola.ttf',):
        try: return ImageFont.truetype(p, sz)
        except Exception: pass
    return ImageFont.load_default()
f_sm, f_xs = F(16, False), F(12)

W, H = 320, 950
# shared layout (px) — must match drawDevice fractions in roadbook_renderer.cpp
LCD_L = (18, 34, 152, 110); LCD_R = (170, 34, 303, 110)
BEZEL = (13, 162, 307, 864); HOLE = (20, 169, 300, 857); FOOT_Y = 898

# ---- real material textures (luminance), fit to the body; None => procedural ----
def tex_L(name):
    p = os.path.join(TEX, name)
    if not os.path.exists(p) or os.path.getsize(p) == 0: return None
    return ImageOps.fit(Image.open(p).convert('L'), (W, H), Image.LANCZOS)
CARBON = tex_L('carbon_fiber_weave_woven.jpg') or tex_L('carbon_fiber_weave_manchester.jpg')
BRUSH  = tex_L('brushed_aluminium.jpg')
def knurl_patch(w, h):
    p = os.path.join(TEX, 'knurled_metal_closeup.jpg')
    if not os.path.exists(p) or os.path.getsize(p) == 0: return None
    return ImageOps.fit(Image.open(p).convert('L'), (max(1, int(w)), max(1, int(h))), Image.LANCZOS)

# models: name, base rgb, bevel, screw, lcd-window bg, bezel, screen bg, label,
# brand text, weave(=carbon, else brushed), optional knobs(=roadbook holder)
MODELS = [
    dict(name='Carbon',  base=(34,37,42),   bevel=(86,90,98),  screw=(60,63,70),  lcdwin=(8,7,6),    bezel=(52,55,62),  screen=(18,19,23), label=(150,156,166), brand='RALLYBOOK', weave=True),
    dict(name='Tablet',  base=(40,43,49),   bevel=(120,126,135),screw=(70,74,82), lcdwin=(14,16,22), bezel=(28,30,36),  screen=(20,22,28), label=(150,200,235), brand='NAV-PAD',  weave=False),
    dict(name='Classic', base=(96,86,64),   bevel=(150,138,108),screw=(160,148,120),lcdwin=(12,16,8), bezel=(112,102,78),screen=(22,24,16), label=(196,184,140),brand='ICO 2000', weave=False),
    dict(name='Stealth', base=(20,20,23),   bevel=(54,54,60),  screw=(40,40,45),  lcdwin=(8,6,6),    bezel=(36,36,40),  screen=(13,13,15), label=(130,130,138), brand='STEALTH',  weave=True),
    dict(name='Desert',  base=(168,134,86), bevel=(210,180,124),screw=(132,108,70),lcdwin=(14,10,4),  bezel=(132,106,64),screen=(28,24,16), label=(80,62,36),    brand='DAKAR',    weave=False),
    dict(name='Night',   base=(22,30,50),   bevel=(58,74,116), screw=(46,58,90),  lcdwin=(6,8,14),   bezel=(44,58,92),  screen=(12,16,26), label=(120,150,200), brand='NIGHT',    weave=True),
    dict(name='Retro',   base=(40,48,36),   bevel=(86,104,74), screw=(78,94,66),  lcdwin=(6,12,6),   bezel=(66,82,56),  screen=(16,24,14), label=(140,180,120), brand='RETRO',    weave=False),
    dict(name='Enduro',  base=(40,30,22),   bevel=(255,128,34),screw=(132,98,64), lcdwin=(12,8,4),   bezel=(232,112,30),screen=(24,18,12), label=(248,150,64),  brand='ENDURO',   weave=True),
    dict(name='Titanium',base=(126,130,138),bevel=(196,200,206),screw=(150,154,162),lcdwin=(16,18,22),bezel=(146,150,158),screen=(28,30,34),label=(70,74,82),    brand='TITAN',    weave=False),
    dict(name='Forest',  base=(28,44,32),   bevel=(74,118,80), screw=(60,96,64),  lcdwin=(6,12,8),   bezel=(60,98,66),  screen=(16,26,18), label=(130,184,134), brand='TRAIL',    weave=True),
    dict(name='Crimson', base=(46,24,26),   bevel=(214,62,58), screw=(132,54,52), lcdwin=(14,6,6),   bezel=(190,56,52), screen=(26,16,16), label=(232,118,114), brand='ROSSO',    weave=True),
    dict(name='Ocean',   base=(18,40,54),   bevel=(48,128,170),screw=(44,98,128), lcdwin=(6,12,16),  bezel=(44,116,158),screen=(14,26,32), label=(120,180,212), brand='MARINE',   weave=False),
    dict(name='Sandstone',base=(182,160,126),bevel=(220,200,160),screw=(150,132,102),lcdwin=(16,12,6),bezel=(160,140,106),screen=(30,26,18),label=(96,80,54),    brand='SAHARA',   weave=False),
    dict(name='Mono',    base=(30,30,33),   bevel=(214,216,220),screw=(160,162,166),lcdwin=(10,10,12),bezel=(206,208,212),screen=(18,18,20),label=(170,172,176),brand='MONO',     weave=False),
    dict(name='Gold',    base=(34,30,22),   bevel=(224,182,82),screw=(160,128,54), lcdwin=(14,12,6),  bezel=(206,166,72),screen=(22,20,14), label=(226,188,96),  brand='PRESTIGE', weave=True),
    dict(name='Ranger',  base=(52,50,36),   bevel=(212,160,48),screw=(132,108,60),lcdwin=(12,12,6),   bezel=(160,128,48),screen=(24,24,16), label=(206,186,96),  brand='RANGER',   weave=False),
    dict(name='Ice',     base=(150,168,184),bevel=(224,236,246),screw=(176,190,204),lcdwin=(16,20,26),bezel=(168,184,200),screen=(26,30,36),label=(70,92,112),   brand='ARCTIC',   weave=False),
    dict(name='Copper',  base=(72,46,34),   bevel=(208,126,80), screw=(150,94,58), lcdwin=(14,8,4),   bezel=(188,114,70),screen=(26,18,14), label=(220,142,92),  brand='CUPRA',    weave=True),
    dict(name='Lime',    base=(30,36,26),   bevel=(160,224,56), screw=(80,100,46), lcdwin=(8,12,4),   bezel=(128,186,46),screen=(16,22,14), label=(172,224,80),  brand='VOLT',     weave=True),
    dict(name='Navy',    base=(18,26,46),   bevel=(56,78,134),  screw=(48,64,108), lcdwin=(6,8,16),   bezel=(50,72,124), screen=(12,16,28), label=(128,150,210), brand='ADMIRAL',  weave=False),
    # faithful real rally-computer replicas (roadbook holder with wind-knobs / modern)
    dict(name='RaidNav', base=(46,48,52),   bevel=(104,108,114),screw=(160,128,64),lcdwin=(14,10,4),  bezel=(76,80,86),  screen=(22,18,12), label=(206,160,76),  brand='RAID-NAV', weave=False, knobs=True),
    dict(name='Tripmaster',base=(24,24,27), bevel=(70,70,75),  screw=(118,96,54), lcdwin=(16,11,3),   bezel=(54,54,58),  screen=(24,20,12), label=(220,168,76),  brand='TRIPMASTER',weave=False, knobs=True),
    dict(name='RallyeF2', base=(26,26,30),  bevel=(216,64,58), screw=(132,54,50),  lcdwin=(12,10,12), bezel=(188,58,54), screen=(16,16,20), label=(232,118,114), brand='RALLYE F2',weave=True),
]
LCDCOL = [(255,176,40),(95,195,255),(120,230,95),(255,84,74),(255,176,40),(110,200,255),(130,235,110),(255,240,235),
          (180,220,255),(130,235,110),(255,90,80),(95,200,255),(255,186,60),(240,242,245),(255,200,90),(140,235,110),
          (200,225,255),(255,176,60),(160,235,70),(95,200,255),(255,176,40),(255,186,60),(255,120,90)]

# top-down light gradient + soft corner vignette (shared) — gives the body depth
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
GRAD = (1.16 - 0.34 * (yy / H))                                   # brighter at top
vig = 1.0 - 0.5 * (((xx - W/2)/(W/2))**2 + ((yy - H/2)/(H/2))**2)  # darker corners
LIGHT = (GRAD * np.clip(0.78 + 0.30*vig, 0.7, 1.06))[..., None]

def textured_base(m):
    src = CARBON if m['weave'] else BRUSH
    if src is not None:
        L = np.asarray(src, np.float32) / 255.0
        L = np.clip((L - L.mean()) * 1.35 + 0.5, 0.0, 1.0)        # contrast around mean
        fac = (0.46 + 0.92 * L)[..., None]                       # modulation 0.46..1.38
    else:
        fac = np.ones((H, W, 1), np.float32)
    arr = np.array(m['base'], np.float32)[None, None, :] * fac * LIGHT
    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), 'RGB')

def build(m):
    body = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    mask = Image.new('L', (W, H), 0); ImageDraw.Draw(mask).rounded_rectangle([0, 0, W-1, H-1], radius=30, fill=255)
    body.paste(textured_base(m).convert('RGBA'), (0, 0), mask)
    d = ImageDraw.Draw(body, 'RGBA')
    # bevel: bright top-left edge, dark inner line
    d.rounded_rectangle([1, 1, W-2, H-2], radius=30, outline=m['bevel'] + (255,), width=3)
    d.rounded_rectangle([5, 5, W-6, H-6], radius=26, outline=(8, 9, 11, 150), width=1)
    d.line([10, 8, W-10, 8], fill=tuple(min(255, c+50) for c in m['bevel']) + (170,), width=1)  # top glint
    # corner screws (knurled metal)
    kn = knurl_patch(18, 18)
    for sx, sy in [(24, 24), (W-24, 24), (24, H-24), (W-24, H-24)]:
        if kn is not None:
            sc = Image.new('RGBA', (18, 18), (0,0,0,0)); cm = Image.new('L', (18,18), 0)
            ImageDraw.Draw(cm).ellipse([0,0,17,17], fill=255)
            tint = np.array(m['screw'], np.float32)[None,None,:]*(np.asarray(kn,np.float32)/180.0)[...,None]
            sc.paste(Image.fromarray(np.clip(tint,0,255).astype(np.uint8),'RGB').convert('RGBA'),(0,0),cm)
            body.paste(sc, (sx-9, sy-9), sc)
            d.ellipse([sx-9, sy-9, sx+9, sy+9], outline=(14,15,18,220), width=1)
        else:
            d.ellipse([sx-8, sy-8, sx+8, sy+8], fill=m['screw'], outline=(18,19,22))
        d.line([sx-4, sy-1, sx+4, sy-1], fill=(18, 19, 22, 220), width=2)
    if m.get('knobs'):   # knurled wind-knobs + centre mount tab of a roadbook holder
        for kx in (50, W-50-50):
            kp = knurl_patch(50, 22)
            if kp is not None:
                tint = np.array(tuple(min(255,c+34) for c in m['base']),np.float32)[None,None,:]*(np.asarray(kp,np.float32)/170.0)[...,None]
                body.paste(Image.fromarray(np.clip(tint,0,255).astype(np.uint8),'RGB'), (kx, 6))
            d.rounded_rectangle([kx, 6, kx+50, 28], radius=7, outline=(10,11,13,255), width=2)
            d.line([kx+4, 9, kx+46, 9], fill=tuple(min(255,c+70) for c in m['base'])+(200,), width=1)
        d.rounded_rectangle([W//2-18, 3, W//2+18, 18], radius=4, fill=m['bevel'], outline=(10,11,13))
    # recessed LCD windows
    for box, lab in [(LCD_L, 'TOTAL  km'), (LCD_R, 'SPEED  km/h')]:
        d.rounded_rectangle(box, radius=9, fill=m['lcdwin'])
        d.rounded_rectangle([box[0], box[1], box[2], box[1]+14], radius=9, fill=tuple(min(255,c+16) for c in m['lcdwin']))  # top sheen
        d.rounded_rectangle(box, radius=9, outline=(2, 2, 2, 255), width=2)
        d.text((box[0]+4, box[1]-15), lab, font=f_xs, fill=m['label'])
    d.line([16, 127, 304, 127], fill=(8, 9, 11, 200), width=1)
    for lx, lab in [(16, 'NEXT'), (120, 'HDG'), (214, 'CAP')]:
        d.text((lx, 134), lab, font=f_xs, fill=m['label'])
    # screen: metallic bezel + recessed dark glass + inner shadow toward the hole
    d.rounded_rectangle(BEZEL, radius=12, fill=m['screen'])
    d.rounded_rectangle(BEZEL, radius=12, outline=m['bezel'], width=3)
    inner = Image.new('RGBA', (W, H), (0, 0, 0, 0)); di = ImageDraw.Draw(inner)
    for k, a in [(0, 150), (3, 90), (6, 45)]:
        di.rounded_rectangle([HOLE[0]-k, HOLE[1]-k, HOLE[2]+k, HOLE[3]+k], radius=8, outline=(0,0,0,a), width=2)
    body.alpha_composite(inner.filter(ImageFilter.GaussianBlur(1.2)))
    d.rounded_rectangle(HOLE, radius=8, outline=(0, 0, 0, 255), width=3)
    # glossy footer buttons
    for i, (col, lab) in enumerate([((214,52,46), 'F6'), ((72,184,92), 'F7'), ((60,124,214), 'F9')]):
        bx = 22 + i*40
        d.ellipse([bx, FOOT_Y, bx+24, FOOT_Y+24], fill=col, outline=(8, 9, 11, 255))
        d.ellipse([bx+5, FOOT_Y+4, bx+15, FOOT_Y+11], fill=tuple(min(255,c+70) for c in col)+(150,))  # highlight
        d.text((bx+4, FOOT_Y+26), lab, font=f_xs, fill=m['label'])
    d.text((W-126, FOOT_Y-2), m['brand'], font=f_sm, fill=m['label'])
    # cut the transparent screen hole
    hole = Image.new('L', (W, H), 0); ImageDraw.Draw(hole).rounded_rectangle(HOLE, radius=8, fill=255)
    r, g, b, a = body.split(); a = Image.composite(Image.new('L', (W, H), 0), a, hole)
    return Image.merge('RGBA', (r, g, b, a))

def to_tga(img, path):
    arr = np.array(img.convert('RGBA'))[::-1]; h, w = arr.shape[:2]
    hdr = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH', w, h) + bytes([32, 0x08])
    open(path, 'wb').write(hdr + arr[:, :, [2,1,0,3]].astype(np.uint8).tobytes())

bodies = []
for i, m in enumerate(MODELS):
    body = build(m); bodies.append(body)
    to_tga(body, os.path.join(ICONS, f'rb_device{i}.tga'))

# preview sheet: all models with sample digits
sheet = Image.new('RGB', (len(MODELS) * 150 + 20, 500), (40, 44, 52)); sd = ImageDraw.Draw(sheet)
for i, (m, body) in enumerate(zip(MODELS, bodies)):
    th = body.resize((140, int(140 * H / W)))
    bg = Image.new('RGBA', th.size, (m['screen'][0], m['screen'][1], m['screen'][2], 255)); bg.alpha_composite(th)
    sc = 140 / W; dd = ImageDraw.Draw(bg); lc = LCDCOL[i]
    dd.text((int(80*sc), int(60*sc)), '12.34', font=F(int(34*sc)), fill=lc)
    dd.text((int(196*sc), int(60*sc)), '108', font=F(int(34*sc)), fill=lc)
    sheet.paste(bg, (10 + i*150, 6)); sd.text((14 + i*150, 478), m['name'], fill=(225,225,225))
sheet.save(os.path.join(OUT, 'models_sheet.png'))
print('wrote', len(MODELS), 'device models (real-texture):', ', '.join(m['name'] for m in MODELS))
