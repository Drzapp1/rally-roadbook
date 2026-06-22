# Build the final roadbook sprite set:
#   - iconic rally signs rendered from Rally_Symbols.otf (authentic)
#   - terrain pictograms from game-icons.net PNGs
# Output white 64x64 game-format TGAs + a contact sheet to verify.
from PIL import Image, ImageDraw, ImageFont
import os, struct

ICON_PNG = r'D:\BikesRoadbook\tools\iconpack\png'
FONT     = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'
DST      = r'D:\BikesRoadbook\data\icons'
os.makedirs(DST, exist_ok=True)

# rally-font signs:  code -> codepoint
FONTG = {'caution': 33, 'stop': 86, 'info': 73, 'fuel': 75, 'noentry': 47, 'north': 94}
# game-icons terrain: code -> png basename
ICONS = {'crest': 'crest', 'dip': 'dip', 'bump': 'bump', 'hole': 'hole', 'rocks': 'rocks',
         'ruts': 'ruts', 'water': 'water', 'jump': 'jump', 'narrow': 'narrow', 'fork': 'fork', 'gate': 'gate'}

def to_tga(img, path):
    img = img.convert('RGBA').resize((64, 64))
    px = img.load()
    hdr = bytes([0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0]) + struct.pack('<HH', 64, 64) + bytes([32, 0x08])
    data = bytearray()
    for y in range(63, -1, -1):
        for x in range(64):
            r, g, b, a = px[x, y]
            data += bytes([b, g, r, a])
    open(path, 'wb').write(hdr + bytes(data))

def glyph(cp):
    f = ImageFont.truetype(FONT, 54)
    im = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    ImageDraw.Draw(im).text((32, 30), chr(cp), font=f, fill=(255, 255, 255, 255), anchor='mm')
    return im

final = {}
for code, cp in FONTG.items():
    im = glyph(cp); to_tga(im, os.path.join(DST, f'rb_{code}.tga')); final[code] = im
for code, name in ICONS.items():
    p = os.path.join(ICON_PNG, name + '.png')
    if os.path.exists(p):
        im = Image.open(p).convert('RGBA'); to_tga(im, os.path.join(DST, f'rb_{code}.tga')); final[code] = im
# keepL / keepR from the down-arrow icon
ap = os.path.join(ICON_PNG, 'arrow.png')
if os.path.exists(ap):
    a = Image.open(ap).convert('RGBA')
    kr = a.rotate(90, expand=False)    # down -> right
    kl = a.rotate(-90, expand=False)   # down -> left
    to_tga(kr, os.path.join(DST, 'rb_keepR.tga')); final['keepR'] = kr
    to_tga(kl, os.path.join(DST, 'rb_keepL.tga')); final['keepL'] = kl

codes = sorted(final)
cols = 6; cell = 120; rows = (len(codes) + cols - 1) // cols
sheet = Image.new('RGB', (cols * cell, rows * cell), (26, 29, 36)); d = ImageDraw.Draw(sheet)
for i, code in enumerate(codes):
    cx, cy = (i % cols) * cell, (i // cols) * cell
    d.rectangle([cx + 4, cy + 4, cx + cell - 4, cy + cell - 26], fill=(48, 52, 62))
    ic = final[code].resize((76, 76)); sheet.paste(ic, (cx + (cell - 76) // 2, cy + 12), ic)
    d.text((cx + 8, cy + cell - 22), code, fill=(220, 224, 230))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\sprites.png')
print('wrote', len(codes), 'sprites:', ', '.join(codes))
