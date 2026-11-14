# Valdorso - Le icone degli oggetti ad acquerello (13/11/2026, scritto da Claude; proposta approvata da Fra la notte del 13/11).
#
# Prima le icone erano solo linee chiare a penna (disegnate dal C++ in ValdorsoOggetti.cpp). Ora ogni oggetto ha la sua
# icona: lo STESSO disegno a inchiostro, colorato ad acquerello con il colore vero dell'oggetto, su un riquadro di
# pergamena (lo stile del Registro e del Taccuino). Le linee non si ricopiano a mano: icone_estrai.cpp compila il codice
# vero dei disegni e le scrive in JSON, così un oggetto nuovo nel C++ ha subito la sua icona rilanciando questo script.
#
# Scrive in <uscita>:
#   T_Icona_<Id>.png      256 x 256, con trasparenza agli angoli (una per oggetto, più T_Icona_Moneta per gli Aureus)
#   Anteprima_Icone.png   tutte insieme, per controllarle
#   icone.json            l'elenco (id, nome, colori usati)
# Le importa nell'editor Content/Python/nuovi/importa_icone.py (in /Game/UI/Icone); il gioco le usa da solo
# (SValdorsoZaino.cpp, ValdorsoDisegnoOggetti::Disegna) e, se un'icona manca, disegna le linee di prima.
#
# Uso: python icone.py <cartella Source/Valdorso> <uscita>       (serve g++; numpy e Pillow)
# Sempre uguale (semi fissi per ogni oggetto).

import json
import math
import os
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

LATO = 256                 # i pixel dell'icona
SCALA_DISEGNO = 0.72       # il disegno (da -1 a 1) occupa questa parte del mezzo lato (nel gioco: 0,62 del posto su 0,92)
CARTA = (233, 220, 186)
CARTA_SCURA = (196, 172, 128)
INCHIOSTRO = (43, 29, 18)

# --------------------------------------------------------------------------------------------- i colori degli oggetti
# Per ogni oggetto: il colore principale (la forma più grande) e il secondo (le forme piccole, o il ferro delle armi).
LEGNO = (126, 86, 48)
LEGNO_CHIARO = (168, 124, 76)
FERRO = (138, 146, 152)
FERRO_SCURO = (92, 98, 104)
OTTONE = (196, 158, 70)
ORO = (214, 172, 60)
PELLE = (122, 82, 50)
CUOIO = (150, 98, 56)
CARTA_VECCHIA = (226, 208, 160)
LANA = (110, 70, 52)
PIETRA = (140, 136, 126)
OSSO = (226, 214, 184)
CARNE = (176, 66, 58)
VERDE = (96, 128, 56)
ACQUA = (88, 132, 150)

COLORI = {
    "Pane_Nero": ((122, 78, 38), (160, 110, 60)),
    "Formaggio": ((226, 186, 88), (196, 150, 60)),
    "Mela": ((176, 44, 34), VERDE),
    "Borraccia": (PELLE, LEGNO_CHIARO),
    "Erbe_Palude": (VERDE, (150, 120, 190)),
    "Coltello": (FERRO, (200, 186, 150)),
    "Acciarino": (FERRO_SCURO, PIETRA),
    "Rami_Secchi": (LEGNO, LEGNO_CHIARO),
    "Funghi": ((150, 96, 60), OSSO),
    "Mantello": (LANA, (140, 96, 70)),
    "Mappa_Valle": (CARTA_VECCHIA, (150, 110, 70)),
    "Corda": ((176, 146, 96), (150, 120, 76)),
    "Tessera": (CARTA_VECCHIA, (170, 40, 34)),
    "Falcetto": (FERRO, LEGNO),
    "Bastone_Pastore": (LEGNO, LEGNO_CHIARO),
    "Martello": (FERRO_SCURO, LEGNO),
    "Pugnale": (FERRO, LEGNO),
    "Candeliere": (OTTONE, (250, 236, 200)),
    "Quaderno": ((120, 70, 44), CARTA_VECCHIA),
    "Medaglietta": (OTTONE, ORO),
    "Bilancia": (OTTONE, LEGNO),
    "Trappola": ((170, 140, 90), LEGNO),
    "Rete": ((176, 150, 100), (120, 150, 160)),
    "Ricordo_Anello": (ORO, (120, 160, 190)),
    "Ricordo_Lettera": (CARTA_VECCHIA, (150, 40, 34)),
    "Ricordo_Spada": (FERRO, LEGNO),
    "Ricordo_Amuleto": (LEGNO_CHIARO, (150, 60, 40)),
    "Ricordo_Mappa": (CARTA_VECCHIA, (150, 110, 70)),
    "Ricordo_Libro": ((96, 52, 40), OTTONE),
    "Spada_Corta": (FERRO, LEGNO),
    "Scudo_Legno": (LEGNO_CHIARO, FERRO_SCURO),
    "Pelle_Lupo": ((120, 112, 100), (90, 84, 76)),
    "Zanne_Lupo": (OSSO, (200, 186, 150)),
    "Mantello_Lupo": ((120, 112, 100), LANA),
    "Cozze": ((50, 56, 74), (120, 130, 150)),
    "Perla": ((236, 232, 226), (200, 196, 210)),
    "Accetta": (FERRO, LEGNO),
    "Ascia_Guerra": (FERRO, LEGNO),
    "Mazza": (FERRO_SCURO, LEGNO),
    "Clava": (LEGNO, LEGNO_CHIARO),
    "Lancia": (FERRO, LEGNO),
    "Forcone": (FERRO, LEGNO),
    "Bastone_Ferrato": (LEGNO, FERRO_SCURO),
    "Spada_Lunga": (FERRO, LEGNO),
    "Gladio": (FERRO, (150, 70, 40)),
    "Brocchiere": (FERRO, FERRO_SCURO),
    "Scudo_Rotondo": ((150, 40, 34), OTTONE),
    "Scudo_Aquilone": ((60, 80, 130), OTTONE),
    "Scudo_Legione": ((150, 40, 34), ORO),
    "Carne_Cruda": (CARNE, OSSO),
    "Pelle_Cinghiale": ((90, 70, 54), (70, 56, 44)),
    "Zanne_Cinghiale": (OSSO, (200, 186, 150)),
    "Pelle_Orso": ((88, 62, 44), (70, 50, 36)),
    "Artigli_Orso": ((50, 42, 36), OSSO),
    "Pelle_Cervo": ((168, 112, 66), (200, 160, 110)),
    "Palchi_Cervo": ((200, 180, 140), OSSO),
    "Pelle_Lepre": ((170, 150, 120), (200, 186, 160)),
    "Pelle_Volpe": ((196, 106, 50), (236, 220, 196)),
    "Carne_Cotta": ((140, 70, 40), OSSO),
    "Funghi_Arrosto": ((120, 72, 40), OSSO),
    "Stufato": ((90, 70, 60), (150, 80, 46)),
    "Zuppa_Bruno": ((90, 70, 60), (190, 140, 70)),
    "Cuoio": (CUOIO, (120, 76, 44)),
    "Mantello_Orso": ((88, 62, 44), LANA),
    "Cappuccio_Volpe": ((196, 106, 50), (236, 220, 196)),
    "Guanti_Lepre": (CUOIO, (200, 186, 160)),
    "Foglio_Incarico": (CARTA_VECCHIA, (60, 80, 130)),
    "Tessera_Carovana": (LEGNO_CHIARO, (150, 40, 34)),
    "Piccone": (FERRO_SCURO, LEGNO),
    "Moneta_Nani": ((170, 120, 70), OTTONE),
    "Pugnale_Nani": (FERRO, OTTONE),
    "Martello_Nani": (FERRO_SCURO, OTTONE),
    "Cuore_di_Pietra": (PIETRA, (120, 190, 200)),
    "Sale": ((236, 234, 228), (200, 196, 190)),
    "Spezie": ((170, 80, 40), (200, 140, 50)),
    "Stoffa_Fine": ((110, 60, 120), OTTONE),
    "Legna": (LEGNO, LEGNO_CHIARO),
    "Pietra_Grezza": (PIETRA, (170, 166, 156)),
    "Ferro_Grezzo": ((110, 80, 70), (150, 90, 60)),
    "Asse": (LEGNO_CHIARO, LEGNO),
    "Lingotto_Ferro": (FERRO, FERRO_SCURO),
    "Chiodi": (FERRO_SCURO, FERRO),
    "Pietra_Squadrata": ((170, 164, 150), PIETRA),
    "Secchio": (LEGNO, FERRO_SCURO),
    "Secchio_Acqua_Pura": (LEGNO, (110, 170, 200)),
    "Secchio_Acqua_Chiara": (LEGNO, ACQUA),
    "Secchio_Acqua_Torbida": (LEGNO, (110, 110, 70)),
    "Fischietto_di_Livia": (OTTONE, (150, 40, 34)),
    "Sasso_di_Tobia": (PIETRA, (120, 90, 60)),
    "Chiodo_Runico": (FERRO_SCURO, (110, 180, 220)),
    "Candela_della_Veglia": ((236, 220, 170), (240, 170, 60)),
    "Mappa_Gallerie": (CARTA_VECCHIA, (120, 90, 60)),
    "Taccuino_Gisella": ((70, 90, 70), CARTA_VECCHIA),
    "Lanterna_Cecco": (FERRO_SCURO, (240, 180, 70)),
    "Anello_Ranieri": (ORO, (150, 40, 34)),
    "Moneta": (ORO, (230, 196, 90)),
}
PREDEFINITO = ((150, 130, 100), (120, 100, 80))

# --------------------------------------------------------------------------------------------- il disegno estratto


def estrai(cartella_sorgenti):
    """Compila icone_estrai.cpp con la parte dei disegni di ValdorsoOggetti.cpp e restituisce il catalogo con le linee."""
    qui = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(cartella_sorgenti, "ValdorsoOggetti.cpp"), encoding="utf-8") as f:
        righe = f.read().split("\n")
    # la parte anonima: da "namespace" (la prima) fino alla graffa che la chiude, prima di "namespace ValdorsoOggetti"
    inizio = next(i for i, r in enumerate(righe) if r.strip() == "namespace")
    fine = next(i for i, r in enumerate(righe) if r.startswith("namespace ValdorsoOggetti"))
    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "oggetti_estratti.inc"), "w", encoding="utf-8") as f:
            f.write("\n".join(righe[inizio:fine]))
        exe = os.path.join(tmp, "estrai")
        subprocess.run(["g++", "-std=c++17", "-O1", "-w", "-fpermissive", "-I", tmp, "-I", qui,
                        "-include", os.path.join(qui, "icone_shim.h"), os.path.join(qui, "icone_estrai.cpp"), "-o", exe],
                       check=True)
        uscita = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    return json.loads(uscita)


# --------------------------------------------------------------------------------------------- i pennelli


def rumore(n, scala, rng, ottave=4):
    """Rumore morbido (somma di griglie casuali ingrandite), tra 0 e 1."""
    tot = np.zeros((n, n))
    peso = 1.0
    somma = 0.0
    for o in range(ottave):
        k = max(2, int(n / scala * (2 ** o)))
        g = rng.random((k, k))
        img = Image.fromarray((g * 255).astype(np.uint8)).resize((n, n), Image.BICUBIC)
        tot += np.asarray(img, np.float64) / 255.0 * peso
        somma += peso
        peso *= 0.5
    tot /= somma
    return (tot - tot.min()) / (tot.max() - tot.min() + 1e-9)


def piastrella(rng):
    """Il riquadro di pergamena: macchie leggere, bordi più scuri, angoli arrotondati (fuori trasparente)."""
    n = LATO
    y, x = np.mgrid[0:n, 0:n] / (n - 1) * 2 - 1
    macchie = rumore(n, 60, rng) * 0.6 + rumore(n, 12, rng, 2) * 0.4
    bordo = np.clip((np.maximum(abs(x), abs(y)) - 0.55) / 0.45, 0, 1) ** 1.6
    t = np.clip(0.18 * macchie + 0.55 * bordo, 0, 1)[..., None]
    rgb = np.array(CARTA) * (1 - t) + np.array(CARTA_SCURA) * t
    rgb *= (0.97 + 0.06 * rumore(n, 3, rng, 1))[..., None]          # la grana della carta
    alfa = Image.new("L", (n, n), 0)
    ImageDraw.Draw(alfa).rounded_rectangle([3, 3, n - 4, n - 4], radius=int(n * 0.09), fill=255)
    alfa = alfa.filter(ImageFilter.GaussianBlur(0.8))
    img = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).convert("RGBA")
    img.putalpha(alfa)
    # il filo del bordo, a penna
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([6, 6, n - 7, n - 7], radius=int(n * 0.08), outline=(122, 92, 54, 150), width=3)
    return img


def in_pixel(p):
    m = LATO / 2
    return (m + p[0] * m * SCALA_DISEGNO, m - p[1] * m * SCALA_DISEGNO)


def chiusa(linea):
    if len(linea) < 4:
        return False
    a, b = np.array(linea[0]), np.array(linea[-1])
    return np.linalg.norm(a - b) < 0.04


def quasi_chiusa(linea):
    """Un arco che quasi si chiude (la mela col picciolo, la borraccia col collo): si riempie come se fosse chiuso."""
    if chiusa(linea):
        return True
    if len(linea) < 8:
        return False
    p = np.array(linea)
    lato = max(np.ptp(p[:, 0]), np.ptp(p[:, 1]))
    return lato > 0.2 and np.linalg.norm(p[0] - p[-1]) < 0.45 * lato


def area(linea):
    p = np.array(linea)
    return 0.5 * abs(np.dot(p[:, 0], np.roll(p[:, 1], 1)) - np.dot(p[:, 1], np.roll(p[:, 0], 1)))


def colore_della_forma(ogg, linea, indice, principale, secondo):
    """Quale colore va a una forma chiusa. La più grande prende il principale, le piccole il secondo. Per le armi e gli
    attrezzi (si impugnano) conta l'altezza: in alto il ferro (lama, testa), in basso il manico."""
    if ogg.get("posto", 0) == 1 and ogg["id"] not in ("Clava", "Bastone_Pastore", "Bastone_Ferrato"):
        cy = float(np.mean(np.array(linea)[:, 1]))
        return principale if cy > -0.25 else secondo
    return principale if indice == 0 else secondo


def acquerello(maschera, colore, rng, intensita=0.85):
    """Una pennellata d'acquerello dentro una maschera: bordi che si sfrangiano, il pigmento che si raccoglie sul bordo,
    la grana, qualche zona più chiara. Restituisce RGBA."""
    n = LATO
    m = np.asarray(maschera, np.float64) / 255.0
    # i bordi sfrangiati: la maschera spostata dal rumore
    dx = (rumore(n, 40, rng, 3) - 0.5) * 7
    dy = (rumore(n, 40, rng, 3) - 0.5) * 7
    yy, xx = np.mgrid[0:n, 0:n]
    xs = np.clip((xx + dx).astype(int), 0, n - 1)
    ys = np.clip((yy + dy).astype(int), 0, n - 1)
    m = m[ys, xs]
    m = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(1.2)), np.float64) / 255.0
    # il pigmento sul bordo (dove la maschera scende) e le chiazze
    dentro = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(5)), np.float64) / 255.0
    bordo = np.clip((m - dentro) * 3.0 + (1 - dentro) * m * 0.9, 0, 1)
    chiazze = rumore(n, 35, rng, 3)
    grana = rumore(n, 2.5, rng, 1)
    densita = np.clip(0.55 + 0.35 * chiazze + 0.45 * bordo + 0.12 * (grana - 0.5), 0, 1.3)
    alfa = np.clip(m * densita * intensita, 0, 1)
    c = np.array(colore, np.float64)
    # dove il pigmento è denso il colore scurisce un poco
    rgb = c[None, None, :] * (1.0 - 0.28 * np.clip(densita - 0.7, 0, 1)[..., None])
    out = np.dstack([np.clip(rgb, 0, 255), alfa * 255]).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def tremolio(linea, rng, forza=0.012):
    """La mano non è un righello: i punti si spostano appena, in modo morbido."""
    p = np.array(linea, np.float64)
    if len(p) < 2:
        return p
    # più punti lungo i segmenti lunghi, così il tremolio si vede anche sulle linee dritte
    nuovi = [p[0]]
    for a, b in zip(p[:-1], p[1:]):
        passi = max(1, int(np.linalg.norm(b - a) / 0.06))
        for k in range(1, passi + 1):
            nuovi.append(a + (b - a) * k / passi)
    p = np.array(nuovi)
    fase = rng.uniform(0, 6.28, 2)
    t = np.linspace(0, 1, len(p))
    p[:, 0] += np.sin(t * 7 + fase[0]) * forza
    p[:, 1] += np.sin(t * 5 + fase[1]) * forza
    if chiusa(linea):
        p[-1] = p[0]
    return p


def icona(ogg, rng):
    n = LATO
    img = piastrella(rng)
    principale, secondo = COLORI.get(ogg["id"], PREDEFINITO)
    linee = [l for l in ogg["linee"] if len(l) >= 2]
    chiuse = sorted([l for l in linee if quasi_chiusa(l)], key=area, reverse=True)
    aperte = [l for l in linee if not quasi_chiusa(l)]

    # 1. l'ombra sotto l'oggetto, appena accennata (sotto il colore)
    ombra = Image.new("L", (n, n), 0)
    ImageDraw.Draw(ombra).ellipse([n * 0.26, n * 0.80, n * 0.74, n * 0.88], fill=50)
    ombra = ombra.filter(ImageFilter.GaussianBlur(6))
    nera = Image.new("RGBA", (n, n), (60, 40, 24, 0))
    nera.putalpha(ombra)
    img = Image.alpha_composite(img, nera)

    # 2. l'acquerello: le forme chiuse, dalla più grande alla più piccola (le piccole sopra)
    for i, l in enumerate(chiuse):
        if area(l) < 0.004:
            continue
        maschera = Image.new("L", (n, n), 0)
        ImageDraw.Draw(maschera).polygon([in_pixel(p) for p in l], fill=255)
        col = colore_della_forma(ogg, l, i, principale, secondo)
        img = Image.alpha_composite(img, acquerello(maschera, col, rng, 0.82 if i == 0 else 0.9))
    # le linee aperte (lame, manici, rami, chiodi, corde): una velatura del colore lungo la linea, più larga se
    # l'oggetto non ha forme chiuse da riempire
    for l in aperte:
        maschera = Image.new("L", (n, n), 0)
        larga = 0.075 if not chiuse else 0.05
        ImageDraw.Draw(maschera).line([in_pixel(p) for p in l], fill=255, width=int(n * larga), joint="curve")
        col = colore_della_forma(ogg, l, 0 if not chiuse else 1, principale, secondo)
        img = Image.alpha_composite(img, acquerello(maschera, col, rng, 0.6))

    # 3. l'inchiostro: tutte le linee, con il tremolio della mano; un poco più spesse in basso a destra (la luce)
    tratto = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(tratto)
    for l in linee:
        p = tremolio(l, rng)
        punti = [in_pixel(q) for q in p]
        d.line(punti, fill=INCHIOSTRO + (235,), width=7, joint="curve")
    tratto = tratto.filter(ImageFilter.GaussianBlur(0.7))
    img = Image.alpha_composite(img, tratto)

    # 4. un colpo di luce in alto a sinistra (la carta che si vede sotto l'acqua)
    luce = Image.new("L", (n, n), 0)
    ImageDraw.Draw(luce).ellipse([n * 0.30, n * 0.30, n * 0.44, n * 0.38], fill=60)
    luce = luce.filter(ImageFilter.GaussianBlur(5))
    bianca = Image.new("RGBA", (n, n), (255, 250, 236, 0))
    bianca.putalpha(luce)
    img = Image.alpha_composite(img, bianca)
    # gli angoli restano trasparenti (la piastrella)
    alfa_piastrella = piastrella(np.random.default_rng(0)).getchannel("A")
    img.putalpha(Image.fromarray(np.minimum(np.asarray(img.getchannel("A")), np.asarray(alfa_piastrella))))
    return img


def anteprima(icone, uscita):
    colonne = 10
    lato = 128
    righe = math.ceil(len(icone) / colonne)
    foglio = Image.new("RGB", (colonne * (lato + 8) + 8, righe * (lato + 22) + 8), (42, 33, 24))
    d = ImageDraw.Draw(foglio)
    for k, (ogg, img) in enumerate(icone):
        x = 8 + (k % colonne) * (lato + 8)
        y = 8 + (k // colonne) * (lato + 22)
        piccola = img.resize((lato, lato), Image.LANCZOS)
        foglio.paste(piccola, (x, y), piccola)
        d.text((x + 2, y + lato + 3), ogg["id"][:20], fill=(220, 205, 170))
    foglio.save(uscita)


def main(cartella_sorgenti, uscita):
    os.makedirs(uscita, exist_ok=True)
    catalogo = estrai(cartella_sorgenti)
    fatte = []
    elenco = []
    for ogg in catalogo:
        # un seme per oggetto (sempre lo stesso): l'acquerello non cambia a ogni lancio
        seme = sum(ord(c) * (i + 1) for i, c in enumerate(ogg["id"])) % 100000
        img = icona(ogg, np.random.default_rng(seme))
        img.save(os.path.join(uscita, "T_Icona_%s.png" % ogg["id"]))
        fatte.append((ogg, img))
        principale, secondo = COLORI.get(ogg["id"], PREDEFINITO)
        elenco.append({"id": ogg["id"], "nome": ogg["nome"], "colori": [principale, secondo],
                       "colore_scelto": ogg["id"] in COLORI})
    anteprima(fatte, os.path.join(uscita, "Anteprima_Icone.png"))
    with open(os.path.join(uscita, "icone.json"), "w", encoding="utf-8") as f:
        json.dump(elenco, f, ensure_ascii=False, indent=1)
    senza = [e["id"] for e in elenco if not e["colore_scelto"]]
    print("icone: %d (in %s)%s" % (len(fatte), uscita, ("; senza colore scelto: " + ", ".join(senza)) if senza else ""))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
