# High-res paged glyph chart for the rally-symbol font (readable codepoints).
from PIL import Image, ImageDraw, ImageFont
FONT = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'
WEB  = r'C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\mxbmrp3_data\web'
from fontTools.ttLib import TTFont
tt = TTFont(FONT); s = set()
for t in tt['cmap'].tables: s.update(t.cmap.keys())
cps = sorted(s)

glyph = ImageFont.truetype(FONT, 70)
lab   = ImageFont.truetype(WEB + r'\RobotoMono-Bold.ttf', 18)
PAGE, cols, cell = 40, 8, 132
import math
pages = math.ceil(len(cps) / PAGE)
for p in range(pages):
    chunk = cps[p*PAGE:(p+1)*PAGE]
    rows = math.ceil(len(chunk) / cols)
    img = Image.new('RGB', (cols*cell, rows*cell), (24, 26, 32)); d = ImageDraw.Draw(img)
    for i, cp in enumerate(chunk):
        cx, cy = (i % cols)*cell, (i // cols)*cell
        d.rectangle([cx+3, cy+3, cx+cell-3, cy+cell-24], fill=(48, 52, 62))
        try: d.text((cx+cell//2, cy+(cell-24)//2+3), chr(cp), font=glyph, fill=(255,255,255), anchor='mm')
        except Exception: pass
        d.text((cx+6, cy+cell-22), str(cp), font=lab, fill=(190, 220, 170))
    img.save(rf'D:\BikesRoadbook\tools\iconpack\font_p{p}.png')
print('pages:', pages, 'codepoints:', len(cps))
