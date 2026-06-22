# Faithful pixel mock of the new roadbook case layout (Paper style) to verify
# the look against the reference before in-game testing.
from PIL import Image, ImageDraw, ImageFont
import math

WEB = r'C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\mxbmrp3_data\web'
try:
    bold = ImageFont.truetype(WEB + r'\RobotoMono-Bold.ttf', 30)
    bold_s = ImageFont.truetype(WEB + r'\RobotoMono-Bold.ttf', 22)
    reg = ImageFont.truetype(WEB + r'\RobotoMono-Regular.ttf', 16)
    reg_s = ImageFont.truetype(WEB + r'\RobotoMono-Regular.ttf', 13)
except Exception:
    bold = bold_s = reg = reg_s = ImageFont.load_default()

INK = (24, 21, 19); DIM = (120, 114, 106); PAPER = (243, 241, 234); PAPERA = (252, 251, 247)
CYAN = (40, 185, 235); RED = (206, 38, 28); AMBER = (232, 148, 22)

W, H = 300, 162  # one case (px), ~matches normalized strip cell at 1080p
cells = [
    dict(total="0,20", part="0,20", num="1", abv="R",   cap="328", dl=0,
         tul=[(0.5,0.05),(0.5,0.45),(0.62,0.63),(0.86,0.72)]),
    dict(total="0,52", part="0,32", num="2", abv="kpS", cap="072", dl=0,
         tul=[(0.5,0.05),(0.5,0.95)]),
    dict(total="1,22", part="0,29", num="3", abv="R",   cap="204", dl=2,
         tul=[(0.4,0.05),(0.4,0.5),(0.46,0.74),(0.6,0.74),(0.66,0.5),(0.66,0.12)]),
]

img = Image.new('RGB', (W, H * len(cells) + 4 * (len(cells) + 1)), (60, 64, 72))
d = ImageDraw.Draw(img)

def cell(ox, oy, c, active):
    d.rectangle([ox, oy, ox + W, oy + H], fill=PAPER, outline=AMBER if active else (70, 66, 60), width=3 if active else 1)
    lw, cw = int(W * 0.30), int(W * 0.42)
    zx1, zx2 = ox + lw, ox + lw + cw
    d.line([zx1, oy, zx1, oy + H], fill=DIM); d.line([zx2, oy, zx2, oy + H], fill=DIM)
    # left
    d.text((ox + 8, oy + 14), c["total"], font=bold, fill=INK)
    pbx, pby, pbw, pbh = ox + 6, oy + H - 38, int(lw * 0.62), 26
    d.rectangle([pbx, pby, pbx + pbw, pby + pbh], fill=PAPERA, outline=DIM)
    d.text((pbx + 5, pby + 5), c["part"], font=reg, fill=INK)
    nsx, nsy = zx1 - 24, oy + H - 28
    d.rectangle([nsx, nsy, nsx + 20, nsy + 22], fill=PAPERA, outline=DIM)
    d.text((nsx + 6, nsy + 3), c["num"], font=reg_s, fill=INK)
    # centre tulip
    cx, cy = zx1 + cw / 2, oy + H * 0.45
    hh = H * 0.32; hw = hh
    def SX(px): return cx + (px - 0.5) * 2 * hw
    def SY(py): return cy - (py - 0.5) * 2 * hh
    pts = [(SX(p[0]), SY(p[1])) for p in c["tul"]]
    d.line(pts, fill=CYAN, width=10, joint='curve')   # highlight
    d.line(pts, fill=INK, width=5, joint='curve')      # black line
    ex, ey = SX(c["tul"][0][0]), SY(c["tul"][0][1])
    d.ellipse([ex - 7, ey - 7, ex + 7, ey + 7], fill=INK)
    # arrow head at exit
    x1, y1 = pts[-2]; x2, y2 = pts[-1]
    dx, dy = x2 - x1, y2 - y1; L = math.hypot(dx, dy) or 1; dx, dy = dx / L, dy / L
    ah, aw = 22, 13
    tip = (x2 + dx * ah, y2 + dy * ah)
    d.polygon([tip, (x2 - dy * aw, y2 + dx * aw), (x2 + dy * aw, y2 - dx * aw)], fill=INK)
    # right
    rcx = zx2 + (W * 0.28) / 2
    tb = d.textbbox((0, 0), c["abv"], font=bold_s)
    d.text((rcx - (tb[2] - tb[0]) / 2, oy + 20), c["abv"], font=bold_s, fill=INK)
    cb = d.textbbox((0, 0), c["cap"], font=reg_s)
    d.text((rcx - (cb[2] - cb[0]) / 2, oy + 52), c["cap"], font=reg_s, fill=DIM)
    if c["dl"] > 0:
        d.text((zx2 + 6, oy + H - 26), "!" * c["dl"], font=bold_s, fill=RED)

y = 4
for i, c in enumerate(cells):
    cell(6, y, c, active=(i == 0))
    y += H + 4
img.save(r'D:\BikesRoadbook\tools\iconpack\mock_cells.png')
print('wrote mock_cells.png')
