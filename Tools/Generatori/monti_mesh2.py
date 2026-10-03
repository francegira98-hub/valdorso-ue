import numpy as np, trimesh
rng = np.random.default_rng(1312)
# --- rumore di Perlin 2D ---
P = rng.permutation(512); P = np.concatenate([P, P])
G = rng.normal(size=(512, 2)); G /= np.linalg.norm(G, axis=1, keepdims=True)
def perlin(x, y):
    xi = np.floor(x).astype(np.int64); yi = np.floor(y).astype(np.int64)
    xf = x - xi; yf = y - yi
    def g(ix, iy, dx, dy):
        h = P[(P[ix & 511] + iy) & 511]
        return G[h, 0] * dx + G[h, 1] * dy
    u = xf * xf * xf * (xf * (xf * 6 - 15) + 10); v = yf * yf * yf * (yf * (yf * 6 - 15) + 10)
    n00 = g(xi, yi, xf, yf); n10 = g(xi + 1, yi, xf - 1, yf)
    n01 = g(xi, yi + 1, xf, yf - 1); n11 = g(xi + 1, yi + 1, xf - 1, yf - 1)
    return (n00 * (1 - u) + n10 * u) * (1 - v) + (n01 * (1 - u) + n11 * u) * v
def ridged(x, y, ott=9):
    tot = 0; amp = 1; freq = 1; peso = 1; norma = 0
    for o in range(ott):
        n = 1 - np.abs(perlin(x * freq + o * 17.3, y * freq - o * 9.1) * 1.4)
        n = np.clip(n, 0, 1) ** 2
        n *= peso; peso = np.clip(n * 1.6, 0, 1)
        tot += n * amp; norma += amp
        freq *= 2.03; amp *= 0.5
    return tot / norma

R0, R1, HMAX = 22000.0, 150000.0, 60000.0          # centimetri
NA, NR = 1600, 260
ang = np.linspace(0, 2 * np.pi, NA, endpoint=False)
t = (np.arange(NR) / (NR - 1)) ** 1.7
r = R0 + (R1 - R0) * t
A, T = np.meshgrid(ang, t); Rr = R0 + (R1 - R0) * T
X = Rr * np.cos(A); Y = Rr * np.sin(A)
s = 1 / 22000.0
rid = ridged(X * s, Y * s)
grande = 0.5 + 0.5 * perlin(X / 70000.0 + 3.3, Y / 70000.0 - 1.7)
env = np.clip(T / 0.12, 0, 1); env = env * env * (3 - 2 * env)
H = 0.11 * Rr * env * rid * (0.30 + 1.0 * grande) - 150.0
# --- mesh ---
V = np.stack([X, Y, H], -1).reshape(-1, 3)
idx = np.arange(NA * NR).reshape(NR, NA)
a = idx[:-1, :]; b = idx[:-1, np.r_[1:NA, 0]]; c = idx[1:, :]; d = idx[1:, np.r_[1:NA, 0]]
F = np.concatenate([np.stack([a, c, b], -1).reshape(-1, 3), np.stack([b, c, d], -1).reshape(-1, 3)])
m = trimesh.Trimesh(V, F, process=False)
# verso l'alto (z positivo)
if m.face_normals[:, 2].mean() < 0:
    F = F[:, ::-1]; m = trimesh.Trimesh(V, F, process=False)
N = m.vertex_normals
# neve: in alto, sui pendii non troppo ripidi, con un po' di rumore
hn = H.reshape(-1) / (0.11 * Rr.reshape(-1))
rumore = perlin(V[:, 0] / 3000.0, V[:, 1] / 3000.0) * 0.12
neve = np.clip((hn + rumore - 0.56) / 0.12, 0, 1) * np.clip((H.reshape(-1) - 7000) / 3000, 0, 1) * np.clip((N[:, 2] - 0.45) / 0.2, 0, 1)
print('verts', len(V), 'tris', len(F), 'Hmax', H.max(), 'neve media', neve.mean())
np.save('monti_H.npy', H)
# --- glTF: Y in alto, metri ---
pos = np.stack([V[:, 0], V[:, 2], -V[:, 1]], -1) / 100.0
nor = np.stack([N[:, 0], N[:, 2], -N[:, 1]], -1)
uv = np.stack([V[:, 0], V[:, 1]], -1) / 2000.0
# R = neve, G = bosco (in basso e sui pendii dolci, a chiazze), B = variazione della roccia
macchie = perlin(V[:, 0] / 6000.0 + 7.7, V[:, 1] / 6000.0 - 3.1)
bosco = np.clip((0.42 - hn + macchie * 0.25) / 0.12, 0, 1) * np.clip((N[:, 2] - 0.62) / 0.15, 0, 1)
bosco *= np.clip((V[:, 2] + 100) / 400, 0, 1) * (1 - neve)
var = np.clip(0.5 + perlin(V[:, 0] / 9000.0 - 2.2, V[:, 1] / 9000.0 + 5.4) * 0.9 + perlin(V[:, 0] / 2500.0, V[:, 1] / 2500.0) * 0.4, 0, 1)
print('bosco medio', bosco.mean())
col = np.stack([neve, bosco, var, np.ones_like(neve)], -1)
np.save('monti_col.npy', col[:, :3].reshape(NR, NA, 3))
mesh = trimesh.Trimesh(pos, F[:, ::-1], vertex_normals=nor, process=False)  # lo scambio di assi inverte l'orientamento
mesh.visual = trimesh.visual.TextureVisuals(uv=uv)
mesh.visual.vertex_attributes = {}
import pygltflib, struct
# scrittura manuale del glb con COLOR_0
def blob(arr): return arr.astype(np.float32).tobytes()
buf = b''; views = []; accs = []
def add(data, tipo, comp, cnt, target, minmax=None):
    global buf
    off = len(buf); buf += data
    while len(buf) % 4: buf += b'\0'
    views.append(pygltflib.BufferView(buffer=0, byteOffset=off, byteLength=len(data), target=target))
    acc = pygltflib.Accessor(bufferView=len(views) - 1, componentType=comp, count=cnt, type=tipo)
    if minmax: acc.min, acc.max = minmax
    accs.append(acc); return len(accs) - 1
FL = pygltflib.FLOAT; AB = pygltflib.ARRAY_BUFFER
ip = add(blob(pos), 'VEC3', FL, len(pos), AB, (pos.min(0).tolist(), pos.max(0).tolist()))
inn = add(blob(nor), 'VEC3', FL, len(nor), AB)
iuv = add(blob(uv), 'VEC2', FL, len(uv), AB)
ic = add(blob(col), 'VEC4', FL, len(col), AB)
tri = F.astype(np.uint32)  # (x,y,z)->(x,z,-y) e una rotazione: l orientamento resta
ii = add(tri.tobytes(), 'SCALAR', pygltflib.UNSIGNED_INT, tri.size, pygltflib.ELEMENT_ARRAY_BUFFER)
g = pygltflib.GLTF2(
    scene=0, scenes=[pygltflib.Scene(nodes=[0])], nodes=[pygltflib.Node(mesh=0, name='SM_MontiValdorso2')],
    meshes=[pygltflib.Mesh(name='SM_MontiValdorso2', primitives=[pygltflib.Primitive(
        attributes=pygltflib.Attributes(POSITION=ip, NORMAL=inn, TEXCOORD_0=iuv, COLOR_0=ic), indices=ii, material=0)])],
    materials=[pygltflib.Material(name='M_Montagna')],
    accessors=accs, bufferViews=views, buffers=[pygltflib.Buffer(byteLength=len(buf))])
g.set_binary_blob(buf)
g.save_binary('SM_MontiValdorso2.glb')
