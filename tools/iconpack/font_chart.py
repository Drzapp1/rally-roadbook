# Enumerate EVERY glyph in the rally-symbol font and render a labelled chart,
# so codes can be mapped to real font glyphs (no hand-drawing).
from PIL import Image, ImageDraw, ImageFont
import os
FONT = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'

cps = []
try:
    from fontTools.ttLib import TTFont
    f = TTFont(FONT)
    s = set()
    for t in f['cmap'].tables:
        s.update(t.cmap.keys())
    cps = sorted(s)
    print('cmap codepoints:', len(cps))
except Exception as e:
    print('fontTools missing/err:', e, '-> scanning 0x20..0x7E + 0xF000..0xF0FF')
    cps = list(range(0x20, 0x7F)) + list(range(0xF000, 0xF100))

pil = ImageFont.truetype(FONT, 46)
cols = 12
cell = 88
rows = (len(cps) + cols - 1) // cols
sheet = Image.new('RGB', (cols * cell, rows * cell), (24, 26, 32))
d = ImageDraw.Draw(sheet)
small = ImageFont.load_default()
for i, cp in enumerate(cps):
    cx, cy = (i % cols) * cell, (i // cols) * cell
    d.rectangle([cx + 2, cy + 2, cx + cell - 2, cy + cell - 16], fill=(46, 50, 60))
    try:
        d.text((cx + cell // 2, cy + (cell - 16) // 2 + 2), chr(cp), font=pil, fill=(255, 255, 255), anchor='mm')
    except Exception:
        pass
    d.text((cx + 4, cy + cell - 14), f'{cp}/{cp:#x}', font=small, fill=(180, 210, 160))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\font_chart.png')
print('wrote font_chart.png  rows=%d' % rows)
