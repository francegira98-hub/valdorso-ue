# Valdorso - Disegna la schermata d'avvio (Splash.bmp, EdSplash.bmp) e l'icona (Application.ico).
# Scritto da Claude il 03/10/2026. U = cartella con Font/ e Texture/valdorso/ (sul PC: V:\). Richiede Pillow e numpy.
import math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops
U='V:/'
CINZEL=U+'Font/Cinzel/static/Cinzel-Bold.ttf'
CINZEL_R=U+'Font/Cinzel/static/Cinzel-Regular.ttf'
GARA=U+'Font/EB_Garamond/static/EBGaramond-Italic.ttf'
FONDO=(14,11,9); ORO=(212,175,55); OROC=(243,220,140); PERG=(232,217,181); SEC=(201,185,154); BRACE=(217,119,43)

def gradiente_oro(maschera):
    w,h=maschera.size
    y=np.linspace(0,1,h)[:,None]
    a=np.array(OROC,float); b=np.array(ORO,float); c=np.array((150,110,35),float)
    t=np.clip(y,0,1)
    col=np.where(t<0.55, a+(b-a)*(t/0.55), b+(c-b)*((t-0.55)/0.45))
    arr=np.repeat(col[:,None,:],w,axis=1).astype(np.uint8)
    img=Image.fromarray(arr,'RGB').convert('RGBA'); img.putalpha(maschera); return img

def splash(W,H,S=3):
    w,h=W*S,H*S
    rng=random.Random(1312)
    # cielo: ritaglio del nostro cielo notturno, scurito e scaldato
    cielo=Image.open(U+'Texture/valdorso/T_CieloNotte.png').convert('RGB')
    cw,ch=cielo.size
    crop=cielo.crop((int(cw*0.30),int(ch*0.08),int(cw*0.30)+int(ch*0.42*w/h),int(ch*0.08)+int(ch*0.42))).resize((w,h),Image.LANCZOS)
    c=np.asarray(crop).astype(float)/255
    c=c*np.array([0.55,0.50,0.62])*0.85
    base=np.array(FONDO,float)/255
    yy=np.linspace(0,1,h)[:,None,None]; xx=np.linspace(-1,1,w)[None,:,None]
    vign=np.clip(1-0.55*(xx**2)-0.35*(yy-0.42)**2*3,0.25,1)
    img=(base+c*0.9)*vign
    # bagliore di brace in basso al centro
    glow=np.exp(-((xx/0.55)**2+((yy-1.05)/0.35)**2))
    img=img+glow*np.array(BRACE)/255*0.32
    # fascia bassa scura per il testo dell'editor
    fade=np.clip((yy-0.80)/0.12,0,1)
    img=img*(1-0.55*fade)
    im=Image.fromarray((np.clip(img,0,1)*255).astype(np.uint8),'RGB').convert('RGBA')

    cx,cy=w//2,int(h*0.40)
    # cerchio di rune, oro tenue
    rune=Image.open(U+'Texture/valdorso/T_CerchioRune.png').convert('L')
    R=int(h*0.86)
    rune=rune.resize((R,R),Image.LANCZOS)
    m=rune.point(lambda v:int(v*0.30))
    strato=Image.new('RGBA',(R,R),ORO+(0,)); strato.putalpha(m)
    im.alpha_composite(strato,(cx-R//2,cy-R//2))

    # titolo
    fs=int(h*0.235)
    font=ImageFont.truetype(CINZEL,fs)
    testo='VALDORSO'
    sp=int(fs*0.06)
    larg=[font.getbbox(ch)[2]-font.getbbox(ch)[0] for ch in testo]
    adv=[font.getlength(ch) for ch in testo]
    tot=sum(adv)+sp*(len(testo)-1)
    m=Image.new('L',(w,h),0); d=ImageDraw.Draw(m)
    x=cx-tot/2; asc,desc=font.getmetrics()
    ty=cy-int(fs*0.62)
    for ch,a in zip(testo,adv):
        d.text((x,ty),ch,font=font,fill=255); x+=a+sp
    # bagliore
    alone=m.filter(ImageFilter.GaussianBlur(fs*0.18)).point(lambda v:int(v*0.55))
    s=Image.new('RGBA',(w,h),BRACE+(0,)); s.putalpha(alone); im.alpha_composite(s)
    ombra=m.filter(ImageFilter.GaussianBlur(fs*0.03)).point(lambda v:int(v*0.8))
    s=Image.new('RGBA',(w,h),(0,0,0,0)); s.putalpha(ombra); im.alpha_composite(s,(int(S*2),int(S*3)))
    bbox=m.getbbox()
    sub=m.crop((0,bbox[1],w,bbox[3]))
    g=gradiente_oro(sub)
    im.alpha_composite(g,(0,bbox[1]))

    # linea con rombo
    ly=bbox[3]+int(h*0.075)
    d=ImageDraw.Draw(im)
    lw=int(w*0.30); t=max(1,S)
    for side in (-1,1):
        x0=cx+side*int(h*0.03); x1=cx+side*lw
        for i in range(abs(x1-x0)):
            xx_=x0+side*i; a=int(200*(1-i/abs(x1-x0))**1.3)
            d.line([(xx_,ly),(xx_,ly+t-1)],fill=ORO+(a,))
    r=int(h*0.014)
    d.polygon([(cx,ly-r),(cx+r,ly+t//2),(cx,ly+r+t),(cx-r,ly+t//2)],fill=OROC+(255,))

    # motto
    fm=ImageFont.truetype(GARA,int(h*0.07))
    motto='Ogni gesto lascia un segno.'
    mw=fm.getlength(motto)
    d.text((cx-mw/2,ly+int(h*0.035)),motto,font=fm,fill=SEC+(235,))

    # braci che salgono
    braci=Image.new('RGBA',(w,h),(0,0,0,0)); db=ImageDraw.Draw(braci)
    for i in range(150):
        x=rng.gauss(cx,w*0.22); y=h*(0.55+rng.random()*0.5)
        rr=rng.uniform(0.6,2.2)*S
        a=int(rng.uniform(80,230)*(y/h))
        colr=(255,int(rng.uniform(120,190)),int(rng.uniform(40,90)),a)
        db.ellipse([x-rr,y-rr,x+rr,y+rr],fill=colr)
    alone=braci.filter(ImageFilter.GaussianBlur(3*S))
    im.alpha_composite(alone); im.alpha_composite(braci)
    # bordo sottile oro
    d=ImageDraw.Draw(im)
    d.rectangle([S*3,S*3,w-S*3-1,h-S*3-1],outline=(120,95,45,160),width=S)
    return im.resize((W,H),Image.LANCZOS).convert('RGB')

def icona(N=256,S=4):
    n=N*S
    im=Image.new('RGBA',(n,n),(0,0,0,0))
    # fondo tondo scuro con bordo oro
    m=Image.new('L',(n,n),0); ImageDraw.Draw(m).rounded_rectangle([0,0,n-1,n-1],radius=int(n*0.2),fill=255)
    yy=np.linspace(0,1,n)[:,None]; xx=np.linspace(-1,1,n)[None,:]
    g=np.exp(-((xx/0.8)**2+((yy-0.95)/0.45)**2))
    base=np.array(FONDO,float)[None,None,:]+g[...,None]*np.array([70,30,8])[None,None,:]
    base=np.repeat(base,1,axis=0)
    fondo=Image.fromarray(np.clip(base,0,255).astype(np.uint8),'RGB').convert('RGBA'); fondo.putalpha(m)
    im.alpha_composite(fondo)
    d=ImageDraw.Draw(im)
    d.rounded_rectangle([int(n*0.03)]*2+[n-1-int(n*0.03)]*2,radius=int(n*0.18),outline=ORO+(255,),width=int(n*0.025))
    # cerchio sottile
    r=int(n*0.36); c=n//2
    d.ellipse([c-r,c-r+int(n*0.02),c+r,c+r+int(n*0.02)],outline=ORO+(150,),width=int(n*0.012))
    # V
    f=ImageFont.truetype(CINZEL,int(n*0.66))
    mm=Image.new('L',(n,n),0); dm=ImageDraw.Draw(mm)
    bb=f.getbbox('V'); vw=bb[2]-bb[0]; vh=bb[3]-bb[1]
    dm.text((c-vw/2-bb[0],c-vh/2-bb[1]+int(n*0.01)),'V',font=f,fill=255)
    al=mm.filter(ImageFilter.GaussianBlur(n*0.04)).point(lambda v:int(v*0.6))
    s=Image.new('RGBA',(n,n),BRACE+(0,)); s.putalpha(al); im.alpha_composite(s)
    bx=mm.getbbox(); sub=mm.crop((0,bx[1],n,bx[3])); im.alpha_composite(gradiente_oro(sub),(0,bx[1]))
    # rombo di brace sotto la V
    rr=int(n*0.04); y0=int(n*0.80)
    d=ImageDraw.Draw(im); d.polygon([(c,y0-rr),(c+rr,y0),(c,y0+rr),(c-rr,y0)],fill=(240,150,60,255))
    return im.resize((N,N),Image.LANCZOS)

s=splash(900,464); s.save('Splash.bmp'); s.save('anteprima_splash.png')
e=splash(900,464); e.save('EdSplash.bmp')
ic=icona(256); ic.save('anteprima_icona.png')
ic.save('Application.ico',sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
# anteprima icona in piccolo
prev=Image.new("RGBA",(470,280),(40,40,40,255))
x=10
for sz in (256,64,48,32,16):
    prev.alpha_composite(ic.resize((sz,sz),Image.LANCZOS),(x,10)); x+=sz+10
prev.save('anteprima_icone_misure.png')
print('ok')
