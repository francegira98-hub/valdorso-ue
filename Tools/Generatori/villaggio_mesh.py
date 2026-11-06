# Valdorso - I mattoni per costruire il villaggio (v0.1.3). Scritto da Claude il 05/11/2026.
# Una "Mesh" raccoglie triangoli divisi per materiale (pietra, intonaco, assi, ...) e si scrive in un .glb che Unreal
# importa. Misure in cm, assi come in Unreal: X avanti (est), Y a destra (sud), Z in alto. Le UV sono in "piastrelle":
# una texture copre TILE[materiale] cm, così le pietre hanno la stessa misura su ogni muro.
# I materiali in Unreal sono a due facce (crea_villaggio.py): il verso dei triangoli non conta, le normali sì.

import json
import math
import os
import struct

import numpy as np

TILE = {
    "Pietra": 200.0, "Intonaco": 200.0, "Assi": 200.0, "Trave": 100.0, "Coppi": 200.0, "Paglia": 200.0,
    "Ciottoli": 200.0, "Terra": 400.0, "Vetro": 100.0, "Ferro": 100.0, "Acqua": 400.0, "Fango": 400.0,
    "Canna": 100.0, "Verde": 100.0, "Fiamma": 100.0, "Stoffa": 200.0,
    "Roccia": 400.0, "Muschio": 100.0, "Runa": 100.0,
}


def v3(x, y, z):
    return np.array([x, y, z], dtype=np.float64)


def normalizza(v):
    l = np.linalg.norm(v)
    return v / l if l > 1e-9 else v


def ruota_z(p, gradi):
    a = math.radians(gradi)
    c, s = math.cos(a), math.sin(a)
    return np.array([p[0] * c - p[1] * s, p[0] * s + p[1] * c, p[2]])


class Mesh:
    def __init__(self, nome):
        self.nome = nome
        self.parti = {}     # materiale -> [posizioni, normali, uv, colori, indici]

    def _parte(self, mat):
        if mat not in self.parti:
            self.parti[mat] = [[], [], [], [], []]
        return self.parti[mat]

    def triangoli(self, mat, P, N, UV, T, C=None):
        parte = self._parte(mat)
        base = sum(len(p) for p in parte[0])
        parte[0].append(np.asarray(P, np.float64))
        parte[1].append(np.asarray(N, np.float64))
        parte[2].append(np.asarray(UV, np.float64))
        if C is None:
            C = np.ones((len(P), 4))
        parte[3].append(np.asarray(C, np.float64))
        parte[4].append(np.asarray(T, np.int64) + base)

    def quad(self, mat, a, b, c, d, uv=None, normale=None, colore=None):
        """Quadrilatero a-b-c-d (in giro). UV: se non date, planari sul piano del quad (u lungo a->b, v giù)."""
        a, b, c, d = (np.asarray(x, np.float64) for x in (a, b, c, d))
        n = normale if normale is not None else normalizza(np.cross(b - a, d - a))
        if uv is None:
            uv = self.uv_piano(mat, [a, b, c, d], b - a, n)
        C = None if colore is None else [colore] * 4
        self.triangoli(mat, [a, b, c, d], [n] * 4, uv, [[0, 1, 2], [0, 2, 3]], C)

    def poligono(self, mat, punti, asse_u=None, colore=None):
        """Poligono convesso piatto (a ventaglio)."""
        P = [np.asarray(p, np.float64) for p in punti]
        n = normalizza(np.cross(P[1] - P[0], P[2] - P[0]))
        au = asse_u if asse_u is not None else P[1] - P[0]
        uv = self.uv_piano(mat, P, au, n)
        T = [[0, i, i + 1] for i in range(1, len(P) - 1)]
        C = None if colore is None else [colore] * len(P)
        self.triangoli(mat, P, [n] * len(P), uv, T, C)

    @staticmethod
    def uv_piano(mat, punti, asse_u, n):
        t = TILE.get(mat, 200.0)
        u = normalizza(np.asarray(asse_u, np.float64))
        # v: verso il basso del mondo se il piano è in piedi, altrimenti perpendicolare a u nel piano
        w = np.cross(n, u)
        if w[2] > 0:
            w = -w
        return [[np.dot(p, u) / t, np.dot(p, w) / t] for p in punti]

    def scatola(self, mat, centro, ax, ay, az, uv_assi=None):
        """Parallelepipedo: centro e tre semi-assi (vettori). mat può essere un dict per faccia:
        '+z', '-z', '+x', '-x', '+y', '-y' (o 'altri'). Le facce ±z hanno u lungo ax e v lungo -ay (ay = 'su')."""
        c = np.asarray(centro, np.float64)
        A = [np.asarray(ax, np.float64), np.asarray(ay, np.float64), np.asarray(az, np.float64)]
        facce = [
            ("+z", A[2], A[0], A[1]), ("-z", -A[2], A[0], A[1]),
            ("+x", A[0], A[1], A[2]), ("-x", -A[0], A[1], A[2]),
            ("+y", A[1], A[0], A[2]), ("-y", -A[1], A[0], A[2]),
        ]
        for chiave, nvec, uvec, vvec in facce:
            m = mat.get(chiave, mat.get("altri")) if isinstance(mat, dict) else mat
            if m is None:
                continue
            f = c + nvec
            p = [f - uvec - vvec, f + uvec - vvec, f + uvec + vvec, f - uvec + vvec]
            n = normalizza(nvec)
            t = TILE.get(m, 200.0)
            uu = normalizza(uvec)
            vv = normalizza(vvec)
            # v cresce "in giù" lungo vvec: la cima della texture sta dalla parte di +vvec
            uv = [[np.dot(q, uu) / t, -np.dot(q, vv) / t] for q in p]
            self.quad(m, *p, uv=uv, normale=n)

    def blocco(self, mat, x0, x1, y0, y1, z0, z1):
        """Scatola allineata agli assi, dai minimi ai massimi."""
        self.scatola(mat, [(x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2],
                     [(x1 - x0) / 2, 0, 0], [0, (y1 - y0) / 2, 0], [0, 0, (z1 - z0) / 2])

    def trave(self, mat, a, b, larghezza, altezza=None, su=(0, 0, 1)):
        """Trave da a a b con sezione larghezza x altezza."""
        a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
        altezza = altezza or larghezza
        d = b - a
        l = np.linalg.norm(d)
        if l < 1e-6:
            return
        u = d / l
        s = np.asarray(su, np.float64)
        if abs(np.dot(s, u)) > 0.95:
            s = np.array([1.0, 0, 0]) if abs(u[0]) < 0.9 else np.array([0, 1.0, 0])
        lato = normalizza(np.cross(s, u))
        alto = normalizza(np.cross(u, lato))
        self.scatola(mat, (a + b) / 2, u * l / 2, lato * larghezza / 2, alto * altezza / 2)

    def cilindro(self, mat, base, raggio, altezza, lati=8, tappo=True, raggio_alto=None, punta=0.0, asse=(0, 0, 1)):
        """Cilindro (o tronco di cono) lungo 'asse'; con punta > 0 finisce a punta (pali della palizzata)."""
        b = np.asarray(base, np.float64)
        ax = normalizza(np.asarray(asse, np.float64))
        e1 = normalizza(np.cross(ax, [0, 0, 1] if abs(ax[2]) < 0.9 else [1, 0, 0]))
        e2 = np.cross(ax, e1)
        r2 = raggio if raggio_alto is None else raggio_alto
        t = TILE.get(mat, 100.0)
        P, N, UV, T = [], [], [], []
        circ = 2 * math.pi * raggio
        for i in range(lati + 1):
            a = 2 * math.pi * i / lati
            d = e1 * math.cos(a) + e2 * math.sin(a)
            P += [b + d * raggio, b + d * r2 + ax * altezza]
            N += [d, d]
            UV += [[circ * i / lati / t, 0], [circ * i / lati / t, -altezza / t]]
        for i in range(lati):
            k = 2 * i
            T += [[k, k + 2, k + 1], [k + 1, k + 2, k + 3]]
        self.triangoli(mat, P, N, UV, T)
        cima = b + ax * altezza
        if punta > 0:
            P, N, UV, T = [], [], [], []
            for i in range(lati + 1):
                a = 2 * math.pi * i / lati
                d = e1 * math.cos(a) + e2 * math.sin(a)
                P += [cima + d * r2, cima + ax * punta]
                n = normalizza(d * punta + ax * r2)
                N += [n, n]
                UV += [[circ * i / lati / t, 0], [circ * i / lati / t, -punta / t]]
            for i in range(lati):
                k = 2 * i
                T += [[k, k + 2, k + 1]]
            self.triangoli(mat, P, N, UV, T)
        elif tappo:
            for centro, r, n in ((cima, r2, ax), (b, raggio, -ax)):
                P = [centro] + [centro + (e1 * math.cos(2 * math.pi * i / lati) + e2 * math.sin(2 * math.pi * i / lati)) * r for i in range(lati)]
                UV = [[np.dot(p, e1) / t, np.dot(p, e2) / t] for p in P]
                T = [[0, i + 1, (i + 1) % lati + 1] for i in range(lati)]
                self.triangoli(mat, P, [n] * len(P), UV, T)

    def piramide(self, mat, centro_base, mezzo_x, mezzo_y, altezza, sporgenza=0.0):
        """Tetto a piramide su base rettangolare (torri, pozzo). V scende lungo la falda."""
        c = np.asarray(centro_base, np.float64)
        hx, hy = mezzo_x + sporgenza, mezzo_y + sporgenza
        cima = c + [0, 0, altezza]
        angoli = [c + [-hx, -hy, 0], c + [hx, -hy, 0], c + [hx, hy, 0], c + [-hx, hy, 0]]
        t = TILE.get(mat, 200.0)
        for i in range(4):
            a, b = angoli[i], angoli[(i + 1) % 4]
            n = normalizza(np.cross(b - a, cima - a))
            if n[2] < 0:
                n = -n
            m = (a + b) / 2
            lung = np.linalg.norm(cima - m)
            u = normalizza(b - a)
            P = [a, b, cima]
            UV = [[np.dot(a - m, u) / t, lung / t], [np.dot(b - m, u) / t, lung / t], [0, 0]]
            self.triangoli(mat, P, [n] * 3, UV, [[0, 1, 2]])
            self.triangoli("Assi", [a, b, cima], [-n] * 3, UV, [[0, 2, 1]])

    def muro(self, mat, origine, direzione_gradi, lunghezza, altezza, spessore, buchi=(), mat_cima=None):
        """Muro dritto: parte da 'origine' (a terra, sulla linea di mezzo) e va lungo la direzione (gradi, 0 = +X).
        buchi: lista di (u0, u1, z0, z1) in cm dall'inizio del muro e da terra. Si divide in blocchi intorno ai buchi."""
        o = np.asarray(origine, np.float64)
        a = math.radians(direzione_gradi)
        u = np.array([math.cos(a), math.sin(a), 0.0])
        nrm = np.array([-u[1], u[0], 0.0])
        us = sorted(set([0.0, lunghezza] + [b[0] for b in buchi] + [b[1] for b in buchi]))
        zs = sorted(set([0.0, altezza] + [b[2] for b in buchi] + [b[3] for b in buchi]))
        for i in range(len(us) - 1):
            for j in range(len(zs) - 1):
                cu, cz = (us[i] + us[i + 1]) / 2, (zs[j] + zs[j + 1]) / 2
                if any(b[0] <= cu <= b[1] and b[2] <= cz <= b[3] for b in buchi):
                    continue
                if us[i + 1] - us[i] < 0.5 or zs[j + 1] - zs[j] < 0.5:
                    continue
                c = o + u * cu + np.array([0, 0, cz])
                m = mat
                if mat_cima and j == len(zs) - 2:
                    m = {"altri": mat, "+z": mat_cima}
                self.scatola(m, c, u * (us[i + 1] - us[i]) / 2, nrm * spessore / 2, [0, 0, (zs[j + 1] - zs[j]) / 2])

    def lastra(self, mat, x0, x1, y0, y1, z0, z1, buchi=()):
        """Pavimento/solaio orizzontale con buchi rettangolari (x0, x1, y0, y1)."""
        xs = sorted(set([x0, x1] + [b[0] for b in buchi] + [b[1] for b in buchi]))
        ys = sorted(set([y0, y1] + [b[2] for b in buchi] + [b[3] for b in buchi]))
        for i in range(len(xs) - 1):
            for j in range(len(ys) - 1):
                cx, cy = (xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2
                if any(b[0] <= cx <= b[1] and b[2] <= cy <= b[3] for b in buchi):
                    continue
                self.blocco(mat, xs[i], xs[i + 1], ys[j], ys[j + 1], z0, z1)

    def unisci(self, altra, spostamento=(0, 0, 0), gradi=0.0):
        """Aggiunge un'altra mesh, girata di 'gradi' intorno a Z e spostata."""
        for mat, parte in altra.parti.items():
            inizio = 0
            for P, N, UV, C, T in zip(*[parte[k] for k in range(5)]):
                P2 = np.array([ruota_z(p, gradi) for p in P]) + np.asarray(spostamento, np.float64)
                N2 = np.array([ruota_z(n, gradi) for n in N])
                self.triangoli(mat, P2, N2, UV, np.asarray(T) - inizio, C)
                inizio += len(P)

    def conta(self):
        return sum(sum(len(t) for t in p[4]) for p in self.parti.values())

    def scatola_limiti(self):
        tutti = np.concatenate([np.concatenate(p[0]) for p in self.parti.values()])
        return tutti.min(0), tutti.max(0)

    # ------------------------------------------------------------------------------------------------ glb

    def scrivi(self, percorso):
        """Un .glb: un nodo, una mesh, una primitiva per materiale (il nome del materiale = lo slot in Unreal).
        Posizioni in cm (Unreal) -> glTF in metri, Y in alto: (x, y, z) -> (x, z, -y), come cascata.py."""
        pezzi, viste, accessi, primitive, materiali = [], [], [], [], []
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

        for mat, parte in sorted(self.parti.items()):
            P = np.concatenate(parte[0])
            N = np.concatenate(parte[1])
            UV = np.concatenate(parte[2])
            C = np.concatenate(parte[3])
            T = np.concatenate(parte[4]).reshape(-1, 3).copy()
            # (05/11) il giro di ogni triangolo deve andare d'accordo con la sua normale (antiorario visto dalla parte
            # della normale): altrimenti Unreal lo prende per il retro e con il materiale a due facce lo illumina
            # da sotto (al primo lancio piazza e metà delle facce erano nere).
            a, b, c = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]]
            geo = np.cross(b - a, c - a)
            media = N[T[:, 0]] + N[T[:, 1]] + N[T[:, 2]]
            storti = np.einsum("ij,ij->i", geo, media) < 0
            T[storti] = T[storti][:, [0, 2, 1]]
            T = T.reshape(-1)
            pos = (np.stack([P[:, 0], P[:, 2], -P[:, 1]], -1) / 100.0).astype(np.float32)
            nor = np.stack([N[:, 0], N[:, 2], -N[:, 1]], -1)
            nor /= (np.linalg.norm(nor, axis=1, keepdims=True) + 1e-9)
            nor = nor.astype(np.float32)
            ip = aggiungi(pos, "VEC3", 5126, len(pos), 34962, (pos.min(0).tolist(), pos.max(0).tolist()))
            inn = aggiungi(nor, "VEC3", 5126, len(nor), 34962)
            iuv = aggiungi(UV.astype(np.float32), "VEC2", 5126, len(UV), 34962)
            ic = aggiungi(C.astype(np.float32), "VEC4", 5126, len(C), 34962)
            ii = aggiungi(T.astype(np.uint32), "SCALAR", 5125, len(T), 34963)
            materiali.append({"name": "MV_" + mat, "doubleSided": True})
            primitive.append({"attributes": {"POSITION": ip, "NORMAL": inn, "TEXCOORD_0": iuv, "COLOR_0": ic},
                              "indices": ii, "material": len(materiali) - 1})
        binario = b"".join(pezzi)
        gltf = {
            "asset": {"version": "2.0", "generator": "Valdorso villaggio.py"},
            "scene": 0, "scenes": [{"nodes": [0]}],
            "nodes": [{"mesh": 0, "name": self.nome}],
            "meshes": [{"name": self.nome, "primitives": primitive}],
            "materials": materiali,
            "buffers": [{"byteLength": len(binario)}],
            "bufferViews": viste, "accessors": accessi,
        }
        testo = json.dumps(gltf).encode("utf-8")
        testo += b" " * ((4 - len(testo) % 4) % 4)
        with open(percorso, "wb") as f:
            f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(testo) + 8 + len(binario)))
            f.write(struct.pack("<II", len(testo), 0x4E4F534A) + testo)
            f.write(struct.pack("<II", len(binario), 0x004E4942) + binario)
