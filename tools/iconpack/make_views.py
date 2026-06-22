# Illustrates the two device placements (Chase right-edge vs Onboard low-centre)
# over a faux first-person riding background. Purely a docs/preview image.
import os
from PIL import Image, ImageDraw
HERE = r'D:\BikesRoadbook\tools\iconpack'
dev = Image.open(os.path.join(HERE, 'device', 'device_mock.png')).convert('RGBA')

def riding_bg(w, h):
    bg = Image.new('RGB', (w, h)); d = ImageDraw.Draw(bg)
    for y in range(h):
        t = y / h
        if t < 0.55:  # sky
            c = (int(150 + 60 * t), int(180 + 40 * t), int(220 - 40 * t))
        else:         # desert ground
            u = (t - 0.55) / 0.45
            c = (int(196 - 30 * u), int(150 - 30 * u), int(96 - 20 * u))
        d.line([(0, y), (w, y)], fill=c)
    # distant hills + a track
    d.polygon([(0, int(h * 0.55)), (w * 0.3, int(h * 0.46)), (w * 0.6, int(h * 0.55))], fill=(150, 120, 90))
    d.polygon([(w * 0.45, int(h * 0.55)), (w, int(h * 0.49)), (w, int(h * 0.55))], fill=(140, 112, 84))
    d.polygon([(int(w * 0.42), int(h * 0.55)), (int(w * 0.58), int(h * 0.55)), (int(w * 0.72), h), (int(w * 0.28), h)], fill=(170, 132, 86))
    # handlebar hint (bottom)
    d.ellipse([-w * 0.1, h * 0.92, w * 0.45, h * 1.2], fill=(28, 30, 34))
    d.ellipse([w * 0.55, h * 0.92, w * 1.1, h * 1.2], fill=(28, 30, 34))
    return bg

def place(canvas, scale):
    w, h = canvas.size
    dw = int(0.182 * w * scale); dh = int(dw * dev.height / dev.width)
    return dw, dh

W, H = 960, 540
# CHASE
chase = riding_bg(W, H)
dw = int(0.182 * W); dh = int(dw * dev.height / dev.width)
ds = dev.resize((dw, dh))
chase.paste(ds, (int(0.812 * W), int(0.022 * H)), ds)
ImageDraw.Draw(chase).text((12, 12), 'CHASE  (right side)', fill=(20, 20, 20))
# ONBOARD
onb = riding_bg(W, H)
sc = 0.55; dw2 = int(0.182 * W * sc); dh2 = int(dw2 * dev.height / dev.width)
ds2 = dev.resize((dw2, dh2))
cx = int(0.5 * W - dw2 / 2); cy = int(0.66 * H - dh2 / 2)
onb.paste(ds2, (cx, cy), ds2)
ImageDraw.Draw(onb).text((12, 12), 'ONBOARD  (cockpit / low-centre)', fill=(20, 20, 20))

out = Image.new('RGB', (W * 2 + 12, H), (24, 26, 30))
out.paste(chase, (0, 0)); out.paste(onb, (W + 12, 0))
out.save(os.path.join(HERE, 'device', 'views.png'))
print('wrote views.png')
