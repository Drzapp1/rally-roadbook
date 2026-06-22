from PIL import Image
import os, glob
DIR = r'D:\BikesRoadbook\tools\iconpack\arrows'
pngs = sorted(glob.glob(os.path.join(DIR, '*.png')))
print('pngs:', [os.path.basename(p) for p in pngs], 'sizes:', [Image.open(p).size for p in pngs])
cell = 220
sheet = Image.new('RGB', (cell * len(pngs), cell + 24), (40, 44, 52))
from PIL import ImageDraw
d = ImageDraw.Draw(sheet)
for i, p in enumerate(pngs):
    im = Image.open(p).convert('RGBA')
    s = min((cell - 20) / im.width, (cell - 20) / im.height)
    im = im.resize((max(1, int(im.width * s)), max(1, int(im.height * s))), Image.NEAREST)
    bg = Image.new('RGBA', (cell, cell), (255, 255, 255, 255))  # white bg to see black art
    bg.paste(im, ((cell - im.width) // 2, (cell - im.height) // 2), im)
    sheet.paste(bg, (i * cell, 0))
    d.text((i * cell + 6, cell + 4), os.path.basename(p), fill=(230, 230, 230))
sheet.save(os.path.join(DIR, 'arrows_sheet.png'))
print('wrote arrows_sheet.png')
