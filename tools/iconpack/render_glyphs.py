# Render every glyph of the Rally Symbols font to a labelled contact sheet.
from PIL import Image, ImageDraw, ImageFont

FONT = r'D:\BikesRoadbook\tools\iconpack\font\Rally_Symbols.otf'
font = ImageFont.truetype(FONT, 52)

# which codepoints actually have glyphs (via fonttools if present)
codes = []
try:
    from fontTools.ttLib import TTFont
    tt = TTFont(FONT)
    cmap = tt.getBestCmap()
    codes = sorted(c for c in cmap.keys() if 32 < c < 0x2300)
    print('cmap codepoints:', len(codes))
except Exception as e:
    print('no fonttools (', e, ') - scanning ASCII+latin1')
    codes = list(range(33, 127)) + list(range(161, 256))

cols = 12
cell = 80
rows = (len(codes) + cols - 1) // cols
sheet = Image.new('RGB', (cols * cell, rows * cell), (26, 29, 36))
d = ImageDraw.Draw(sheet)
for i, cp in enumerate(codes):
    cx, cy = (i % cols) * cell, (i // cols) * cell
    d.rectangle([cx + 2, cy + 2, cx + cell - 2, cy + cell - 16], fill=(46, 50, 60))
    g = Image.new('RGBA', (cell, cell), (0, 0, 0, 0))
    ImageDraw.Draw(g).text((cell / 2, (cell - 14) / 2), chr(cp), font=font, fill=(255, 255, 255, 255), anchor='mm')
    sheet.paste(g, (cx, cy), g)
    d.text((cx + 3, cy + cell - 14), str(cp), fill=(150, 160, 175))
sheet.save(r'D:\BikesRoadbook\tools\iconpack\font_sheet.png')
print('wrote font_sheet.png with', len(codes), 'glyphs')
