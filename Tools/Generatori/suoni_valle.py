# Valdorso - I suoni della valle (v0.1.3). Scritto da Claude il 06/11/2026. (I suoni delle schermate sono in suoni.py.)
# Fa da zero (niente registrazioni, niente licenze) i suoni della valle, che si ripetono senza scatti:
#   2D, stereo, intorno al colono:  S_Vento, S_Uccelli (giorno), S_Grilli (notte)
#   3D, mono, nel punto giusto:     S_Fiume, S_Lago (la riva), S_Rane, S_Cascata, S_Fucina, S_Locanda, S_Mulino
#   un colpo solo:                  S_CampanaTempio (la campana del Tempio del Cuore)
#   (13/11) un colpo solo, 3D:      S_Porta_Chiave (chiusa a chiave), S_Porta_Apri, S_Porta_Chiudi (AValdorsoPorta)
#   (06/11, sera) 3D, nelle grotte:  S_Grotta (gocce e eco nella grotta est), S_Martelli (sotto la frana, di notte)
# Li importa e li mette nella valle Content/Python/nuovi/crea_suoni_valle.py; la logica (giorno, notte, pioggia, campane che
# suonano le ore) è in ValdorsoAmbiente.h/.cpp e ValdorsoCampane.h/.cpp. Se un giorno ci sono suoni veri migliori (Fab),
# basta mettere un file con lo stesso nome e rilanciare crea_suoni_valle.py.
# Uso: python suoni_valle.py <uscita>

import os
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile

SR = 48000


def rumore(n, rng):
    return rng.standard_normal(n)


def rosa(n, rng):
    """Rumore rosa (1/f), fatto nelle frequenze."""
    s = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / SR)
    f[0] = f[1]
    s /= np.sqrt(f)
    x = np.fft.irfft(s, n)
    return x / np.std(x)


def marrone(n, rng):
    s = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / SR)
    f[0] = f[1]
    s /= f
    x = np.fft.irfft(s, n)
    return x / np.std(x)


def banda(x, basso, alto, ordine=2):
    return signal.sosfilt(signal.butter(ordine, [basso / (SR / 2), alto / (SR / 2)], btype="band", output="sos"), x)


def passa_basso(x, f, ordine=2):
    return signal.sosfilt(signal.butter(ordine, f / (SR / 2), btype="low", output="sos"), x)


def passa_alto(x, f, ordine=2):
    return signal.sosfilt(signal.butter(ordine, f / (SR / 2), btype="high", output="sos"), x)


def lento(n, rng, periodo_s, profondita):
    """Una curva lenta e casuale intorno a 1 (raffiche, onde), che torna uguale alla fine (si ripete senza scatti)."""
    k = max(2, int(n / SR / periodo_s))
    v = rng.uniform(-1, 1, k)
    t = np.arange(n) / n * k
    i0 = np.floor(t).astype(int) % k
    i1 = (i0 + 1) % k
    a = t - np.floor(t)
    a = a * a * (3 - 2 * a)
    return 1.0 + profondita * (v[i0] * (1 - a) + v[i1] * a)


def giro(x, sovrapposti):
    """Chiude il giro: la coda (sovrapposti campioni) sfuma nella testa con potenza costante."""
    n = x.shape[0] - sovrapposti
    t = np.linspace(0, np.pi / 2, sovrapposti)
    if x.ndim == 2:
        t = t[:, None]
    y = x[:n].copy()
    y[:sovrapposti] = x[:sovrapposti] * np.sin(t) + x[n:] * np.cos(t)
    return y


def aggiungi(buffer, pezzo, inizio, pan=None):
    """Somma un evento nel buffer, girando alla fine (una goccia oltre la fine riparte dall'inizio)."""
    n = buffer.shape[0]
    idx = (inizio + np.arange(len(pezzo))) % n
    if buffer.ndim == 2:
        p = 0.0 if pan is None else pan
        sx, dx = np.cos((p + 1) * np.pi / 4), np.sin((p + 1) * np.pi / 4)
        np.add.at(buffer[:, 0], idx, pezzo * sx)
        np.add.at(buffer[:, 1], idx, pezzo * dx)
    else:
        np.add.at(buffer, idx, pezzo)


def eco(x, secondi=0.5, quanto=0.25, rng=None):
    """Un poco di aria intorno: riverbero corto fatto con rumore che si spegne, in giro (la coda che passa la fine
    riparte dall'inizio, così il suono si ripete senza scatti)."""
    rng = rng or np.random.default_rng(1)
    m = int(secondi * SR)
    ir = rng.standard_normal(m) * np.exp(-np.arange(m) / (m / 5))
    ir = passa_basso(ir, 5000)
    ir /= np.sqrt(np.sum(ir ** 2))

    def una(c):
        n = len(c)
        r = np.real(np.fft.ifft(np.fft.fft(c) * np.fft.fft(ir, n)))
        return c + quanto * r

    if x.ndim == 2:
        return np.stack([una(x[:, 0]), una(x[:, 1])], axis=1)
    return una(x)


def normalizza(x, picco=0.8):
    return x / (np.max(np.abs(x)) + 1e-9) * picco


def scrivi(cartella, nome, x, picco=0.8):
    x = normalizza(x, picco)
    wavfile.write(os.path.join(cartella, nome + ".wav"), SR, (x * 32767).astype(np.int16))
    print("  %s: %.1f s, %s" % (nome, x.shape[0] / SR, "stereo" if x.ndim == 2 else "mono"))


# ------------------------------------------------------------------------------------------------ il vento

def vento(secondi=30.0, seme=1):
    rng = np.random.default_rng(seme)
    s = int(SR * 2)
    n = int(secondi * SR) + s
    canali = []
    for c in range(2):
        b = marrone(n, rng) * 0.6 + rosa(n, rng) * 0.4
        b = banda(b, 60, 900)
        fischio = banda(rosa(n, rng), 500, 1400, 2) * 0.25 * lento(n, rng, 4.0, 0.9)
        canali.append((b + fischio) * lento(n, rng, 3.0 + c, 0.6) * lento(n, rng, 9.0, 0.3))
    return giro(np.stack(canali, axis=1), s)


# ------------------------------------------------------------------------------------------------ gli uccelli

def fischio(rng, f0, f1, durata, vibrato=0.0):
    n = int(durata * SR)
    t = np.arange(n) / SR
    f = np.linspace(f0, f1, n) * (1 + vibrato * np.sin(2 * np.pi * rng.uniform(20, 40) * t))
    fase = 2 * np.pi * np.cumsum(f) / SR
    inv = np.sin(np.pi * np.arange(n) / n) ** 1.5
    return np.sin(fase) * inv + 0.15 * np.sin(2 * fase) * inv


def canto(rng, specie):
    """Una frase di un uccello: specie 0 = merlo (fischi morbidi che scendono), 1 = fringuello (trillo che accelera e
    finisce con una svolta), 2 = cinciallegra (due note ripetute), 3 = pettirosso (note alte e veloci)."""
    pezzi = []
    if specie == 0:
        base = rng.uniform(1800, 2600)
        for _ in range(rng.integers(3, 7)):
            f0 = base * rng.uniform(0.8, 1.3)
            pezzi.append(fischio(rng, f0, f0 * rng.uniform(0.7, 1.1), rng.uniform(0.08, 0.25), 0.01))
            pezzi.append(np.zeros(int(rng.uniform(0.03, 0.09) * SR)))
    elif specie == 1:
        f = rng.uniform(3500, 4500)
        gap = 0.07
        for k in range(rng.integers(8, 14)):
            pezzi.append(fischio(rng, f * 1.15, f * 0.85, 0.04))
            pezzi.append(np.zeros(int(gap * SR)))
            gap *= 0.9
            f *= 0.985
        pezzi.append(fischio(rng, f * 0.9, f * 1.4, 0.18))
    elif specie == 2:
        a, b = rng.uniform(4800, 5600), rng.uniform(3300, 3800)
        for _ in range(rng.integers(3, 6)):
            pezzi.append(fischio(rng, a, a * 0.97, 0.09))
            pezzi.append(np.zeros(int(0.03 * SR)))
            pezzi.append(fischio(rng, b, b * 0.97, 0.12))
            pezzi.append(np.zeros(int(0.06 * SR)))
    else:
        for _ in range(rng.integers(6, 12)):
            f0 = rng.uniform(4500, 7500)
            pezzi.append(fischio(rng, f0, f0 * rng.uniform(0.8, 1.25), rng.uniform(0.03, 0.08), 0.02))
            pezzi.append(np.zeros(int(rng.uniform(0.01, 0.05) * SR)))
    return np.concatenate(pezzi)


def uccelli(secondi=96.0, seme=2):
    """(13/11, dopo la prova: "troppo spesso, serve qualcosa di più calmo") Prima erano 9 uccelli che cantavano ogni
    2-7 secondi in un giro di 40 secondi: un coro continuo. Ora sono 4, quasi tutti lontani, che cantano ogni 12-35
    secondi in un giro lungo (96 s, così non si sente che si ripete); in mezzo restano lunghi momenti di sole foglie.
    Il merlo (morbido) canta più spesso, il pettirosso (acuto) quasi mai."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    x = np.zeros((n, 2))
    # (specie, quanto è lontano 0..1, pausa minima, pausa massima)
    uccelli_valle = [(0, 0.55, 12.0, 26.0), (2, 0.35, 16.0, 32.0), (1, 0.30, 20.0, 35.0), (0, 0.25, 14.0, 30.0)]
    for specie, lontano, pausa_min, pausa_max in uccelli_valle:
        pan = rng.uniform(-0.85, 0.85)
        t = rng.uniform(0, pausa_max)
        while t < secondi:
            c = canto(rng, specie)
            # lontano = via gli acuti (l'aria li mangia)
            c = passa_basso(c, 3200 if lontano < 0.4 else 4500)
            aggiungi(x, c * lontano * rng.uniform(0.7, 1.0), int(t * SR), pan)
            t += len(c) / SR + rng.uniform(pausa_min, pausa_max)
    x = eco(x, 0.9, 0.45, rng)
    # le foglie mosse dall'aria, un po' più presenti di prima (il fondo calmo che resta tra un canto e l'altro)
    fondo = np.stack([banda(rosa(n, rng), 1500, 6000) * lento(n, rng, 6.0, 0.5) for _ in range(2)], axis=1) * 0.02
    return x + fondo


# ------------------------------------------------------------------------------------------------ le porte (13/11)

def risonanza(rng, frequenze, durata, smorzo):
    """Un pezzo di metallo o di legno che suona: alcune frequenze che si spengono (smorzo = secondi per spegnersi)."""
    n = int(durata * SR)
    t = np.arange(n) / SR
    x = np.zeros(n)
    for f in frequenze:
        f = f * rng.uniform(0.97, 1.03)
        x += np.sin(2 * np.pi * f * t + rng.uniform(0, 6.28)) * np.exp(-t / (smorzo * rng.uniform(0.7, 1.3)))
    return x / len(frequenze)


def scatto(rng, forza=1.0):
    """Lo scatto di un saliscendi di ferro: un colpetto secco e il ferro che vibra un attimo."""
    n = int(0.12 * SR)
    t = np.arange(n) / SR
    colpo = banda(rng.standard_normal(n), 1800, 7000) * np.exp(-t / 0.004)
    ferro = risonanza(rng, [1850, 2930, 4310, 6020], 0.12, 0.025)
    return (colpo * 0.8 + ferro * 0.6) * forza


def tonfo(rng, forza=1.0):
    """L'anta di legno che batte sullo stipite: un colpo sordo e basso."""
    n = int(0.5 * SR)
    t = np.arange(n) / SR
    legno = risonanza(rng, [92, 141, 233, 410], 0.5, 0.07)
    botta = passa_basso(rng.standard_normal(n), 900) * np.exp(-t / 0.012)
    return (legno * 1.2 + botta * 0.6) * forza


def cigolio(rng, durata, f0, f1, forza=1.0):
    """I cardini che cigolano: tanti piccoli strappi (attrito che si attacca e si stacca), sempre un po' irregolari,
    che il legno dell'anta fa risuonare."""
    n = int(durata * SR)
    x = np.zeros(n)
    t = 0.0
    while t < durata:
        f = f0 + (f1 - f0) * (t / durata)
        f *= rng.uniform(0.9, 1.1)
        i = int(t * SR)
        if i < n:
            x[i] += rng.uniform(0.6, 1.0)
        t += 1.0 / f
    x = banda(x, 350, 3200, 2)
    # il legno: tre risonanze larghe
    x = banda(x, 500, 1600, 1) * 0.7 + banda(x, 1800, 2600, 1) * 0.3
    inv = np.sin(np.pi * np.arange(n) / n) ** 0.7
    return x * inv * forza


def porta_chiave(seme=21):
    """Chi prova una porta chiusa a chiave: il saliscendi si alza, sbatte due volte contro la serratura, ricade."""
    rng = np.random.default_rng(seme)
    x = np.zeros(int(1.4 * SR))     # (la coda dell'eco gira in tondo: un po' di silenzio in fondo)
    aggiungi(x, scatto(rng, 0.7), int(0.02 * SR))
    for k, quando in enumerate((0.22, 0.38, 0.52)):
        aggiungi(x, scatto(rng, 1.0 - 0.15 * k), int(quando * SR))
        aggiungi(x, tonfo(rng, 0.18), int((quando + 0.005) * SR))
    aggiungi(x, scatto(rng, 0.6), int(0.78 * SR))
    return eco(x, 0.25, 0.15, rng)


def porta_apri(seme=22):
    """Una porta di legno che si apre: il saliscendi e poi i cardini che cigolano salendo."""
    rng = np.random.default_rng(seme)
    x = np.zeros(int(1.4 * SR))
    aggiungi(x, scatto(rng, 0.8), int(0.02 * SR))
    aggiungi(x, cigolio(rng, 0.9, 210, 340, 0.9), int(0.15 * SR))
    return eco(x, 0.3, 0.18, rng)


def porta_chiudi(seme=23):
    """Una porta che si chiude: un cigolio corto, l'anta che batte e il saliscendi che ricade."""
    rng = np.random.default_rng(seme)
    x = np.zeros(int(1.3 * SR))
    aggiungi(x, cigolio(rng, 0.45, 300, 220, 0.6), int(0.02 * SR))
    aggiungi(x, tonfo(rng, 1.0), int(0.5 * SR))
    aggiungi(x, scatto(rng, 0.7), int(0.56 * SR))
    return eco(x, 0.35, 0.2, rng)


# ------------------------------------------------------------------------------------------------ i grilli

def grilli(secondi=20.0, seme=3):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    x = np.zeros((n, 2))
    for g in range(14):
        f = rng.uniform(4200, 5100)
        pan = rng.uniform(-1, 1)
        forza = rng.uniform(0.15, 1.0)
        periodo = rng.uniform(0.45, 0.8)                # un cri-cri ogni tanto
        impulsi = rng.integers(3, 5)
        dur_imp = 0.014
        tt = np.arange(int(dur_imp * SR)) / SR
        imp = np.sin(2 * np.pi * f * tt) * np.sin(np.pi * tt / dur_imp) ** 2
        cri = np.zeros(int((impulsi * 0.03 + 0.02) * SR))
        for k in range(impulsi):
            i = int(k * 0.03 * SR)
            cri[i:i + len(imp)] += imp
        t = rng.uniform(0, periodo)
        while t < secondi:
            aggiungi(x, cri * forza * rng.uniform(0.8, 1.0), int(t * SR), pan)
            t += periodo * rng.uniform(0.97, 1.03)
    # un trillo continuo lontano (grilli degli alberi)
    tt = np.arange(n) / SR
    trillo = np.sin(2 * np.pi * 2900 * tt) * (0.5 + 0.5 * np.sin(2 * np.pi * 45 * tt)) ** 4
    x += np.stack([trillo * 0.02, trillo * 0.016], axis=1)
    return eco(x, 0.4, 0.2, rng)


# ------------------------------------------------------------------------------------------------ le rane

def gracidio(rng):
    """Un cra: impulsi a 20-35 al secondo, ognuno filtrato da due risonanze."""
    rate = rng.uniform(18, 35)
    durata = rng.uniform(0.18, 0.45)
    n = int(durata * SR)
    x = np.zeros(n)
    passo = int(SR / rate)
    for i in range(0, n - 200, passo):
        x[i] = rng.uniform(0.7, 1.0)
    f1, f2 = rng.uniform(350, 600), rng.uniform(1100, 1700)
    y = banda(x, f1 * 0.8, f1 * 1.2, 2) * 1.0 + banda(x, f2 * 0.85, f2 * 1.15, 2) * 0.5
    inv = np.sin(np.pi * np.arange(n) / n) ** 0.7
    return y * inv


def rane(secondi=24.0, seme=4):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    x = np.zeros(n)
    for r in range(7):
        forza = rng.uniform(0.3, 1.0)
        t = rng.uniform(0, 3)
        while t < secondi:
            # un gruppo di cra vicini, poi silenzio
            for _ in range(rng.integers(2, 6)):
                c = gracidio(rng)
                aggiungi(x, c * forza, int(t * SR))
                t += len(c) / SR + rng.uniform(0.15, 0.4)
            t += rng.uniform(2.5, 8.0)
    return eco(x, 0.5, 0.3, rng)


# ------------------------------------------------------------------------------------------------ l'acqua

def fiume(secondi=20.0, seme=5):
    rng = np.random.default_rng(seme)
    s = int(SR * 1.5)
    n = int(secondi * SR) + s
    scorre = banda(rosa(n, rng), 150, 4000) * lento(n, rng, 1.5, 0.25)
    bolle = np.zeros(n)
    for _ in range(int(secondi * 18)):
        d = rng.uniform(0.02, 0.07)
        m = int(d * SR)
        tt = np.arange(m) / SR
        f0 = rng.uniform(400, 1400)
        b = np.sin(2 * np.pi * np.cumsum(f0 * (1 + 2.5 * tt / d)) / SR) * np.exp(-tt / (d / 3))
        aggiungi(bolle, b * rng.uniform(0.1, 0.5), int(rng.uniform(0, n)))
    return giro(scorre * 0.8 + bolle * 0.6, s)


def lago(secondi=24.0, seme=6):
    rng = np.random.default_rng(seme)
    s = int(SR * 2)
    n = int(secondi * SR) + s
    onde = passa_basso(rosa(n, rng), 1200) * np.clip(lento(n, rng, 2.2, 0.9), 0.05, None) ** 2
    schizzi = np.zeros(n)
    for _ in range(int(secondi * 1.2)):
        m = int(rng.uniform(0.15, 0.4) * SR)
        sch = banda(rng.standard_normal(m), 800, 5000) * np.exp(-np.arange(m) / (m / 4))
        aggiungi(schizzi, sch * rng.uniform(0.1, 0.35), int(rng.uniform(0, n)))
    return giro(onde + schizzi, s)


def cascata(secondi=20.0, seme=7):
    rng = np.random.default_rng(seme)
    s = int(SR * 1.5)
    n = int(secondi * SR) + s
    rombo = passa_basso(marrone(n, rng), 180) * 0.8
    getto = banda(rosa(n, rng), 300, 6000) * lento(n, rng, 0.7, 0.15)
    return giro(rombo + getto, s)


# ------------------------------------------------------------------------------------------------ il villaggio

def colpo_incudine(rng, forza=1.0):
    d = 0.9
    m = int(d * SR)
    tt = np.arange(m) / SR
    base = rng.uniform(820, 900)
    x = np.zeros(m)
    for rapporto, ampiezza, decadimento in ((1.0, 1.0, 0.35), (2.76, 0.6, 0.2), (5.40, 0.35, 0.12), (8.93, 0.2, 0.07)):
        x += ampiezza * np.sin(2 * np.pi * base * rapporto * tt) * np.exp(-tt / decadimento)
    clic = rng.standard_normal(m) * np.exp(-tt / 0.004) * 0.8
    return (x + clic) * forza


def fucina(secondi=14.0, seme=8):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    fuoco = banda(rosa(n, rng), 80, 1200) * 0.08 * lento(n, rng, 1.0, 0.4)
    scoppi = np.zeros(n)
    for _ in range(int(secondi * 6)):
        m = int(0.01 * SR)
        aggiungi(scoppi, rng.standard_normal(m) * np.exp(-np.arange(m) / 60) * rng.uniform(0.05, 0.25), int(rng.uniform(0, n)))
    x = fuoco + passa_alto(scoppi, 1500)
    # Marta batte: gruppi di colpi, poi gira il ferro e lo rimette nel fuoco
    t = 0.5
    while t < secondi - 1:
        for k in range(rng.integers(3, 7)):
            aggiungi(x, colpo_incudine(rng, rng.uniform(0.6, 1.0) * (0.55 if k % 2 else 1.0)), int(t * SR))
            t += rng.uniform(0.42, 0.5)
        t += rng.uniform(1.5, 3.0)
    return eco(x, 0.35, 0.3, rng)


def voce(rng, durata, f0):
    """Una persona che parla (si sente da fuori, senza parole): impulsi alla voce, tre formanti che cambiano a sillabe."""
    n = int(durata * SR)
    tt = np.arange(n) / SR
    f = f0 * (1 + 0.08 * np.sin(2 * np.pi * rng.uniform(0.3, 0.8) * tt))
    fase = np.cumsum(f) / SR
    impulsi = (np.diff(np.floor(fase), prepend=0) > 0).astype(float)
    sillabe = np.clip(np.sin(2 * np.pi * rng.uniform(3.5, 5.5) * tt + rng.uniform(0, 6)), 0, 1) ** 0.8
    y = np.zeros(n)
    blocco = int(0.12 * SR)
    for i in range(0, n, blocco):
        j = min(n, i + blocco)
        f1, f2 = rng.uniform(400, 800), rng.uniform(900, 2000)
        y[i:j] = banda(impulsi[i:j], f1 * 0.8, f1 * 1.25) + 0.5 * banda(impulsi[i:j], f2 * 0.85, f2 * 1.15)
    return y * sillabe * np.sin(np.pi * tt / durata) ** 0.3


def locanda(secondi=24.0, seme=9):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    x = np.zeros(n)
    for p in range(8):
        f0 = rng.uniform(95, 140) if p % 3 else rng.uniform(170, 230)
        t = rng.uniform(0, 3)
        while t < secondi:
            d = rng.uniform(1.0, 3.5)
            aggiungi(x, voce(rng, d, f0) * rng.uniform(0.4, 1.0), int(t * SR))
            t += d + rng.uniform(0.8, 4.0)
    # risate e boccali
    for _ in range(int(secondi / 5)):
        d = rng.uniform(0.6, 1.2)
        r = voce(rng, d, rng.uniform(150, 220)) * (0.5 + 0.5 * np.sin(2 * np.pi * 6 * np.arange(int(d * SR)) / SR) > 0.5)
        aggiungi(x, r * 1.2, int(rng.uniform(0, n)))
    for _ in range(int(secondi / 2.5)):
        m = int(0.25 * SR)
        tt = np.arange(m) / SR
        f = rng.uniform(1800, 2600)
        aggiungi(x, (np.sin(2 * np.pi * f * tt) + 0.5 * np.sin(2 * np.pi * f * 2.3 * tt)) * np.exp(-tt / 0.05) * 0.4, int(rng.uniform(0, n)))
    # si sente da fuori: i muri tolgono gli acuti
    return passa_basso(eco(x, 0.4, 0.4, rng), 2500)


def mulino(secondi=12.0, seme=10):
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    x = fiume(secondi, seme + 1) * 0.5      # (fiume() chiude già il giro: lunga esattamente secondi)
    t = 0.2
    while t < secondi:
        # una pala entra nell'acqua: uno schiaffo
        m = int(0.35 * SR)
        sch = banda(rng.standard_normal(m), 300, 3500) * np.exp(-np.arange(m) / (m / 5))
        aggiungi(x, sch * rng.uniform(0.5, 0.8), int(t * SR))
        t += 1.0
    # il legno dell'asse che cigola, lento
    tt = np.arange(n) / SR
    for k in range(int(secondi / 4)):
        inizio = rng.uniform(0, secondi - 1.2)
        d = rng.uniform(0.6, 1.1)
        m = int(d * SR)
        f = np.linspace(rng.uniform(140, 200), rng.uniform(220, 320), m)
        sega = signal.sawtooth(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.pi * np.arange(m) / m) ** 2
        aggiungi(x, banda(sega, 200, 2500) * 0.25, int(inizio * SR))
    return x


# ------------------------------------------------------------------------------------------------ la campana

def campana(secondi=9.0, seme=11, nota=310.0):
    """Un colpo di campana: le parziali di una campana da chiesa (hum, prime, terza minore, quinta, nominale, ...),
    ognuna con il suo spegnersi, e il colpo del batacchio."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    tt = np.arange(n) / SR
    x = np.zeros(n)
    for rapporto, ampiezza, decadimento in ((0.5, 0.55, 4.5), (1.0, 0.8, 3.2), (1.19, 0.6, 2.4), (1.5, 0.35, 2.0),
                                            (2.0, 0.7, 1.8), (2.52, 0.3, 1.1), (3.0, 0.25, 0.9), (4.07, 0.18, 0.6),
                                            (5.3, 0.1, 0.4)):
        f = nota * rapporto * (1 + rng.uniform(-0.002, 0.002))
        battimento = 1 + 0.06 * np.sin(2 * np.pi * rng.uniform(0.6, 1.4) * tt)
        x += ampiezza * np.sin(2 * np.pi * f * tt + rng.uniform(0, 6)) * np.exp(-tt / decadimento) * battimento
    colpo = banda(rng.standard_normal(int(0.03 * SR)), 1500, 8000) * np.exp(-np.arange(int(0.03 * SR)) / 300)
    x[:len(colpo)] += colpo * 0.6
    attacco = np.clip(tt / 0.003, 0, 1)
    return x * attacco


# ------------------------------------------------------------------------------------------------ le grotte (06/11, sera)

def goccia(rng, forza=1.0):
    """Una goccia che cade in una pozza: un 'plin' che sale di tono, cortissimo."""
    d = 0.12
    m = int(d * SR)
    tt = np.arange(m) / SR
    f0 = rng.uniform(1100, 2600)
    f = f0 * (1 + 0.9 * (1 - np.exp(-tt / 0.012)))
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-tt / rng.uniform(0.018, 0.035))
    x += rng.standard_normal(m) * np.exp(-tt / 0.002) * 0.15
    return x * forza


def grotta(secondi=32.0, seme=12):
    """S_Grotta: dentro la grotta est. Un fondo cupo che quasi non si sente, gocce lontane e vicine (alcune tornano
    sempre dallo stesso punto, a ritmo), e l'eco lunga della roccia."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    s = int(SR * 1.5)
    fondo = giro(passa_basso(marrone(n + s, rng), 140), s) * 0.05 * lento(n, rng, 3.0, 0.3)
    x = fondo.copy()
    # tre gocce "fisse" con il loro ritmo, più qualche goccia sparsa
    for periodo, forza in ((2.7, 0.9), (4.1, 0.5), (6.3, 0.35)):
        t = rng.uniform(0, periodo)
        f = rng.uniform(0.85, 1.15)
        while t < secondi:
            aggiungi(x, goccia(np.random.default_rng(int(periodo * 100)), forza * f * rng.uniform(0.85, 1.1)), int(t * SR))
            t += periodo * rng.uniform(0.92, 1.08)
    for _ in range(int(secondi / 2.5)):
        aggiungi(x, goccia(rng, rng.uniform(0.1, 0.4)), int(rng.uniform(0, n)))
    return eco(eco(x, 1.4, 0.9, rng), 0.25, 0.4, rng)


def martelli(secondi=48.0, seme=13):
    """S_Martelli: sotto la montagna, di notte (dalla lore: dal 309 i martelli dei nani). Colpi d'incudine lontani e
    soffocati dalla roccia: un colpo, poi niente, poi un altro. Quattro colpi in 48 secondi, mai a ritmo."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    s = int(SR * 1.5)
    x = giro(passa_basso(marrone(n + s, rng), 90), s) * 0.02
    t = rng.uniform(1.0, 3.0)
    while t < secondi - 2.0:
        colpo = passa_basso(colpo_incudine(rng, rng.uniform(0.7, 1.0)), 700, 4)
        tonfo = passa_basso(rng.standard_normal(int(0.25 * SR)), 120) * np.exp(-np.arange(int(0.25 * SR)) / (0.06 * SR)) * 3.0
        aggiungi(x, colpo, int(t * SR))
        aggiungi(x, tonfo, int(t * SR))
        t += rng.uniform(7.0, 15.0)
    return eco(x, 1.8, 1.2, rng)


# ------------------------------------------------------------------------------------------------ il drago (11/11)

def ruggito(secondi=7.0, seme=14):
    """S_Drago_Ruggito: il drago che passa alto sopra i monti, lontanissimo (v0.1.7). Non un urlo: un brontolio enorme
    che sale e scende, filtrato dalla distanza (niente acuti), con l'eco delle montagne. Si sente piu' che si ascolta."""
    rng = np.random.default_rng(seme)
    n = int(secondi * SR)
    tt = np.arange(n) / SR
    # la voce: una fondamentale bassa che sale piano e poi crolla, con le armoniche ruvide
    inv = np.clip(tt / 1.6, 0, 1) ** 1.5 * np.clip((secondi - 1.2 - tt) / 2.8, 0, 1)
    f0 = 46.0 + 22.0 * np.sin(np.pi * np.clip(tt / (secondi - 1.0), 0, 1)) + 3.0 * np.sin(2 * np.pi * 5.5 * tt)
    fase = 2 * np.pi * np.cumsum(f0) / SR
    voce = np.zeros(n)
    for k, a in ((1, 1.0), (2, 0.7), (3, 0.5), (4, 0.32), (5, 0.22), (7, 0.12)):
        voce += a * np.sin(k * fase + rng.uniform(0, 6))
    # la gola: rumore modulato alla stessa voce
    gola = banda(rng.standard_normal(n), 60, 420) * (0.6 + 0.4 * np.sin(fase))
    x = (voce * 0.7 + gola * 0.9) * inv
    x = np.tanh(x * 1.8)
    # lontano: via gli acuti, e il rimbombo dei monti
    x = passa_basso(x, 520, 3)
    x = np.concatenate([x, np.zeros(int(3.0 * SR))])     # (l'eco gira in tondo: la coda finisce nel silenzio)
    x = eco(x, secondi=0.9, quanto=0.35, rng=rng)
    x = eco(x, secondi=1.7, quanto=0.2, rng=rng)
    return x


if __name__ == "__main__":
    cartelle = [a for a in sys.argv[1:] if not a.startswith("--")]
    uscita = cartelle[0] if cartelle else "."
    os.makedirs(uscita, exist_ok=True)
    print("suoni in", uscita)
    # (13/11) le porte (un colpo solo, 3D) e gli uccelli più calmi; con "--solo-nuovi" fa solo questi quattro
    scrivi(uscita, "S_Porta_Chiave", porta_chiave(), 0.8)
    scrivi(uscita, "S_Porta_Apri", porta_apri(), 0.7)
    scrivi(uscita, "S_Porta_Chiudi", porta_chiudi(), 0.8)
    if "--solo-nuovi" in sys.argv:
        scrivi(uscita, "S_Uccelli", uccelli(), 0.6)
        sys.exit(0)
    scrivi(uscita, "S_Vento", vento(), 0.7)
    scrivi(uscita, "S_Uccelli", uccelli(), 0.6)
    scrivi(uscita, "S_Grilli", grilli(), 0.6)
    scrivi(uscita, "S_Rane", rane(), 0.8)
    scrivi(uscita, "S_Fiume", fiume(), 0.7)
    scrivi(uscita, "S_Lago", lago(), 0.7)
    scrivi(uscita, "S_Cascata", cascata(), 0.8)
    scrivi(uscita, "S_Fucina", fucina(), 0.8)
    scrivi(uscita, "S_Locanda", locanda(), 0.7)
    scrivi(uscita, "S_Mulino", mulino(), 0.7)
    scrivi(uscita, "S_CampanaTempio", campana(), 0.9)
    scrivi(uscita, "S_Grotta", grotta(), 0.6)
    scrivi(uscita, "S_Martelli", martelli(), 0.7)
    scrivi(uscita, "S_Drago_Ruggito", ruggito(), 0.8)
