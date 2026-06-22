# Final roadbook sprite set:
#   - hand-drawn line-style lexicon symbols (authentic roadbook look)
#   - rally-font signs (caution/stop/info/fuel/noentry/north)
#   - a few game-icons (rocks/jump/fork) + arrows (keepL/keepR)
# Output white 64x64 game-format TGAs + a contact sheet.
from PIL import Image, ImageDraw, ImageFont
import os, struct, math

ICON_PNG = r'D:\BikesRoadbook\tools\iconpack\png'
FONT     = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'
DST      = r'D:\BikesRoadbook\data\icons'
os.makedirs(DST, exist_ok=True)
W = (255, 255, 255, 255)
LW = 5

def canvas():
    im = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)

def poly(d, pts): d.line(pts, fill=W, width=LW, joint='curve')

def s_crest():       im, d = canvas(); poly(d, [(7,42),(18,42),(26,22),(32,17),(38,22),(46,42),(57,42)]); return im
def s_dip():         im, d = canvas(); poly(d, [(7,24),(18,24),(26,44),(32,49),(38,44),(46,24),(57,24)]); return im
def s_bump():        im, d = canvas(); poly(d, [(6,43),(23,43),(28,30),(32,26),(36,30),(41,43),(58,43)]); return im
def s_compression(): im, d = canvas(); poly(d, [(7,24),(25,24),(27,46),(37,46),(39,24),(57,24)]); return im
def s_ditch():       im, d = canvas(); poly(d, [(7,26),(28,26),(32,50),(36,26),(57,26)]); return im
def s_hole():
    im, d = canvas(); d.ellipse([16,16,48,48], outline=W, width=LW)
    d.line([(23,23),(41,41)], fill=W, width=LW); d.line([(41,23),(23,41)], fill=W, width=LW); return im
def s_ruts():
    im, d = canvas()
    for cx in (20, 32, 44):
        poly(d, [(cx + 4*math.sin(y*0.45), y) for y in range(10, 56, 2)])
    return im
def s_water():
    im, d = canvas()
    for cy in (22, 32, 42):
        poly(d, [(x, cy + 4*math.sin(x*0.4)) for x in range(8, 58, 2)])
    return im
def s_narrow():
    im, d = canvas()
    poly(d, [(12,12),(28,32),(12,52)]); poly(d, [(52,12),(36,32),(52,52)]); return im
def s_twisty():
    im, d = canvas()
    poly(d, [(16,8),(42,20),(20,32),(46,44),(20,56)]); return im
def s_bumpy():
    im, d = canvas(); poly(d, [(6,46),(14,28),(22,46),(30,28),(38,46),(46,28),(54,46)]); return im
def s_gate():
    im, d = canvas()
    for x in (14, 23, 32, 41, 50): d.line([(x,20),(x,52)], fill=W, width=4)
    d.line([(12,20),(52,20)], fill=W, width=4); return im

LINE = {
    'crest': s_crest, 'dip': s_dip, 'bump': s_bump, 'compression': s_compression,
    'ditch': s_ditch, 'hole': s_hole, 'ruts': s_ruts, 'water': s_water,
    'narrow': s_narrow, 'twisty': s_twisty, 'bumpy': s_bumpy, 'gate': s_gate,
}
FONTG = {'caution': 33, 'stop': 86, 'info': 73, 'fuel': 75, 'noentry': 47, 'north': 94}
GAME  = {'rocks': 'rocks', 'jump': 'jump', 'fork': 'fork'}

def to_tga(img, path):
    img = img.convert('RGBA').resize((64, 64))
    px = img.load()
    hdr = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH',64,64) + bytes([32,0x08])
    data = bytearray()
    for y in range(63,-1,-1):
        for x in range(64):
            r,g,b,a = px[x,y]; data += bytes([b,g,r,a])
    open(path,'wb').write(hdr + bytes(data))

def glyph(cp):
    f = ImageFont.truetype(FONT, 54)
    im = Image.new('RGBA',(64,64),(0,0,0,0))
    ImageDraw.Draw(im).text((32,30), chr(cp), font=f, fill=W, anchor='mm'); return im

final = {}
for code, fn in LINE.items(): final[code] = fn()
for code, cp in FONTG.items(): final[code] = glyph(cp)
for code, name in GAME.items():
    p = os.path.join(ICON_PNG, name + '.png')
    if os.path.exists(p): final[code] = Image.open(p).convert('RGBA')
ap = os.path.join(ICON_PNG, 'arrow.png')
if os.path.exists(ap):
    a = Image.open(ap).convert('RGBA')
    final['keepR'] = a.rotate(90, expand=False); final['keepL'] = a.rotate(-90, expand=False)

for code, im in final.items(): to_tga(im, os.path.join(DST, f'rb_{code}.tga'))

codes = sorted(final)
cols = 6; cell = 116; rows = (len(codes)+cols-1)//cols
sheet = Image.new('RGB',(cols*cell, rows*cell),(26,29,36)); dd = ImageDraw.Draw(sheet)
for i, code in enumerate(codes):
    cx, cy = (i%cols)*cell, (i//cols)*cell
    dd.rectangle([cx+4,cy+4,cx+cell-4,cy+cell-24], fill=(48,52,62))
    ic = final[code].resize((72,72)); sheet.paste(ic,(cx+(cell-72)//2,cy+10),ic)
    dd.text((cx+8,cy+cell-20), code, fill=(220,224,230))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\sprites2.png')
print('wrote', len(codes), 'sprites:', ', '.join(sorted(final)))
