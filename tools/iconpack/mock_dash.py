# Faithful pixel mock of the rally trip-computer dash (ERTF/F2R style).
from PIL import Image, ImageDraw, ImageFont
WEB = r'C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\mxbmrp3_data\web'
big   = ImageFont.truetype(WEB + r'\RobotoMono-Bold.ttf', 36)
mid   = ImageFont.truetype(WEB + r'\RobotoMono-Regular.ttf', 16)
small = ImageFont.truetype(WEB + r'\RobotoMono-Regular.ttf', 12)

BG=(10,11,13); FRAME=(64,66,72); LCD=(255,174,32); LCDD=(120,84,22); GREY=(150,150,154); RED=(235,95,95)
W,H = 430, 143
img = Image.new('RGB',(W+20,H+20),(38,40,46)); d=ImageDraw.Draw(img)
ox,oy=10,10
d.rectangle([ox,oy,ox+W,oy+H], fill=BG, outline=FRAME, width=2)
midx = ox+int(W*0.52)
d.line([midx,oy+22,midx,oy+H-26], fill=FRAME, width=1)
d.text((ox+14,oy+10),"RALLY TRIP",font=mid,fill=GREY)
# REC
d.ellipse([ox+W-70,oy+9,ox+W-58,oy+21], fill=(230,40,40))
d.text((ox+W-52,oy+10),"REC",font=small,fill=RED)
# TOTAL
d.text((ox+16,oy+38),"TOTAL km",font=small,fill=LCDD)
d.text((ox+16,oy+54),"12.34",font=big,fill=LCD)
# NEXT
d.text((midx+16,oy+38),"NEXT m",font=small,fill=LCDD)
d.text((midx+16,oy+54),"320",font=big,fill=LCD)
# bottom
d.text((ox+16,oy+H-26),"HDG 087",font=mid,fill=LCD)
d.text((midx+16,oy+H-26),"CAP 092",font=mid,fill=LCD)
d.text((ox+W-34,oy+H-24),"F4",font=small,fill=GREY)
img.save(r'D:\BikesRoadbook\tools\iconpack\mock_dash.png'); print('wrote mock_dash.png')
