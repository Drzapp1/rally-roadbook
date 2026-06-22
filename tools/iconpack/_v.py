from PIL import Image
import math
arrow = Image.open('arrows/arrow_up.png').convert('RGBA')
# show the white arrow on dark + simulate 4 exit directions (rotate the image)
sheet = Image.new('RGB',(5*120,150),(30,33,40))
def place(idx, im, label):
    from PIL import ImageDraw
    bg = Image.new('RGBA',(120,120),(50,54,64,255))
    a = im.resize((96,96))
    bg.paste(a,(12,12),a); sheet.paste(bg,(idx*120,0))
    ImageDraw.Draw(sheet).text((idx*120+6,126),label,fill=(225,225,225))
place(0, arrow, 'up (raw)')
# PIL rotate is CCW; arrow points up(screen). To point a screen dir, rotate by -atan2 from up.
for i,(name,dx,dy) in enumerate([('exit up',0,-1),('exit right',1,-0.2),('exit left',-1,-0.2),('exit up-right',0.7,-0.7)],start=1):
    ang = math.degrees(math.atan2(dx, -dy))   # cw angle from up
    place(i, arrow.rotate(-ang, resample=Image.BICUBIC, expand=False), name)
sheet.save('arrows/arrow_orient.png'); print('ok')
