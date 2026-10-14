import cv2, numpy as np
from PIL import Image, ImageDraw, ImageFilter
src='/home/claude/stuck/assets_src/sheet.png'; out='/home/claude/stuck/Resources/'
im=cv2.imread(src)  # BGR
# ---- knob ----
cx,cy,r=361,1015,163
k=im.copy()
b,g,rr=[k[:,:,i].astype(int) for i in range(3)]
orange=((rr-b)>40)&(rr>70)
reg=np.zeros_like(orange); reg[cy-r:cy-r+80, cx-25:cx+25]=True
mask=(orange&reg).astype(np.uint8)*255
mask=cv2.dilate(mask,np.ones((5,5),np.uint8),iterations=2)
crop=k[cy-r-2:cy+r+3, cx-r-2:cx+r+3].copy()
S0=crop.shape[0]; c0=(S0-1)/2
def rot(a): return cv2.warpAffine(crop,cv2.getRotationMatrix2D((c0,c0),a,1.0),(S0,S0),flags=cv2.INTER_CUBIC,borderMode=cv2.BORDER_REFLECT)
yy,xx=np.mgrid[0:S0,0:S0]; ang=np.degrees(np.arctan2(xx-c0,-(yy-c0)))
w=np.clip((14-np.abs(ang))/5,0,1)[...,None]
w*= (np.hypot(xx-c0,yy-c0)>8)[...,None]
patch=(rot(28).astype(float)+rot(-28).astype(float))/2
crop=(crop*(1-w)+patch*w).astype(np.uint8)
crop=cv2.cvtColor(crop,cv2.COLOR_BGR2RGB)
S=crop.shape[0]
big=4
m=Image.new('L',(S*big,S*big),0)
ImageDraw.Draw(m).ellipse([(2)*big,(2)*big,(2+2*r)*big,(2+2*r)*big],fill=255)
m=m.resize((S,S),Image.LANCZOS)
knob=Image.fromarray(crop).convert('RGBA'); knob.putalpha(m)
knob.save(out+'knob.png')
# ---- panel ----
p=im[0:815,207:1048].copy()
p=cv2.cvtColor(p,cv2.COLOR_BGR2RGB)
# black out white rounded corners
h,w=p.shape[:2]
ff=np.zeros((h+2,w+2),np.uint8)
for sx,sy in [(0,0),(w-1,0),(0,h-1),(w-1,h-1)]:
    if p[sy,sx].sum()>600:
        cv2.floodFill(p,ff,(sx,sy),(12,11,10),(40,40,40),(40,40,40))
# blank readout interior
x0,y0,x1,y1=331,644,511,677
fill=np.array([10,8,6],np.uint8)
sub=p[y0:y1,x0:x1].astype(int)
p[y0:y1,x0:x1]=fill
Image.fromarray(p).save(out+'panel.png')
print('knob',knob.size,'panel',p.shape)
