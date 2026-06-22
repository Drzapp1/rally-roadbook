# Render the official arrowhead SVG to rb_arrow.tga (white, 64x64, points up).
import subprocess, struct, os
from PIL import Image
HERE = r'D:\BikesRoadbook\tools\iconpack'
DST  = r'D:\BikesRoadbook\data\icons'

# render SVG -> PNG via node/resvg
node = r'''import { Resvg } from '@resvg/resvg-js';
import { readFileSync, writeFileSync } from 'fs';
const r = new Resvg(readFileSync('arrowhead.svg','utf8'), { fitTo:{mode:'width',value:128}, background:'rgba(0,0,0,0)' });
writeFileSync('arrows/arrow_up.png', r.render().asPng());
'''
open(os.path.join(HERE, '_arrow.mjs'), 'w').write(node)
subprocess.run(['node', '_arrow.mjs'], cwd=HERE, check=True, shell=True)

def whiten_center(im, target=64, inner=60):
    im = im.convert('RGBA')
    bb = im.getbbox()
    if bb: im = im.crop(bb)
    _, _, _, a = im.split()
    w = Image.new('RGBA', im.size, (255, 255, 255, 0)); w.putalpha(a)
    iw, ih = w.size
    s = inner / max(iw, ih)
    nw, nh = max(1, round(iw * s)), max(1, round(ih * s))
    w = w.resize((nw, nh), Image.LANCZOS)
    c = Image.new('RGBA', (target, target), (0, 0, 0, 0))
    c.paste(w, ((target - nw) // 2, (target - nh) // 2), w)
    return c

def to_tga(img, path):
    img = img.convert('RGBA'); px = img.load()
    hdr = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH', 64, 64) + bytes([32, 0x08])
    data = bytearray()
    for y in range(63, -1, -1):
        for x in range(64):
            r, g, b, a = px[x, y]; data += bytes([b, g, r, a])
    open(path, 'wb').write(hdr + bytes(data))

to_tga(whiten_center(Image.open(os.path.join(HERE, 'arrows', 'arrow_up.png'))),
       os.path.join(DST, 'rb_arrow.tga'))
print('wrote rb_arrow.tga')
