# Valdorso - Le scale di fuori per le porte alte (17/11/2026, scritto da Claude; v0.2.7 "Il tempio").
#
# Il collaudo dei passaggi (Content/Python/nuovi/collauda_passaggi.py) ha trovato tre porte dove, fuori dalla soglia, il
# pavimento finisce e si cade di 3-4 metri: Mulino, Casermetta_della_Guardia, Torre_del_passo. Sono edifici nostri
# (villaggio.py) messi su terreno in pendenza: il pavimento sta sopra il punto piu' alto del terreno, la porta guarda
# a valle. Qui si fa, per ognuna, una scala di legno: un ballatoio davanti alla porta e una rampa lungo la facciata,
# che scende fino al terreno vero (Altezze_3000.png), con i pali fino a terra e il parapetto.
#
# Uso:  python scale_esterne.py <cartella valle (Altezze_3000.png)> <collaudo_passaggi.json> <uscita>
# Scrive in <uscita>:  glb/SM_Scala_Fuori_<nome>.glb  e  Valdorso_scale_esterne.json  (li legge crea_mura.py, "Mura: 8").
#
# Il sistema di ogni scala (cm, come lo vede Unreal): origine sul bordo del pavimento davanti alla porta, al centro, alla
# quota del pavimento; +X verso fuori, +Y lungo la facciata (X verso Y come lo yaw di Unreal), Z in alto.
# I .glb escono con gli assi dei pezzi della chat 3D (scala Y = +1 in crea_mura.py), NON con quelli del villaggio:
# per questo dentro Mesh la Y si mette con il segno cambiato.

import json
import math
import os
import sys

import numpy as np
from PIL import Image

from villaggio_mesh import Mesh

Image.MAX_IMAGE_PIXELS = None
QUOTA_MAX = 80000.0
LATO_CM = 300000.0

ALZATA = 18.0
PEDATA = 28.0
BALLATOIO_X = 170.0        # quanto esce dal muro (il collaudo cammina 1,5 m oltre la soglia: il parapetto resta piu' in la')
BALLATOIO_Y = 90.0         # mezza larghezza
RAMPA_X0, RAMPA_X1 = 15.0, 145.0   # la rampa e' larga 130
PARAPETTO = 95.0

# porta -> (nome del pezzo, da che parte scende la rampa: +1 = verso +Y, -1 = verso -Y, 0 = dove serve meno scala)
# (17/11) Casermetta: a -Y ci sono due casse e una botte sotto la rampa; Torre: a -Y c'e' il fuoco del campo.
PORTE = {
    "Mulino porta": ("Mulino", 0),
    "Casermetta_della_Guardia porta": ("Casermetta", +1),
    "Torre_del_passo porta": ("TorrePasso", +1),
}


class Terreno:
    def __init__(self, cartella):
        self.h = np.array(Image.open(os.path.join(cartella, "Altezze_3000.png"))).astype(np.float64) / 65535.0 * QUOTA_MAX
        self.S = LATO_CM / (self.h.shape[0] - 1)

    def quota(self, x, y):
        c, r = x / self.S, y / self.S
        c0, r0 = int(math.floor(c)), int(math.floor(r))
        fc, fr = c - c0, r - r0
        h = self.h
        return (h[r0, c0] * (1 - fc) * (1 - fr) + h[r0, c0 + 1] * fc * (1 - fr) + h[r0 + 1, c0] * (1 - fc) * fr
                + h[r0 + 1, c0 + 1] * fc * fr)


def blocco(m, mat, x0, x1, y0, y1, z0, z1):
    """Come Mesh.blocco, ma nel sistema della scala (la Y cambia segno dentro Mesh: vedi in cima)."""
    m.blocco(mat, min(x0, x1), max(x0, x1), min(-y0, -y1), max(-y0, -y1), min(z0, z1), max(z0, z1))


def trave(m, mat, a, b, lato):
    m.trave(mat, [a[0], -a[1], a[2]], [b[0], -b[1], b[2]], lato, lato)


def conta_gradini(terra, verso):
    """Quanti gradini servono scendendo verso 'verso' (+1 o -1): l'ultimo ha il piano a non piu' di ALZATA sopra il
    terreno nel punto dove si mette il piede dopo (30 cm oltre l'ultimo gradino)."""
    for n in range(1, 60):
        y_dopo = verso * (BALLATOIO_Y + n * PEDATA + 30.0)
        # (al centro della rampa: dove il terreno pende di traverso, dalla parte alta l'ultimo gradino entra nel terreno)
        if -ALZATA * n - terra((RAMPA_X0 + RAMPA_X1) / 2, y_dopo) <= ALZATA:
            return n
    return None


def fai_scala(nome, terra, verso):
    """terra(x, y) = quota del terreno nel sistema della scala (0 = pavimento). Ritorna (mesh, dati)."""
    if verso == 0:
        prove = {v: conta_gradini(terra, v) for v in (1, -1)}
        verso = min((v for v in prove if prove[v]), key=lambda v: prove[v], default=1)
    n = conta_gradini(terra, verso)
    if n is None:
        raise RuntimeError("%s: il terreno e' troppo in basso anche con 59 gradini" % nome)
    m = Mesh("SM_Scala_Fuori_" + nome)
    L = "legno"
    # ---- il ballatoio: tavolato, travi sotto, quattro pali fino a terra
    blocco(m, L, 0.0, BALLATOIO_X, -BALLATOIO_Y, BALLATOIO_Y, -8.0, 0.0)
    for y in (-BALLATOIO_Y + 8.0, 0.0, BALLATOIO_Y - 8.0):
        blocco(m, L, 0.0, BALLATOIO_X, y - 7.0, y + 7.0, -24.0, -8.0)
    for x in (10.0, BALLATOIO_X - 10.0):
        for y in (-BALLATOIO_Y + 10.0, BALLATOIO_Y - 10.0):
            blocco(m, L, x - 8.0, x + 8.0, y - 8.0, y + 8.0, terra(x, y) - 60.0, -8.0)
    # ---- la rampa: ogni gradino un blocco pieno che arriva 36 cm sotto il suo piano (cosi' sotto non si vede il vuoto)
    for k in range(1, n + 1):
        ya = verso * (BALLATOIO_Y + (k - 1) * PEDATA)
        yb = verso * (BALLATOIO_Y + k * PEDATA)
        blocco(m, L, RAMPA_X0, RAMPA_X1, ya, yb, -ALZATA * k - 36.0, -ALZATA * k)
    # i pali della rampa: due ogni quattro gradini, e sempre sotto l'ultimo
    for k in sorted(set(list(range(4, n + 1, 4)) + [n])):
        y = verso * (BALLATOIO_Y + (k - 0.5) * PEDATA)
        for x in (RAMPA_X0 + 7.0, RAMPA_X1 - 7.0):
            blocco(m, L, x - 7.0, x + 7.0, y - 7.0, y + 7.0, terra(x, y) - 60.0, -ALZATA * k - 36.0)
    # ---- i parapetti
    # il davanti del ballatoio e il fianco dalla parte dove la rampa non c'e'
    fianco = -verso * BALLATOIO_Y
    for a, b in (((BALLATOIO_X - 4.0, -BALLATOIO_Y, 0.0), (BALLATOIO_X - 4.0, BALLATOIO_Y, 0.0)),
                 ((0.0, fianco - verso * -4.0, 0.0), (BALLATOIO_X, fianco - verso * -4.0, 0.0))):
        trave(m, L, (a[0], a[1], PARAPETTO), (b[0], b[1], PARAPETTO), 8.0)
        trave(m, L, (a[0], a[1], PARAPETTO / 2), (b[0], b[1], PARAPETTO / 2), 6.0)
        passi = max(2, int(round(math.hypot(b[0] - a[0], b[1] - a[1]) / 60.0)))
        for i in range(passi + 1):
            t = i / passi
            x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
            blocco(m, L, x - 4.0, x + 4.0, y - 4.0, y + 4.0, 0.0, PARAPETTO)
    # la rampa: parapetto dai due lati (fuori e verso il muro: tra la rampa e il muro resta un vuoto), che segue la pendenza
    y0 = verso * BALLATOIO_Y
    y1 = verso * (BALLATOIO_Y + n * PEDATA)
    z1 = -ALZATA * n
    for x in (RAMPA_X0 + 3.0, RAMPA_X1 - 3.0):
        trave(m, L, (x, y0, PARAPETTO), (x, y1, z1 + PARAPETTO), 8.0)
        trave(m, L, (x, y0, PARAPETTO / 2), (x, y1, z1 + PARAPETTO / 2), 6.0)
        # i montanti: uno ogni due gradini e sempre uno in fondo; stanno sulla linea dell'alzata, quindi partono dal
        # gradino piu' basso dei due (l'ultimo dal piano dell'ultimo gradino)
        for k in sorted(set(list(range(0, n + 1, 2)) + [n])):
            y = verso * (BALLATOIO_Y + k * PEDATA)
            if k == n:
                y -= verso * 4.0
            zk = -ALZATA * min(k + 1, n)
            cima = PARAPETTO - ALZATA * k
            blocco(m, L, x - 4.0, x + 4.0, y - 4.0, y + 4.0, zk - 2.0, cima)
    piede = (RAMPA_X0 + RAMPA_X1) / 2, verso * (BALLATOIO_Y + n * PEDATA + 30.0)
    dati = {"pezzo": m.nome, "gradini": n, "alzata_cm": ALZATA, "pedata_cm": PEDATA, "verso_rampa": int(verso),
            "scende_cm": ALZATA * n, "ultimo_gradino_sul_terreno_cm": round(-ALZATA * n - terra(*piede), 1),
            "lunga_cm": round(n * PEDATA), "larga_cm": RAMPA_X1 - RAMPA_X0,
            "ballatoio_cm": [BALLATOIO_X, 2 * BALLATOIO_Y],
            "piede_cm": [round(piede[0], 1), round(piede[1], 1), round(terra(*piede), 1)]}
    return m, dati


def principale(valle, collaudo, uscita):
    t = Terreno(valle)
    with open(collaudo, encoding="utf-8") as f:
        porte = json.load(f)["porte"]
    os.makedirs(os.path.join(uscita, "glb"), exist_ok=True)
    scale = []
    for porta, (nome, verso) in PORTE.items():
        righe = [r for r in porte if r["porta"] == porta]
        if not righe:
            print("ATTENZIONE: nel collaudo non c'e' '%s'" % porta)
            continue
        r = righe[-1]
        bordo = None
        for nota in r["note"]:
            if "il pavimento finisce" in nota or "non c'e' pavimento" in nota:
                bordo = float(nota.split("a ")[1].split(" cm")[0])
                break
        if bordo is None or bordo <= 0:
            print("ATTENZIONE: %s: il collaudo non dice dove finisce il pavimento" % porta)
            continue
        px, py = r["pianta_cm"]
        pav = float(r["mondo_cm"][2])
        yaw = math.radians(r["passaggio_yaw"])
        dx, dy = math.cos(yaw), math.sin(yaw)
        # il bordo vero sta tra (bordo - 10) e bordo: l'origine della scala a bordo - 6 (crea_mura.py lo rimisura)
        ox, oy = px + dx * (bordo - 6.0), py + dy * (bordo - 6.0)

        def terra(x, y, ox=ox, oy=oy, dx=dx, dy=dy, pav=pav):
            return t.quota(ox + dx * x - dy * y, oy + dy * x + dx * y) - pav

        m, dati = fai_scala(nome, terra, verso)
        m.scrivi(os.path.join(uscita, "glb", m.nome + ".glb"))
        dati.update({"porta": porta, "origine_pianta_cm": [round(ox, 1), round(oy, 1), round(pav, 1)],
                     "yaw": round(r["passaggio_yaw"], 2), "bordo_dalla_porta_cm": bordo - 6.0,
                     "terreno_sotto_il_ballatoio_cm": round(terra(BALLATOIO_X / 2, 0.0), 1)})
        scale.append(dati)
        print("%s: %d gradini verso %+d (scende %.0f cm, lunga %.0f cm), ultimo gradino a %.0f cm dal terreno"
              % (porta, dati["gradini"], dati["verso_rampa"], dati["scende_cm"], dati["lunga_cm"],
                 dati["ultimo_gradino_sul_terreno_cm"]))
    with open(os.path.join(uscita, "Valdorso_scale_esterne.json"), "w", encoding="utf-8") as f:
        json.dump({"_come_si_legge": ["cm; ogni scala ha l'origine sul bordo del pavimento davanti alla porta, al centro, "
                                      "alla quota del pavimento; +X verso fuori, +Y lungo la facciata, yaw di Unreal.",
                                      "origine_pianta_cm: x, y della Pianta (mondo = Pianta + angolo del paesaggio) e z del mondo.",
                                      "Assi dei .glb come i pezzi della chat 3D (scala Y +1). Slot del materiale: legno."],
                   "scale": scale}, f, ensure_ascii=False, indent=1)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__ or "Uso: python scale_esterne.py <cartella valle> <collaudo_passaggi.json> <uscita>")
        sys.exit(1)
    principale(*sys.argv[1:4])
