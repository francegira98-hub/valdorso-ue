# Valdorso - La cascata della valle (v0.1.3): i pezzi che il plugin Water non fa. Scritto da Claude il 07/10/2026.
# Legge Acqua_3000.json (fatto da acqua.py: orlo, salto, larghezza) e scrive in una cartella:
#   SM_Cascata.glb          la "tenda" d'acqua: tre veli uno dietro l'altro che seguono la caduta vera (una parabola),
#                           dall'orlo fino a mezzo metro sotto il pelo della pozza;
#   SM_CascataSchiuma.glb   il disco di schiuma dove l'acqua batte nella pozza;
#   SM_CascataArcobaleno.glb (07/10) un velo di nebbia invisibile davanti alla cascata, dove il materiale disegna
#                           l'arcobaleno quando il sole è alle spalle di chi guarda;
#   T_CascataAcqua.png      strisce d'acqua che scorrono (R strisce lunghe, G schiuma a fiocchi, B gocce), si ripete;
#   T_CascataSchiuma.png    la schiuma vista dall'alto (si ripete);
#   T_CascataNebbia.png     (07/10) 16 sbuffi di nebbia morbidi (4 x 4) per gli spruzzi e la nebbia con Niagara;
#   S_Cascata.wav           il rombo della cascata, 20 s che girano senza stacco (sintesi: nessuna licenza);
#   Cascata_3000.json       dove cade il getto (per lo script dell'editor).
# Le misure dei modelli sono in centimetri, con l'origine al centro dell'orlo, alla quota dell'ACQUA sull'orlo
# (Acqua_3000.json, cascata.orlo_cm): X avanti (dove salta l'acqua), Z in alto.
# I modelli sono simmetrici a destra e a sinistra, quindi il verso dell'asse Y del glTF non conta.
# Uso: python cascata.py <Acqua_3000.json> <cartella_di_uscita>   (serve numpy, scipy, Pillow)

import json
import math
import os
import struct
import sys

import numpy as np
from PIL import Image
from scipy import ndimage, signal
from scipy.io import wavfile

G = 981.0                 # cm/s²
VELOCITA = 260.0          # cm/s: quanto corre l'acqua quando lascia l'orlo
SOTTO_IL_PELO = 50.0      # cm: i veli entrano un poco nell'acqua della pozza, così non si vede dove finiscono


# ------------------------------------------------------------------------------------------------ glb

def scrivi_glb(percorso, posizioni, normali, uv, colori, triangoli):
    """Un .glb con una mesh sola: posizioni in cm (X avanti, Y di lato, Z in alto) -> glTF in metri, Y in alto."""
    pos = np.stack([posizioni[:, 0], posizioni[:, 2], -posizioni[:, 1]], -1).astype(np.float32) / 100.0
    nor = np.stack([normali[:, 0], normali[:, 2], -normali[:, 1]], -1).astype(np.float32)
    uvs = uv.astype(np.float32)
    col = colori.astype(np.float32)
    # (x, y, z) -> (x, z, -y) è una rotazione, non uno specchio: il giro dei triangoli resta quello giusto.
    ind = triangoli.astype(np.uint32).reshape(-1)
    pezzi, viste, accessi = [], [], []
    scarto = 0

    def aggiungi(dati, tipo, componente, quanti, bersaglio, minmax=None):
        nonlocal scarto
        b = dati.tobytes()
        viste.append({"buffer": 0, "byteOffset": scarto, "byteLength": len(b), "target": bersaglio})
        b += b"\0" * ((4 - len(b) % 4) % 4)
        pezzi.append(b)
        scarto += len(b)
        a = {"bufferView": len(viste) - 1, "componentType": componente, "count": quanti, "type": tipo}
        if minmax:
            a["min"], a["max"] = minmax
        accessi.append(a)
        return len(accessi) - 1

    ip = aggiungi(pos, "VEC3", 5126, len(pos), 34962, (pos.min(0).tolist(), pos.max(0).tolist()))
    inn = aggiungi(nor, "VEC3", 5126, len(nor), 34962)
    iuv = aggiungi(uvs, "VEC2", 5126, len(uvs), 34962)
    ic = aggiungi(col, "VEC4", 5126, len(col), 34962)
    ii = aggiungi(ind, "SCALAR", 5125, len(ind), 34963)
    binario = b"".join(pezzi)
    gltf = {
        "asset": {"version": "2.0", "generator": "Valdorso cascata.py"},
        "scene": 0, "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": os.path.splitext(os.path.basename(percorso))[0]}],
"meshes": [{"name": os.path.splitext(os.path.basename(percorso))[0], "primitives": [{"attributes": {"POSITION": ip, "NORMAL": inn, "TEXCOORD_0": iuv, "COLOR_0": ic},
                                    "indices": ii, "material": 0}]}],
        "materials": [{"name": "M_Segnaposto", "doubleSided": True}],
        "buffers": [{"byteLength": len(binario)}],
        "bufferViews": viste, "accessors": accessi,
    }
    testo = json.dumps(gltf).encode("utf-8")
    testo += b" " * ((4 - len(testo) % 4) % 4)
    with open(percorso, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(testo) + 8 + len(binario)))
        f.write(struct.pack("<II", len(testo), 0x4E4F534A) + testo)
        f.write(struct.pack("<II", len(binario), 0x004E4942) + binario)


def griglia(nu, nv):
    idx = np.arange(nu * nv).reshape(nv, nu)
    a, b, c, d = idx[:-1, :-1], idx[:-1, 1:], idx[1:, :-1], idx[1:, 1:]
    return np.concatenate([np.stack([a, c, b], -1).reshape(-1, 3), np.stack([b, c, d], -1).reshape(-1, 3)])


def normali_di(P, nu, nv):
    Q = P.reshape(nv, nu, 3)
    du = np.gradient(Q, axis=1)
    dv = np.gradient(Q, axis=0)
    n = np.cross(dv, du)
    n /= np.linalg.norm(n, axis=2, keepdims=True) + 1e-9
    return n.reshape(-1, 3)


# ------------------------------------------------------------------------------------------------ i veli

def caduta(salto, avanti=0.0, rallenta=1.0):
    """Il percorso del getto: un tratto sull'orlo che si piega, poi la parabola fino sotto il pelo della pozza.
    Restituisce punti (x, z) in cm e la lunghezza lungo il percorso."""
    punti = []
    # sull'orlo: l'acqua arriva piana (z = 0 è il suo pelo) e si abbassa appena prima del salto
    for x in np.linspace(-260.0, 0.0, 14):
        z = -3.0 * math.exp(x / 60.0)
        punti.append((x + avanti * (x + 260) / 260, z))
    v = VELOCITA * rallenta
    fine = math.sqrt(2 * (salto + SOTTO_IL_PELO - 3.0) / G)
    for t in np.linspace(0.0, fine, 70)[1:]:
        punti.append((v * t + avanti, -3.0 - 0.5 * G * t * t))
    punti = np.array(punti)
    lungo = np.concatenate([[0], np.cumsum(np.hypot(*np.diff(punti, axis=0).T))])
    return punti, lungo


def velo(salto, larghezza, indietro, stretto, trasparenza, seme):
    rng = np.random.default_rng(seme)
    percorso, lungo = caduta(salto, avanti=-indietro, rallenta=1.0 - indietro / 400.0)
    nv = len(percorso)
    nu = 33
    u = np.linspace(-1, 1, nu)
    P, UV, C = [], [], []
    onde = rng.uniform(0, 6.28, 4)
    for j, ((x, z), s) in enumerate(zip(percorso, lungo)):
        f = s / lungo[-1]
        mezzo = larghezza / 2 * stretto * (1 + 0.45 * f)                 # il getto si apre cadendo
        for k, a in enumerate(u):
            y = a * mezzo
            # pieghe di traverso: il velo non è un foglio piatto
            pieghe = (14 * math.sin(a * 5.1 + onde[0]) + 7 * math.sin(a * 11.3 + onde[1])) * (0.3 + f)
            P.append((x + pieghe - 25 * a * a * f, y, z))
            UV.append((a * mezzo / 400.0, s / 400.0))
            bordo = max(0.0, 1 - abs(a)) ** 0.6                             # sfuma ai lati
            inizio = min(1.0, s / 120.0)                                    # sfuma dove comincia, sull'orlo
            C.append((trasparenza, (seme % 3) / 2.0, f, bordo * inizio))
    P = np.array(P)
    return P, np.array(UV), np.array(C), griglia(nu, nv), nu, nv


def tenda(salto, larghezza):
    pezzi = [velo(salto, larghezza, 0.0, 1.0, 1.0, 1),         # il velo principale
             velo(salto, larghezza, 60.0, 0.8, 0.8, 2),        # dietro, più stretto: dà spessore
             velo(salto, larghezza, -45.0, 1.2, 0.45, 3)]      # davanti, più largo e leggero: la nebbia che si stacca
    P, UV, C, T, N = [], [], [], [], []
    base = 0
    for p, uv, c, t, nu, nv in pezzi:
        P.append(p); UV.append(uv); C.append(c); T.append(t + base); N.append(normali_di(p, nu, nv))
        base += len(p)
    return np.concatenate(P), np.concatenate(N), np.concatenate(UV), np.concatenate(C), np.concatenate(T)


def disco(raggio, centro_x):
    """Il disco di schiuma, appena sopra il pelo della pozza, centrato dove batte il getto."""
    nr, na = 16, 64
    r = np.linspace(0.03, 1, nr) ** 0.8          # (non da 0: al centro i triangoli sarebbero punti)
    a = np.linspace(0, 2 * math.pi, na, endpoint=False)
    R, A = np.meshgrid(r, a, indexing="ij")
    X = centro_x + R * raggio * np.cos(A) * np.where(np.cos(A) > 0, 1.2, 0.85)   # più lungo verso valle, corto verso la parete
    Y = R * raggio * np.sin(A)
    P = np.stack([X, Y, np.full_like(X, 4.0)], -1).reshape(-1, 3)
    UV = np.stack([X / 600.0, Y / 600.0], -1).reshape(-1, 2)
    forza = (1 - R) ** 1.3
    C = np.stack([forza, R, np.zeros_like(R), np.clip((1 - R) / 0.25, 0, 1)], -1).reshape(-1, 4)
    idx = np.arange(nr * na).reshape(nr, na)
    a0, a1 = idx[:-1, :], idx[:-1, np.r_[1:na, 0]]
    b0, b1 = idx[1:, :], idx[1:, np.r_[1:na, 0]]
    T = np.concatenate([np.stack([a0, b0, a1], -1).reshape(-1, 3), np.stack([a1, b0, b1], -1).reshape(-1, 3)])
    N = np.tile([0.0, 0.0, 1.0], (len(P), 1))
    return P, N, UV, C, T


# ------------------------------------------------------------------------------------------------ texture

def rumore(rng, lato, celle_x, celle_y):
    """Rumore morbido che si ripete senza cuciture (somma di onde a frequenze intere)."""
    y, x = np.mgrid[0:lato, 0:lato] / lato
    tot = np.zeros((lato, lato))
    for _ in range(40):
        fx = rng.integers(0, celle_x + 1)
        fy = rng.integers(0, celle_y + 1)
        if fx == 0 and fy == 0:
            continue
        tot += rng.uniform(0.3, 1) * np.cos(2 * math.pi * (fx * x + fy * y) + rng.uniform(0, 6.28))
    return (tot - tot.min()) / (tot.max() - tot.min())


def texture_acqua(percorso, lato=1024):
    rng = np.random.default_rng(710)
    # R: strisce lunghe (tanta frequenza di traverso, poca in lungo)
    strisce = sum(rumore(rng, lato, 24 * k, 2 * k) / k for k in (1, 2, 3))
    strisce = (strisce - strisce.min()) / (strisce.max() - strisce.min())
    strisce = np.clip((strisce - 0.35) / 0.5, 0, 1) ** 1.4
    # G: fiocchi di schiuma
    fiocchi = rumore(rng, lato, 12, 6) * 0.6 + rumore(rng, lato, 40, 20) * 0.4
    fiocchi = np.clip((fiocchi - 0.45) / 0.35, 0, 1)
    # B: gocce e fili sottili
    gocce = np.zeros((lato, lato))
    for _ in range(900):
        cx, cy = rng.integers(0, lato, 2)
        lungo = rng.integers(10, 60)
        for d in range(lungo):
            gocce[(cy + d) % lato, cx] = max(gocce[(cy + d) % lato, cx], 1 - d / lungo)
    gocce = ndimage.gaussian_filter(gocce, 0.8, mode="wrap")
    gocce = np.clip(gocce / (gocce.max() + 1e-9) * 1.6, 0, 1)
    img = np.stack([strisce, fiocchi, gocce, np.ones_like(strisce)], -1)
    Image.fromarray((img * 255).astype(np.uint8), "RGBA").save(percorso)


def texture_schiuma(percorso, lato=1024):
    rng = np.random.default_rng(711)
    a = rumore(rng, lato, 10, 10) * 0.5 + rumore(rng, lato, 30, 30) * 0.3 + rumore(rng, lato, 70, 70) * 0.2
    bolle = np.zeros((lato, lato))
    for _ in range(2500):
        cx, cy = rng.integers(0, lato, 2)
        r = rng.integers(2, 9)
        y, x = np.ogrid[-r:r + 1, -r:r + 1]
        anello = np.clip(1 - np.abs(np.hypot(x, y) - r * 0.7) / 1.5, 0, 1)
        ys, xs = (np.arange(cy - r, cy + r + 1) % lato), (np.arange(cx - r, cx + r + 1) % lato)
        bolle[np.ix_(ys, xs)] = np.maximum(bolle[np.ix_(ys, xs)], anello)
    s = np.clip((a - 0.4) / 0.4, 0, 1) * 0.75 + bolle * 0.5 * np.clip((a - 0.3) / 0.3, 0, 1)
    s = np.clip(s, 0, 1)
    s8 = (s * 255).astype(np.uint8)
    # RGBA e non in scala di grigi: Unreal la importa come texture a colori, come vuole il materiale
    Image.fromarray(np.stack([s8, s8, s8, np.full_like(s8, 255)], -1), "RGBA").save(percorso)


def texture_nebbia(percorso, lato=1024, celle=4):
    """(07/10) Gli sbuffi di nebbia per Niagara: 4 x 4 riquadri, ognuno uno sbuffo morbido e diverso, bianco,
    con la trasparenza nel canale A (bordi che svaniscono, niente cerchi netti)."""
    rng = np.random.default_rng(713)
    q = lato // celle
    img = np.zeros((lato, lato, 4))
    y, x = (np.mgrid[0:q, 0:q] + 0.5) / q * 2 - 1
    for i in range(celle * celle):
        r = np.hypot(x, y)
        nuvola = rumore(rng, q, 6, 6) * 0.6 + rumore(rng, q, 14, 14) * 0.4
        a = np.clip(1 - r / (0.75 + 0.2 * nuvola), 0, 1) ** 1.6 * (0.55 + 0.45 * nuvola)
        a = ndimage.gaussian_filter(a, 2)
        rr, cc = divmod(i, celle)
        img[rr * q:(rr + 1) * q, cc * q:(cc + 1) * q, :3] = 0.92 + 0.08 * nuvola[..., None]
        img[rr * q:(rr + 1) * q, cc * q:(cc + 1) * q, 3] = a / (a.max() + 1e-9)
    Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), "RGBA").save(percorso)


# ------------------------------------------------------------------------------------------------ suono

def suono(percorso, secondi=20.0, sr=48000):
    rng = np.random.default_rng(712)
    n = int(secondi * sr)
    bianco = rng.standard_normal(n + sr)
    # rombo: basse che respirano piano
    rombo = signal.lfilter(*signal.butter(2, [35 / (sr / 2), 180 / (sr / 2)], btype="band"), bianco)
    respiro = 1 + 0.25 * np.sin(np.arange(n + sr) / sr * 2 * math.pi * 0.13) + 0.15 * np.sin(np.arange(n + sr) / sr * 2 * math.pi * 0.31)
    # scroscio: il grosso del suono, medie
    scroscio = signal.lfilter(*signal.butter(2, [300 / (sr / 2), 3500 / (sr / 2)], btype="band"), rng.standard_normal(n + sr))
    # fruscio: spruzzi alti
    fruscio = signal.lfilter(*signal.butter(2, 4500 / (sr / 2), btype="high"), rng.standard_normal(n + sr))
    # tonfi: getti che battono nella pozza
    tonfi = np.zeros(n + sr)
    for _ in range(int(secondi * 9)):
        i = rng.integers(0, n)
        d = int(sr * rng.uniform(0.03, 0.12))
        busta = np.exp(-np.arange(d) / (d / 4))
        tonfi[i:i + d] += busta * rng.standard_normal(d) * rng.uniform(0.3, 1)
    tonfi = signal.lfilter(*signal.butter(2, [120 / (sr / 2), 1200 / (sr / 2)], btype="band"), tonfi)
    x = 1.1 * rombo / np.std(rombo) * respiro + 1.0 * scroscio / np.std(scroscio) + 0.25 * fruscio / np.std(fruscio) \
        + 0.5 * tonfi / (np.std(tonfi) + 1e-9)
    # gira senza stacco: l'ultimo secondo sfuma nel primo
    inc = sr
    x_loop = x[:n].copy()
    rampa = np.linspace(0, 1, inc)
    x_loop[:inc] = x[:inc] * rampa + x[n:n + inc] * (1 - rampa)
    x_loop = x_loop / np.max(np.abs(x_loop)) * 0.8
    stereo = np.stack([x_loop, np.roll(x_loop, int(0.013 * sr))], -1)   # un poco largo
    wavfile.write(percorso, sr, (stereo * 32767).astype(np.int16))


# ------------------------------------------------------------------------------------------------

def velo_arcobaleno(salto, batte):
    """(07/10) Un rettangolo verticale di 30 x 22 m davanti alla pozza, di traverso al getto: lì sta la nebbia.
    Colore dei vertici: A = quanta nebbia (sfuma ai bordi e in alto), il resto pieno."""
    nu, nv = 25, 19
    u = np.linspace(-1, 1, nu)
    v = np.linspace(0, 1, nv)
    U, V = np.meshgrid(u, v)
    X = np.full_like(U, batte + 800.0) - 300.0 * U * U          # appena curvo, abbraccia la pozza
    Y = U * 1500.0
    Z = -(salto + 50.0) + V * 2200.0
    P = np.stack([X, Y, Z], -1).reshape(-1, 3)
    nebbia = (1 - np.abs(U)) ** 0.8 * np.clip(V / 0.08, 0, 1) * (1 - V) ** 1.2
    C = np.stack([np.ones_like(U), np.ones_like(U), V, nebbia], -1).reshape(-1, 4)
    UV = np.stack([(U + 1) / 2, 1 - V], -1).reshape(-1, 2)
    T = griglia(nu, nv)
    return P, normali_di(P, nu, nv), UV, C, T


def main(file_acqua, uscita):
    with open(file_acqua, encoding="utf-8") as f:
        dati = json.load(f)
    c = dati["cascata"]
    salto = c["salto_cm"]
    larghezza = c["larghezza_cm"]
    os.makedirs(uscita, exist_ok=True)

    P, N, UV, C, T = tenda(salto, larghezza)
    scrivi_glb(os.path.join(uscita, "SM_Cascata.glb"), P, N, UV, C, T)
    # dove batte il getto principale (in avanti dall'orlo)
    batte = VELOCITA * math.sqrt(2 * (salto - 3.0) / G)
    P2, N2, UV2, C2, T2 = disco(500.0, batte)
    scrivi_glb(os.path.join(uscita, "SM_CascataSchiuma.glb"), P2, N2, UV2, C2, T2)
    scrivi_glb(os.path.join(uscita, "SM_CascataArcobaleno.glb"), *velo_arcobaleno(salto, batte))
    texture_acqua(os.path.join(uscita, "T_CascataAcqua.png"))
    texture_schiuma(os.path.join(uscita, "T_CascataSchiuma.png"))
    texture_nebbia(os.path.join(uscita, "T_CascataNebbia.png"))
    suono(os.path.join(uscita, "S_Cascata.wav"))
    info = {"batte_avanti_cm": round(batte, 1), "salto_cm": salto, "larghezza_cm": larghezza,
            "nota": "Origine dei modelli al centro dell'orlo, al pelo dell'acqua; la schiuma va al pelo della pozza, stessa X e Y."}
    with open(os.path.join(uscita, "Cascata_3000.json"), "w", encoding="utf-8") as f:
        json.dump(info, f, ensure_ascii=False, indent=1)
    print(f"Cascata: salto {salto / 100:.1f} m, larga {larghezza / 100:.1f} m, il getto batte {batte / 100:.1f} m avanti; "
          f"{len(T)} triangoli nei veli, {len(T2)} nella schiuma")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
