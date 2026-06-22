# Compose web preview images of every rally-computer device (the device frame
# with a sample roadbook strip on its screen + live LCD readouts) so the models
# can be browsed on the website. Reads the generated rb_device*.tga.
import os, json
from PIL import Image, ImageDraw, ImageFont
ROOT = r'D:\BikesRoadbook'
ICONS = os.path.join(ROOT, 'data', 'icons')
TUL  = os.path.join(ROOT, 'web', 'tulips')
WEBDEV = os.path.join(ROOT, 'web', 'devices'); os.makedirs(WEBDEV, exist_ok=True)
W, H = 320, 950
LCD_L = (18, 34, 152, 110); LCD_R = (170, 34, 303, 110); HOLE = (20, 169, 300, 857)
NAMES = ['Carbon','Tablet','Classic','Stealth','Desert','Night','Retro','Enduro','Titanium','Forest',
         'Crimson','Ocean','Sandstone','Mono','Gold','Ranger','Ice','Copper','Lime','Navy','RaidNav','Tripmaster','RallyeF2']
LCDCOL = [(255,176,40),(95,195,255),(120,230,95),(255,84,74),(255,176,40),(110,200,255),(130,235,110),(255,240,235),
          (180,220,255),(130,235,110),(255,90,80),(95,200,255),(255,186,60),(240,242,245),(255,200,90),(140,235,110),
          (200,225,255),(255,176,60),(160,235,70),(95,200,255),(255,176,40),(255,186,60),(255,120,90)]

def F(sz, bold=True):
    for p in (r'C:\Windows\Fonts\consolab.ttf' if bold else r'C:\Windows\Fonts\consola.ttf',):
        try: return ImageFont.truetype(p, sz)
        except Exception: pass
    return ImageFont.load_default()

def read_tga(path):
    d = open(path, 'rb').read()
    w = int.from_bytes(d[12:14], 'little'); h = int.from_bytes(d[14:16], 'little'); bpp = d[16]
    off = 18 + d[0]; px = d[off:off + w*h*(bpp//8)]
    im = Image.frombytes('RGBA' if bpp == 32 else 'RGB', (w, h), px, 'raw', 'BGRA' if bpp == 32 else 'BGR')
    if not (d[17] & 0x20): im = im.transpose(Image.FLIP_TOP_BOTTOM)
    return im.convert('RGBA')

def sample_strip():
    sw, sh = HOLE[2]-HOLE[0], HOLE[3]-HOLE[1]
    im = Image.new('RGBA', (sw, sh), (13, 15, 19, 255)); d = ImageDraw.Draw(im)
    cells = [('left','12,84','540','L'),('straight','13,02','300','kpS'),('hairpinR','13,18','180','R'),('slightL','13,56','420','L')]
    ch = sh // len(cells); z1, z2 = int(sw*0.30), int(sw*0.72)
    for i, (code, tk, pt, cp) in enumerate(cells):
        y = i*ch
        d.rectangle([2, y+2, sw-3, y+ch-3], outline=(58, 64, 74), width=1)
        d.line([z1, y+2, z1, y+ch-2], fill=(40, 44, 52), width=1); d.line([z2, y+2, z2, y+ch-2], fill=(40, 44, 52), width=1)
        d.text((8, y+8), tk, font=F(26), fill=(230, 234, 240))
        d.rectangle([6, y+ch-28, z1-6, y+ch-6], outline=(70, 76, 86), width=1); d.text((10, y+ch-26), pt, font=F(13), fill=(160, 166, 176))
        tp = os.path.join(TUL, code + '.png')
        if os.path.exists(tp):
            ts = min(int(sw*0.40), ch) - 10; t = Image.open(tp).convert('RGBA').resize((ts, ts))
            im.alpha_composite(t, (z1 + (z2-z1-ts)//2, y + (ch-ts)//2))
        d.text((z2+10, y+10), cp, font=F(26), fill=(230, 234, 240)); d.text((z2+10, y+40), '087', font=F(12), fill=(150, 156, 166))
    return im

STRIP = sample_strip()

def compose(i):
    frame = read_tga(os.path.join(ICONS, f'rb_device{i}.tga'))
    out = Image.new('RGBA', (W, H), (20, 22, 27, 255))
    out.alpha_composite(STRIP, (HOLE[0], HOLE[1]))
    out.alpha_composite(frame)
    d = ImageDraw.Draw(out); lc = LCDCOL[i]
    d.text((LCD_L[2]-8, LCD_L[1]+30), '12.34', font=F(30), fill=lc, anchor='rm')
    d.text((LCD_R[2]-8, LCD_R[1]+30), '108', font=F(30), fill=lc, anchor='rm')
    for x, v in [(40, '320'), (140, '087'), (236, '092')]:
        d.text((x, 150), v, font=F(13), fill=lc, anchor='mm')
    out.convert('RGB').save(os.path.join(WEBDEV, f'device{i}.jpg'), quality=86)

for i in range(len(NAMES)): compose(i)
json.dump([{'id': i, 'name': NAMES[i]} for i in range(len(NAMES))], open(os.path.join(WEBDEV, 'manifest.json'), 'w'), indent=1)
print('wrote', len(NAMES), 'device web previews ->', WEBDEV)
