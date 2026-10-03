import math, random
from PIL import Image, ImageDraw, ImageFilter
W, H = 2048, 1024
S = 3
im = Image.new('L', (W * S, H * S), 0)
d = ImageDraw.Draw(im)
pts = [(390,420),(480,360),(620,330),(800,318),(950,296),(1080,236),(1200,212),(1290,226),(1360,266),(1420,326),
(1480,386),(1560,404),(1622,388),(1648,360),(1682,366),(1694,396),(1730,414),(1810,452),(1885,500),(1932,522),(1948,552),(1918,576),(1840,582),(1760,602),(1690,632),(1620,644),
(1560,652),(1505,700),(1485,780),(1492,900),(1518,948),(1532,976),(1398,978),(1388,940),(1368,800),(1330,762),
(1250,772),(1182,792),(1150,862),(1160,950),(1182,978),(1058,978),(1048,930),(1038,822),
(900,804),(760,804),(662,782),
(642,852),(612,940),(624,978),(500,978),(504,930),(520,822),
(470,762),(432,802),(404,900),(414,978),(290,978),(300,930),(328,800),(318,680),(298,580),(306,480)]
P = [(x * S, y * S) for x, y in pts]
d.polygon(P, fill=255)
random.seed(5)
# pelo: ciuffi a punta verso l'esterno lungo tutto il bordo (tranne i piedi)
base = im.copy()
def dentro(x, y):
    return base.getpixel((int(x * S), int(y * S))) > 0 if 0 <= x < W and 0 <= y < H else False
for i in range(len(pts)):
    (x0, y0), (x1, y1) = pts[i], pts[(i + 1) % len(pts)]
    L = math.hypot(x1 - x0, y1 - y0); n = int(L / 7)
    tx, ty = (x1 - x0) / L, (y1 - y0) / L
    nx, ny = ty, -tx
    if dentro(x0 + tx * L / 2 + nx * 6, y0 + ty * L / 2 + ny * 6):
        nx, ny = -nx, -ny
    for k in range(n):
        t = (k + random.random()) / n; x = x0 + (x1 - x0) * t; y = y0 + (y1 - y0) * t
        if y > 930: continue
        lung = random.uniform(10, 26) if y < 650 else random.uniform(5, 12)
        larg = random.uniform(5, 10)
        pie = (x - nx * 3, y - ny * 3)
        punta = (x + nx * lung + tx * random.uniform(-10, 4), y + ny * lung + ty * random.uniform(-10, 4))
        d.polygon([((pie[0] - tx * larg) * S, (pie[1] - ty * larg) * S), (punta[0] * S, punta[1] * S), ((pie[0] + tx * larg) * S, (pie[1] + ty * larg) * S)], fill=255)
# occhio (un punto di brace, lasciato vuoto nella maschera: lo accende il materiale)
im = im.resize((W, H), Image.LANCZOS).filter(ImageFilter.GaussianBlur(2))
rgba = Image.merge('RGBA', [Image.new('L', (W, H), 255)] * 3 + [im])
rgba.save('T_OmbraOrso.png')
a = Image.new('RGB', (W, H), (90, 100, 120)); a.paste((10, 10, 14), (0, 0), im)
a.resize((1024, 512)).save('prev_orso.png')
