import math, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter
N = 2048; C = N / 2
img = Image.new('L', (N, N), 0)
d = ImageDraw.Draw(img)
def cerchio(r, w):
    d.ellipse([C - r, C - r, C + r, C + r], outline=255, width=w)
cerchio(1000, 7); cerchio(965, 4); cerchio(850, 5); cerchio(720, 4); cerchio(340, 6); cerchio(318, 3)
# iscrizione
font = ImageFont.truetype('/mnt/user-data/uploads/Font/Cinzel/static/Cinzel-Bold.ttf', 74)
frase = "IL CUORE BATTE ANCORA  ✦  "
testo = frase * 2
# larghezze
larg = []
for ch in testo:
    if ch == '✦':
        larg.append(60)
    else:
        b = font.getbbox(ch); larg.append(max(b[2] - b[0], 22) + 10)
tot = sum(larg)
rt = 905
scala = (2 * math.pi * rt) / tot
theta = 90.0  # gradi, in basso; il testo procede con theta decrescente
for ch, w in zip(testo, larg):
    passo = math.degrees(w * scala / rt)
    centro = theta - passo / 2
    if ch.strip():
        tile = Image.new('L', (160, 160), 0)
        td = ImageDraw.Draw(tile)
        if ch == '✦':
            cx, cy, r = 80, 80, 26
            pts = []
            for k in range(8):
                a = math.radians(k * 45 - 90); rr = r if k % 2 == 0 else r * 0.32
                pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
            td.polygon(pts, fill=255)
        else:
            td.text((80, 80), ch, font=font, fill=255, anchor='mm')
        rot = tile.rotate(90 - centro, resample=Image.BICUBIC)
        a = math.radians(centro)
        x = C + rt * math.cos(a); y = C + rt * math.sin(a)
        img.paste(255, (int(x - 80), int(y - 80)), rot)
    theta -= passo
# rune inventate
random.seed(7)
def runa(cx, cy, ang, h=95):
    tile = Image.new('L', (200, 200), 0); t = ImageDraw.Draw(tile)
    w = 7; top, bot = 100 - h / 2, 100 + h / 2
    t.line([(100, top), (100, bot)], fill=255, width=w)
    for _ in range(random.randint(1, 3)):
        y0 = random.choice([top, top + h * 0.25, 100, top + h * 0.75])
        lato = random.choice([-1, 1]); lung = random.choice([28, 36])
        dy = random.choice([-30, 0, 30, 22])
        t.line([(100, y0), (100 + lato * lung, y0 + dy)], fill=255, width=w)
        if random.random() < 0.3:
            t.line([(100 + lato * lung, y0 + dy), (100 + lato * lung, y0 + dy + 30)], fill=255, width=w)
    if random.random() < 0.25:
        t.ellipse([88, top - 26, 112, top - 2], outline=255, width=5)
    rot = tile.rotate(90 - ang, resample=Image.BICUBIC)
    img.paste(255, (int(cx - 100), int(cy - 100)), rot)
for i in range(28):
    ang = i * 360 / 28 + 6
    a = math.radians(ang); r = 785
    runa(C + r * math.cos(a), C + r * math.sin(a), ang)
# pentagono con i cerchi ai vertici
vert = [(C + 700 * math.cos(math.radians(-90 + k * 72)), C + 700 * math.sin(math.radians(-90 + k * 72))) for k in range(5)]
d.line(vert + [vert[0]], fill=255, width=6, joint='curve')
for k in range(5):
    a = math.radians(-90 + k * 72 + 36)
    d.line([(C + 340 * math.cos(a), C + 340 * math.sin(a)), (C + 566 * math.cos(a), C + 566 * math.sin(a))], fill=150, width=3)
    a = math.radians(-90 + k * 72)
    d.line([(C + 340 * math.cos(a), C + 340 * math.sin(a)), (C + 654 * math.cos(a), C + 654 * math.sin(a))], fill=150, width=3)
for (x, y) in vert:
    d.ellipse([x - 46, y - 46, x + 46, y + 46], fill=0, outline=255, width=6)
    d.ellipse([x - 14, y - 14, x + 14, y + 14], fill=255)
# bagliore
alone = img.filter(ImageFilter.GaussianBlur(14))
fin = Image.eval(Image.merge('L', [img]), lambda v: v)
from PIL import ImageChops
fin = ImageChops.add(img, Image.eval(alone, lambda v: int(v * 0.7)))
fin.convert('RGB').save('T_CerchioRune.png')
fin.resize((768, 768)).save('prev_rune.png')
