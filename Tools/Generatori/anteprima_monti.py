import numpy as np, cv2
from scipy.ndimage import map_coordinates
H = np.load('monti_H.npy'); NR, NA = H.shape
C = np.load('monti_col.npy')
R0, R1 = 22000.0, 150000.0
def altezza(x, y):
    r = np.hypot(x, y); th = np.mod(np.arctan2(y, x), 2*np.pi)
    t = np.clip((r - R0) / (R1 - R0), 0, 1); i = (t ** (1/1.7)) * (NR - 1)
    j = th / (2*np.pi) * NA
    h = map_coordinates(np.concatenate([H, H[:, :1]], 1), [i, j], order=1, mode='nearest')
    return np.where(r < R0, -150.0, h)
def colore(x, y):
    r = np.hypot(x, y); th = np.mod(np.arctan2(y, x), 2*np.pi)
    t = np.clip((r - R0) / (R1 - R0), 0, 1); i = (t ** (1/1.7)) * (NR - 1); j = th / (2*np.pi) * NA
    out = [map_coordinates(np.concatenate([C[..., k], C[:, :1, k]], 1), [i, j], order=1, mode='nearest') for k in range(3)]
    return np.stack(out, -1)
W, Hs = 1600, 450
cam = np.array([-980.0, -120.0, 210.0]); yaw0 = np.degrees(np.arctan2(-330 - cam[1], 0 - cam[0]))
fov = 50; f = (W / 2) / np.tan(np.radians(fov / 2))
img = np.zeros((Hs, W, 3)); img[:] = (0.02, 0.03, 0.06)
luce = np.array([np.cos(np.radians(5)) * np.cos(np.radians(24)), np.sin(np.radians(5)) * np.cos(np.radians(24)), np.sin(np.radians(24))])
d = np.geomspace(200, 160000, 1500)
for c in range(W):
    a = np.radians(yaw0) + np.arctan((c - W / 2) / f)
    x = cam[0] + d * np.cos(a); y = cam[1] + d * np.sin(a)
    h = altezza(x, y)
    hx = (altezza(x + 50, y) - altezza(x - 50, y)) / 100; hy = (altezza(x, y + 50) - altezza(x, y - 50)) / 100
    n = np.stack([-hx, -hy, np.ones_like(hx)], -1); n /= np.linalg.norm(n, axis=1, keepdims=True)
    sh = np.clip(n @ luce, 0, 1) * 0.5 + 0.08
    cc = colore(x, y)
    roccia = np.array([0.12,0.11,0.10]) + (np.array([0.20,0.19,0.17]) - np.array([0.12,0.11,0.10])) * cc[:, 2:3]
    base = roccia * (1 - cc[:, 1:2]) + np.array([0.05,0.08,0.04]) * cc[:, 1:2]
    base = base * (1 - cc[:, 0:1]) + np.array([0.45,0.47,0.52]) * cc[:, 0:1]
    sy = Hs * 0.55 - (h - cam[2]) / d * f
    fog = 1 - np.exp(-d / 60000)
    ymax = Hs
    for k in range(len(d)):
        top = int(max(sy[k], 0))
        if top < ymax:
            col = base[k] * sh[k] * 3 * np.where(d[k] < 22000, 0.6, 1.0)
            col = col * (1 - fog[k]) + np.array([0.05, 0.06, 0.10]) * fog[k]
            img[top:ymax, c] = col; ymax = top
cv2.imwrite('anteprima_monti.png', (np.clip(img, 0, 1) ** (1 / 2.2) * 255).astype(np.uint8)[:, :, ::-1])
