# Generates a RANGE of rally-computer device body textures (rb_device0..N.tga),
# all sharing the same layout (so drawDevice's fractions work for every model)
# but with distinct finishes + LCD colours. Plus a preview sheet of all models.
import numpy as np, os, math, struct
from PIL import Image, ImageDraw, ImageFont
HERE = r'D:\BikesRoadbook\tools\iconpack'
OUT  = os.path.join(HERE, 'device'); os.makedirs(OUT, exist_ok=True)
ICONS = r'D:\BikesRoadbook\data\icons'
WEB  = r'C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\mxbmrp3_data\web'

def F(sz, bold=True):
    try: return ImageFont.truetype(WEB + ('\\RobotoMono-Bold.ttf' if bold else '\\RobotoMono-Regular.ttf'), sz)
    except Exception: return ImageFont.load_default()
f_sm, f_xs, f_lcd = F(17, False), F(13), F(38)

W, H = 320, 950
# shared layout (px) — must match drawDevice fractions in roadbook_renderer.cpp
LCD_L = (18, 34, 152, 110); LCD_R = (170, 34, 303, 110)
BEZEL = (13, 162, 307, 864); HOLE = (20, 169, 300, 857); FOOT_Y = 898

# models: name, base rgb, brightness tint, bevel, screw, lcd-window bg, bezel,
# screen bg, label colour, brand text, weave (carbon) or brushed
MODELS = [
    dict(name='Carbon',  base=(28,30,34),   tint=(.82,.9,1), bevel=(74,78,86),  screw=(56,59,66),  lcdwin=(8,7,6),    bezel=(50,53,60),  screen=(20,21,25), label=(120,124,132), brand='RALLYBOOK', weave=True),
    dict(name='Tablet',  base=(206,210,217),tint=(1,1,1),    bevel=(238,240,244),screw=(176,180,188),lcdwin=(18,20,26), bezel=(150,154,162),screen=(28,30,36), label=(96,100,110),  brand='NAV-PAD',  weave=False),
    dict(name='Classic', base=(74,68,52),   tint=(1,.95,.8), bevel=(126,116,92),screw=(150,140,116),lcdwin=(12,16,8),  bezel=(98,90,70),  screen=(24,26,18), label=(176,166,128), brand='ICO 2000', weave=False),
    dict(name='Stealth', base=(15,15,17),   tint=(1,1,1),    bevel=(46,46,50),  screw=(36,36,40),  lcdwin=(8,6,6),    bezel=(34,34,38),  screen=(14,14,16), label=(120,120,126), brand='STEALTH',  weave=True),
    dict(name='Desert',  base=(150,120,78), tint=(1,.95,.78),bevel=(196,166,112),screw=(120,98,64),lcdwin=(14,10,4),  bezel=(120,96,58), screen=(28,24,16), label=(70,56,34),    brand='DAKAR',    weave=False),
    dict(name='Night',   base=(18,24,40),   tint=(.9,.95,1), bevel=(44,58,90),  screw=(40,52,80),  lcdwin=(6,8,14),   bezel=(40,54,86),  screen=(12,16,26), label=(110,130,170), brand='NIGHT',    weave=True),
    dict(name='Retro',   base=(34,40,30),   tint=(.9,1,.85), bevel=(70,86,60),  screw=(70,86,60),  lcdwin=(6,12,6),   bezel=(60,76,50),  screen=(16,24,14), label=(120,160,110), brand='RETRO',    weave=False),
    dict(name='Enduro',  base=(34,26,20),   tint=(1,.92,.8), bevel=(255,120,30),screw=(120,90,60), lcdwin=(12,8,4),   bezel=(232,110,30),screen=(22,18,14), label=(232,140,60),  brand='ENDURO',   weave=True),
    dict(name='Titanium',base=(120,124,130),tint=(1,1,1),    bevel=(184,188,194),screw=(150,154,160),lcdwin=(16,18,22),bezel=(140,144,150),screen=(26,28,32), label=(70,74,82),    brand='TITAN',    weave=False),
    dict(name='Forest',  base=(24,38,28),   tint=(.9,1,.9),  bevel=(64,104,68), screw=(54,86,58),  lcdwin=(6,12,8),   bezel=(56,92,60),  screen=(16,26,18), label=(120,170,124), brand='TRAIL',    weave=True),
    dict(name='Crimson', base=(40,20,22),   tint=(1,.9,.9),  bevel=(196,56,54), screw=(120,50,50), lcdwin=(14,6,6),   bezel=(176,52,50), screen=(26,16,16), label=(220,110,108), brand='ROSSO',    weave=True),
    dict(name='Ocean',   base=(16,34,46),   tint=(.9,.97,1), bevel=(44,118,158),screw=(40,90,118), lcdwin=(6,12,16),  bezel=(40,108,148),screen=(14,24,30), label=(110,170,200), brand='MARINE',   weave=False),
    dict(name='Sandstone',base=(170,150,118),tint=(1,.97,.85),bevel=(212,192,152),screw=(140,124,96),lcdwin=(16,12,6),bezel=(150,132,100),screen=(30,26,18),label=(90,76,52),    brand='SAHARA',   weave=False),
    dict(name='Mono',    base=(24,24,26),   tint=(1,1,1),    bevel=(204,206,210),screw=(150,152,156),lcdwin=(10,10,12),bezel=(196,198,202),screen=(18,18,20),label=(150,152,156), brand='MONO',     weave=False),
    dict(name='Gold',    base=(26,24,20),   tint=(1,.96,.82),bevel=(214,172,72),screw=(150,120,50), lcdwin=(14,12,6),  bezel=(196,158,66),screen=(22,20,14), label=(214,176,86),  brand='PRESTIGE', weave=True),
    dict(name='Ranger',  base=(46,44,32),   tint=(1,.97,.82),bevel=(202,150,42),screw=(120,100,56),lcdwin=(12,12,6),  bezel=(150,120,44),screen=(24,24,16), label=(196,176,90),  brand='RANGER',   weave=False),
    dict(name='Ice',     base=(198,212,224),tint=(1,1,1),    bevel=(232,242,250),screw=(170,184,198),lcdwin=(16,20,26),bezel=(150,166,182),screen=(26,30,36),label=(80,98,116),   brand='ARCTIC',   weave=False),
    dict(name='Copper',  base=(60,38,28),   tint=(1,.92,.8), bevel=(196,116,72), screw=(140,86,54), lcdwin=(14,8,4),   bezel=(176,104,62),screen=(26,18,14), label=(208,132,84),  brand='CUPRA',    weave=True),
    dict(name='Lime',    base=(24,28,22),   tint=(.95,1,.9), bevel=(150,214,46), screw=(70,90,40),  lcdwin=(8,12,4),   bezel=(120,176,40),screen=(16,22,14), label=(160,214,70),  brand='VOLT',     weave=True),
    dict(name='Navy',    base=(16,22,40),   tint=(.92,.95,1),bevel=(52,72,124),  screw=(44,60,100), lcdwin=(6,8,16),   bezel=(46,66,116), screen=(12,16,28), label=(120,140,200), brand='ADMIRAL',  weave=False),
    # faithful real rally-computer replicas (holder with wind-knobs / classic trip / modern)
    dict(name='RaidNav', base=(40,42,46),   tint=(1,1,1),    bevel=(96,100,106),screw=(150,120,60),lcdwin=(14,10,4),  bezel=(70,74,80),  screen=(22,18,12), label=(196,150,70),  brand='RAID-NAV', weave=False, knobs=True),
    dict(name='Tripmaster',base=(20,20,22), tint=(1,1,1),    bevel=(64,64,68),  screw=(110,90,50), lcdwin=(16,11,3),  bezel=(50,50,54),  screen=(24,20,12), label=(210,160,70),  brand='TRIPMASTER',weave=False, knobs=True),
    dict(name='RallyeF2', base=(22,22,26),  tint=(1,.96,.96),bevel=(206,60,54), screw=(120,50,48), lcdwin=(12,10,12), bezel=(178,54,50), screen=(16,16,20), label=(220,110,108), brand='RALLYE F2',weave=True),
]
LCDCOL = [(255,176,40), (95,195,255), (120,230,95), (255,84,74),
          (255,176,40), (110,200,255), (130,235,110), (255,240,235),
          (180,220,255), (130,235,110), (255,90,80), (95,200,255),
          (255,186,60), (240,242,245), (255,200,90), (140,235,110),
          (200,225,255), (255,176,60), (160,235,70), (95,200,255),
          (255,176,40), (255,186,60), (255,120,90)]  # default LCD colour per model

def base_tex(m):
    b0 = m['base']; xx, yy = np.meshgrid(np.arange(W), np.arange(H))
    if m['weave']:
        weave = ((xx//3 + yy//3) % 2).astype(bool); diag = (((xx-yy) % 6) < 3)
        lvl = np.where(weave ^ diag, 12, 0) + np.random.randint(-3, 4, (H, W))
    else:
        lvl = (np.sin(xx*0.7) * 2).astype(np.int16) + (yy.astype(np.int16) * -0.012).astype(np.int16) + np.random.randint(-2, 3, (H, W))
    arr = np.stack([np.clip(b0[0] + lvl, 0, 255), np.clip(b0[1] + lvl, 0, 255), np.clip(b0[2] + lvl, 0, 255)], -1)
    return Image.fromarray(arr.astype(np.uint8), 'RGB')

def build(m):
    body = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    mask = Image.new('L', (W, H), 0); ImageDraw.Draw(mask).rounded_rectangle([0, 0, W-1, H-1], radius=28, fill=255)
    body.paste(base_tex(m).convert('RGBA'), (0, 0), mask)
    d = ImageDraw.Draw(body)
    d.rounded_rectangle([1, 1, W-2, H-2], radius=28, outline=m['bevel'] + (255,), width=2)
    d.rounded_rectangle([4, 4, W-5, H-5], radius=25, outline=(9, 10, 12, 160), width=1)
    for sx, sy in [(22, 22), (W-22, 22), (22, H-22), (W-22, H-22)]:
        d.ellipse([sx-8, sy-8, sx+8, sy+8], fill=m['screw'], outline=(18, 19, 22))
        d.line([sx-4, sy-1, sx+4, sy-1], fill=(20, 21, 24), width=2)
    if m.get('knobs'):   # knurled wind-knobs + mount tab of a real roadbook holder
        hi = tuple(min(255, c + 60) for c in m['base']); kn = tuple(min(255, c + 26) for c in m['base'])
        for kx in (52, W - 52 - 48):
            d.rounded_rectangle([kx, 6, kx + 48, 27], radius=8, fill=kn, outline=(10, 11, 13))
            for gx in range(kx + 5, kx + 46, 4):
                d.line([gx, 9, gx, 24], fill=(12, 13, 15), width=1)
            d.line([kx + 4, 9, kx + 44, 9], fill=hi, width=1)
        d.rounded_rectangle([W // 2 - 18, 3, W // 2 + 18, 17], radius=4, fill=m['bevel'], outline=(10, 11, 13))
    for box, lab in [(LCD_L, 'TOTAL  km'), (LCD_R, 'SPEED  km/h')]:
        d.rounded_rectangle(box, radius=9, fill=m['lcdwin'])
        d.rounded_rectangle(box, radius=9, outline=(2, 2, 2), width=2)
        d.text((box[0]+4, box[1]-16), lab, font=f_xs, fill=m['label'])
    d.line([16, 127, 304, 127], fill=(10, 11, 13), width=1)
    for lx, lab in [(16, 'NEXT'), (120, 'HDG'), (214, 'CAP')]:
        d.text((lx, 134), lab, font=f_xs, fill=m['label'])
    d.rounded_rectangle(BEZEL, radius=11, fill=m['screen'])
    d.rounded_rectangle(BEZEL, radius=11, outline=m['bezel'], width=3)
    d.rounded_rectangle(HOLE, radius=7, outline=(0, 0, 0), width=3)
    for i, (col, lab) in enumerate([((210,50,45), 'F6'), ((70,180,90), 'F7'), ((60,120,210), 'F9')]):
        bx = 22 + i*40
        d.ellipse([bx, FOOT_Y, bx+24, FOOT_Y+24], fill=col, outline=(8, 9, 11))
        d.text((bx+4, FOOT_Y+26), lab, font=f_xs, fill=m['label'])
    d.text((W-128, FOOT_Y-2), m['brand'], font=f_sm, fill=m['label'])
    hole = Image.new('L', (W, H), 0); ImageDraw.Draw(hole).rounded_rectangle(HOLE, radius=7, fill=255)
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
sheet = Image.new('RGB', (len(MODELS) * 180 + 20, 560), (40, 44, 52)); sd = ImageDraw.Draw(sheet)
for i, (m, body) in enumerate(zip(MODELS, bodies)):
    th = body.resize((170, int(170 * H / W)))
    bg = Image.new('RGBA', th.size, (m['screen'][0], m['screen'][1], m['screen'][2], 255)); bg.alpha_composite(th)
    sc = 170 / W
    dd = ImageDraw.Draw(bg); lc = LCDCOL[i]
    fb = F(int(38 * sc)); fs = F(int(17 * sc), False)
    dd.text((int(80*sc), int(56*sc)), '12.34', font=fb, fill=lc)
    dd.text((int(196*sc), int(56*sc)), '108', font=fb, fill=lc)
    dd.text((int(54*sc), int(130*sc)), '320', font=fs, fill=lc); dd.text((int(150*sc), int(130*sc)), '087', font=fs, fill=lc); dd.text((int(246*sc), int(130*sc)), '092', font=fs, fill=lc)
    sheet.paste(bg, (10 + i*180, 6)); sd.text((14 + i*180, 540), m['name'], fill=(225,225,225))
sheet.save(os.path.join(OUT, 'models_sheet.png'))
print('wrote', len(MODELS), 'device models:', ', '.join(m['name'] for m in MODELS))
