# PNG (white on transparent) -> game-format TGA, plus a contact sheet to verify.
from PIL import Image, ImageDraw
import os, struct

SRC  = r'D:\BikesRoadbook\tools\iconpack\png'
DST  = r'D:\BikesRoadbook\data\Roadbook_data\icons'
os.makedirs(DST, exist_ok=True)

def write_tga(img, path):
    img = img.convert('RGBA').resize((64, 64))
    px = img.load()
    hdr = bytes([0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0]) + struct.pack('<HH', 64, 64) + bytes([32, 0x08])
    data = bytearray()
    for y in range(63, -1, -1):           # bottom-left origin
        for x in range(64):
            r, g, b, a = px[x, y]
            data += bytes([b, g, r, a])    # BGRA
    with open(path, 'wb') as f:
        f.write(hdr); f.write(data)

codes = sorted(fn[:-4] for fn in os.listdir(SRC) if fn.endswith('.png'))
for code in codes:
    img = Image.open(os.path.join(SRC, code + '.png')).convert('RGBA')
    write_tga(img, os.path.join(DST, 'rb_' + code + '.tga'))

# contact sheet (white icons on dark cells + labels)
cols = 5
rows = (len(codes) + cols - 1) // cols
cell = 120
sheet = Image.new('RGB', (cols * cell, rows * cell), (26, 29, 36))
d = ImageDraw.Draw(sheet)
for i, code in enumerate(codes):
    cx, cy = (i % cols) * cell, (i // cols) * cell
    d.rectangle([cx + 4, cy + 4, cx + cell - 4, cy + cell - 28], fill=(48, 52, 62))
    icon = Image.open(os.path.join(SRC, code + '.png')).convert('RGBA').resize((76, 76))
    sheet.paste(icon, (cx + (cell - 76) // 2, cy + 12), icon)
    d.text((cx + 8, cy + cell - 22), code, fill=(220, 224, 230))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\contact.png')
print('wrote', len(codes), 'tga +', 'contact.png')
