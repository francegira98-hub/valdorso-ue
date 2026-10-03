import cv2, numpy as np
im = cv2.imread('/mnt/user-data/uploads/Texture/cielo/qwantani_night_puresky_8k.hdr', cv2.IMREAD_UNCHANGED)[:, :, ::-1]  # RGB
W, H = 4096, 2048
im = cv2.resize(im, (W, H), interpolation=cv2.INTER_AREA).astype(np.float32)
im = np.roll(im, -int(0.111 * W), axis=1)
lum = im.mean(2, keepdims=True)
base = cv2.GaussianBlur(im, (0, 0), 24)
det = np.clip(im - base, 0, None)            # stelle e via lattea fine
blum = base.mean(2, keepdims=True)
# base: notte blu profonda, alone compresso
v = (np.arange(H) + 0.5) / H                 # 0 zenit, 0.5 orizzonte
elev = (0.5 - v) * np.pi                     # elevazione
blum = cv2.GaussianBlur(blum, (0, 0), 40)[:, :, None]
tono = blum / (1.0 + blum)                   # comprime l'alone
blu = np.array([0.30, 0.42, 0.85], np.float32)
caldo = np.array([0.95, 0.70, 0.50], np.float32)
alone = np.clip((blum - 0.9) / 3.0, 0, 1) * 0.8
col = tono * (blu * (1 - alone) + caldo * alone)
col *= 0.22
# stelle
dl = det.mean(2, keepdims=True)
cr = det / np.maximum(dl, 1e-6)
cr = 1.0 + (np.clip(cr, 0, 3) - 1.0) * 0.35
peso = 1.0 / (1.0 + blum * 1.5)
e0 = elev[:, None, None]
peso = peso * np.clip((e0 - np.radians(3)) / np.radians(9), 0, 1)
dl = np.where(dl > 0.06, dl, dl * 0.4)
stelle = dl * cr * 1.6 * peso
out = col + stelle
# via lattea: dettaglio medio
mid = cv2.GaussianBlur(im, (0, 0), 3) - base
out += np.clip(mid, 0, None).mean(2, keepdims=True) * peso * 0.25 * np.array([0.9, 0.85, 1.0], np.float32)
# sotto l'orizzonte: buio
e = elev[:, None, None]
sotto = np.clip(-e / 0.08, 0, 1)
out = out * (1 - sotto) + np.array([0.004, 0.006, 0.012], np.float32) * sotto
out = np.clip(out, 0, 1)
print('zenit', out[100].mean(0), 'orizz', out[int(H*0.48)].mean(0), 'max', out.max())
srgb = np.where(out <= 0.0031308, out * 12.92, 1.055 * np.power(out, 1 / 2.4) - 0.055)
cv2.imwrite('T_CieloNotte.png', (np.clip(srgb, 0, 1) * 255 + 0.5).astype(np.uint8)[:, :, ::-1])
prev = cv2.resize((np.clip(srgb * 1.0, 0, 1) * 255).astype(np.uint8)[:, :, ::-1], (2048, 1024), interpolation=cv2.INTER_AREA)
cv2.imwrite('prev_cielo.jpg', prev)
# vista dalla telecamera: banda orizzonte azimut -40..+20, elev -5..25
def az2col(a): return int(((a / 360.0) + 0.5) * W)
r0, r1 = int((0.5 - 25 / 180) * H), int((0.5 + 5 / 180) * H)
crop = srgb[r0:r1, az2col(-40):az2col(20)]
cv2.imwrite('vista.jpg', (np.clip(crop, 0, 1) * 255).astype(np.uint8)[:, :, ::-1])
