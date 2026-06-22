# Whiten + autocrop + center the rasterized lexicon glyphs into 64x64 game TGAs.
# info/north come from the rally font (no SVG equivalent). Clears the old set.
from PIL import Image, ImageDraw, ImageFont
import os, struct, glob
SRC  = r'D:\BikesRoadbook\tools\iconpack\png_tulip'
DST  = r'D:\BikesRoadbook\data\icons'
FONT = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'
os.makedirs(DST, exist_ok=True)

def whiten_center(im, target=64, inner=58):
    im = im.convert('RGBA')
    bb = im.getbbox()
    if bb: im = im.crop(bb)
    _, _, _, a = im.split()
    w = Image.new('RGBA', im.size, (255, 255, 255, 0)); w.putalpha(a)
    iw, ih = w.size
    s = inner / max(iw, ih)
    nw, nh = max(1, round(iw * s)), max(1, round(ih * s))
    w = w.resize((nw, nh), Image.LANCZOS)
    canvas = Image.new('RGBA', (target, target), (0, 0, 0, 0))
    canvas.paste(w, ((target - nw) // 2, (target - nh) // 2), w)
    return canvas

def to_tga(img, path):
    img = img.convert('RGBA'); px = img.load()
    hdr = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH', 64, 64) + bytes([32, 0x08])
    data = bytearray()
    for y in range(63, -1, -1):
        for x in range(64):
            r, g, b, a = px[x, y]; data += bytes([b, g, r, a])
    open(path, 'wb').write(hdr + bytes(data))

def glyph(cp):
    f = ImageFont.truetype(FONT, 54)
    im = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    ImageDraw.Draw(im).text((32, 30), chr(cp), font=f, fill=(255, 255, 255, 255), anchor='mm')
    return im

for old in glob.glob(os.path.join(DST, 'rb_*.tga')): os.remove(old)

final = []
for f in sorted(glob.glob(os.path.join(SRC, '*.png'))):
    code = os.path.splitext(os.path.basename(f))[0]
    to_tga(whiten_center(Image.open(f)), os.path.join(DST, f'rb_{code}.tga'))
    final.append(code)
for code, cp in {'info': 73, 'north': 94, 'stop': 86}.items():
    to_tga(glyph(cp), os.path.join(DST, f'rb_{code}.tga')); final.append(code)

final = sorted(final)
cols = 8; cell = 92; rows = (len(final) + cols - 1) // cols
sheet = Image.new('RGB', (cols * cell, rows * cell), (26, 29, 36)); d = ImageDraw.Draw(sheet)
for i, code in enumerate(final):
    cx, cy = (i % cols) * cell, (i // cols) * cell
    d.rectangle([cx + 3, cy + 3, cx + cell - 3, cy + cell - 18], fill=(50, 54, 64))
    ic = Image.open(os.path.join(DST, f'rb_{code}.tga')).convert('RGBA').resize((64, 64))
    sheet.paste(ic, (cx + (cell - 64) // 2, cy + 8), ic)
    d.text((cx + 5, cy + cell - 16), code, fill=(220, 224, 230))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\tulip_sprites.png')
print('wrote', len(final), 'tga:', ', '.join(final))
