#!/usr/bin/env python3
"""Render the stock tulip catalog to a contact sheet so the shapes can be eyeballed.
Same draw logic as the in-game/web renderer (cyan halo + ink polyline + entry dot +
arrowhead), fed CLEAN stock points instead of the recorded path."""
from PIL import Image, ImageDraw
import math

# stock tulips — normalized (x: 0..1 L->R, y: 0..1 where 1=TOP, 0=BOTTOM), entry first
STOCK = {
    'start':    [(.5,.10),(.5,.84)],
    'straight': [(.5,.08),(.5,.92)],
    'slightL':  [(.5,.08),(.5,.52),(.30,.88)],
    'slightR':  [(.5,.08),(.5,.52),(.70,.88)],
    'left':     [(.5,.08),(.5,.56),(.14,.56)],
    'right':    [(.5,.08),(.5,.56),(.86,.56)],
    'sharpL':   [(.5,.08),(.5,.60),(.26,.36)],
    'sharpR':   [(.5,.08),(.5,.60),(.74,.36)],
    'hairpinL': [(.5,.08),(.5,.70),(.30,.80),(.30,.34)],
    'hairpinR': [(.5,.08),(.5,.70),(.70,.80),(.70,.34)],
    'finish':   [(.5,.08),(.5,.80)],
}
CYAN=(40,185,235); INK=(24,18,15); PAPER=(243,241,234)

def draw_tulip(d, code, ox, oy, w, h):
    pts=STOCK[code]; m=14
    X=lambda p: ox+m+p[0]*(w-2*m); Y=lambda p: oy+m+(1-p[1])*(h-2*m)
    px=[(X(p),Y(p)) for p in pts]
    d.line(px, fill=CYAN, width=11, joint='curve')
    d.line(px, fill=INK,  width=6,  joint='curve')
    # entry dot
    ex,ey=px[0]; d.ellipse([ex-7,ey-7,ex+7,ey+7], fill=INK)
    if code=='start':
        d.ellipse([ex-13,ey-13,ex+13,ey+13], outline=INK, width=3)
    if code=='finish':
        fx,fy=px[-1]
        for k in range(-2,3):  # chequered finish bar
            c=INK if k%2 else (150,150,150)
            d.rectangle([fx+k*9-4, fy-9, fx+k*9+5, fy+1], fill=c)
        return
    # arrowhead at the exit along the last segment
    ax,ay=px[-1]; bx,by=px[-2]; dx,dy=ax-bx,ay-by; L=math.hypot(dx,dy) or 1; ux,uy=dx/L,dy/L; nx,ny=-uy,ux; s=20
    d.polygon([(ax+ux*s,ay+uy*s),(ax+nx*s*0.62,ay+ny*s*0.62),(ax-nx*s*0.62,ay-ny*s*0.62)], fill=INK)

codes=list(STOCK); cols=6; rows=(len(codes)+cols-1)//cols; cw=150; ch=190
sheet=Image.new('RGB',(cols*cw, rows*ch),(60,64,72)); d=ImageDraw.Draw(sheet)
for i,code in enumerate(codes):
    cx=(i%cols)*cw; cy=(i//cols)*ch
    d.rectangle([cx+4,cy+4,cx+cw-4,cy+ch-22], fill=PAPER)
    draw_tulip(d, code, cx+4, cy+4, cw-8, ch-44)
    d.text((cx+8,cy+ch-18), code, fill=(230,232,236))
sheet.save('D:/BikesRoadbook/build/_tulips.png'); print('wrote build/_tulips.png:', codes)
