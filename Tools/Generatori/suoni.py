# Valdorso - Generatore dei suoni del Registro (05/10/2026, Claude).
# Suoni fatti da zero con la matematica (sintesi), quindi senza licenze: battito del Cuore, fuoco, pennino, campana,
# ceralacca e il motivo di ogni fede (3-5 note). Escono come WAV 48 kHz in una cartella.
# Uso: python suoni.py [cartella_di_uscita]   (serve numpy e scipy)

import sys
import numpy as np
from scipy.io import wavfile
from scipy import signal

SR = 48000
rng = np.random.default_rng(312)
USCITA = sys.argv[1] if len(sys.argv) > 1 else "."


def t(sec):
    return np.arange(int(sec * SR)) / SR


def riverbero(x, durata=2.2, umido=0.28, chiaro=4000.0):
    """Un riverbero semplice: rumore che si spegne, filtrato (una stanza di pietra)."""
    n = int(durata * SR)
    ir = rng.standard_normal(n) * np.exp(-6.9 * np.arange(n) / n)
    b, a = signal.butter(2, chiaro / (SR / 2))
    ir = signal.lfilter(b, a, ir)
    ir /= np.sqrt(np.sum(ir ** 2))
    bagnato = signal.fftconvolve(x, ir)
    secco = np.concatenate([x, np.zeros(n - 1)])
    return secco * (1 - umido) + bagnato * umido * 2.5


def salva(nome, x, picco=0.85, loop=False):
    x = np.asarray(x, float)
    # Toglie i bassissimi che non si sentono (e lo spostamento dello zero).
    x = signal.filtfilt(*signal.butter(2, 30 / (SR / 2), btype="high"), x)
    if not loop:
        # niente click alla fine
        coda = min(len(x), int(0.02 * SR))
        x[-coda:] *= np.linspace(1, 0, coda)
    x = x / (np.max(np.abs(x)) + 1e-9) * picco
    wavfile.write(f"{USCITA}/{nome}.wav", SR, (x * 32767).astype(np.int16))
    print("Fatto:", nome, f"{len(x) / SR:.2f} s")


def loop_morbido(x, incrocio=0.5):
    """Rende un suono ripetibile senza scatti: la fine sfuma nell'inizio."""
    n = int(incrocio * SR)
    testa, corpo = x[:n], x[n:]
    f = np.linspace(0, 1, n)
    corpo[-n:] = corpo[-n:] * (1 - f) + testa * f
    return corpo


# ---------------------------------------------------------------- Battito del Cuore (1,6 s, si ripete)
def colpo(forte):
    """(05/10, più dolce su richiesta di Fra) Un colpo morbido: attacco lento, niente schiocco, tono caldo."""
    tt = t(0.6)
    freq = 46 + 12 * np.exp(-tt * 14)                      # scende appena di tono
    fase = 2 * np.pi * np.cumsum(freq) / SR
    attacco = np.clip(tt / 0.018, 0, 1) ** 2               # entra piano, senza "tac"
    corpo = (np.sin(fase) + 0.18 * np.sin(2 * fase)) * np.exp(-tt * 6.5) * attacco
    sotto = np.sin(fase * 0.5) * np.exp(-tt * 4.5) * attacco * 0.45
    x = corpo + sotto
    x = signal.lfilter(*signal.butter(2, 260 / (SR / 2)), x)   # caldo: niente alti
    return x * forte


battito = np.zeros(int(1.6 * SR))
for inizio, forte in [(0.0, 1.0), (0.32, 0.6)]:
    c = colpo(forte)
    i = int(inizio * SR)
    battito[i:i + len(c)] += c[: len(battito) - i]
battito = riverbero(battito, 1.6, 0.18, 700)
# La coda del riverbero oltre 1,6 s torna all'inizio: il battito si ripete senza scatti.
giro = int(1.6 * SR)
coda = battito[giro:]
battito = battito[:giro].copy()
battito[: len(coda)] += coda[:giro]
salva("S_Battito", battito, 0.9, loop=True)

# ---------------------------------------------------------------- Fuoco dei bracieri (8 s, si ripete)
dur = 8.5
rombo = signal.lfilter(*signal.butter(2, 250 / (SR / 2)), np.cumsum(rng.standard_normal(int(dur * SR))) * 0.02)
rombo = rombo - signal.lfilter(*signal.butter(1, 20 / (SR / 2)), rombo)
soffio = signal.lfilter(*signal.butter(2, [400 / (SR / 2), 2500 / (SR / 2)], btype="band"), rng.standard_normal(int(dur * SR)))
onda = 0.6 + 0.4 * np.sin(2 * np.pi * 0.23 * t(dur)) * np.sin(2 * np.pi * 0.07 * t(dur) + 1)
fuoco = rombo / np.max(np.abs(rombo)) * 0.6 + soffio / np.max(np.abs(soffio)) * 0.12 * onda
for _ in range(int(dur * 9)):                               # gli scoppiettii
    i = rng.integers(0, int(dur * SR) - 3000)
    lung = rng.integers(60, 900)
    sc = rng.standard_normal(lung) * np.exp(-np.arange(lung) / (lung / 5)) * rng.uniform(0.2, 1.0)
    fuoco[i:i + lung] += signal.lfilter(*signal.butter(2, rng.uniform(700, 2500) / (SR / 2), btype="high"), sc)
salva("S_Fuoco", loop_morbido(fuoco), 0.6, loop=True)

# ---------------------------------------------------------------- Il pennino che gratta (2 s, si ripete mentre scrive)
dur = 2.5
# (05/10, più delicato su richiesta di Fra) Un pennino d'oca sulla pergamena: tanti graffi minuscoli (le fibre della
# carta) dentro tratti leggeri, con pause; niente fruscio pieno.
pennino = np.zeros(int(dur * SR))
pos = int(0.02 * SR)
while pos < len(pennino) - SR // 8:
    lung = int(rng.uniform(0.05, 0.22) * SR)              # un tratto di penna
    pressione = np.sin(np.linspace(0, np.pi, lung)) ** 1.5 * rng.uniform(0.35, 1.0)
    # i graffi: impulsi radi e irregolari, più fitti quando si preme di più
    graffi = (rng.random(lung) < 0.012 + 0.03 * pressione) * rng.uniform(-1, 1, lung)
    soffio = rng.standard_normal(lung) * 0.08
    tratto = signal.lfilter(*signal.butter(2, [3500 / (SR / 2), 11000 / (SR / 2)], btype="band"), graffi + soffio)
    pennino[pos:pos + lung] += tratto * pressione
    pos += lung + int(rng.uniform(0.04, 0.18) * SR)       # la penna si alza
pennino = riverbero(pennino, 0.5, 0.12, 6000)[: len(pennino)]
salva("S_Pennino", loop_morbido(pennino, 0.3), 0.32, loop=True)

# ---------------------------------------------------------------- La campana del tempio (alla firma)
tt = t(7.0)
fondo = 196.0                                                # sol
parziali = [(0.5, 0.5, 0.9), (1.0, 1.0, 1.4), (1.19, 0.6, 2.0), (1.5, 0.45, 2.2), (2.0, 0.6, 2.6),
            (2.52, 0.3, 3.5), (2.66, 0.25, 3.8), (3.01, 0.2, 4.5), (4.1, 0.12, 6.0)]
campana = np.zeros(len(tt))
for rapporto, ampiezza, smorza in parziali:
    battimento = 1 + 0.003 * rng.standard_normal()
    campana += ampiezza * np.sin(2 * np.pi * fondo * rapporto * battimento * tt + rng.uniform(0, 6)) * np.exp(-tt * smorza / 2.2)
campana += rng.standard_normal(len(tt)) * np.exp(-tt * 80) * 0.4   # il battaglio
salva("S_Campana", riverbero(campana, 3.0, 0.35, 3500), 0.85)

# ---------------------------------------------------------------- La ceralacca (il sigillo che cade)
tt = t(0.5)
# (05/10, un po' più acuta su richiesta di Fra)
tonfo = np.sin(2 * np.pi * (230 + 140 * np.exp(-tt * 30)) * tt) * np.exp(-tt * 26)
schiaccia = signal.lfilter(*signal.butter(2, [700 / (SR / 2), 3500 / (SR / 2)], btype="band"), rng.standard_normal(len(tt))) * np.exp(-tt * 14) * 0.5
salva("S_Ceralacca", riverbero(tonfo + schiaccia, 0.8, 0.2, 2000), 0.8)


# ---------------------------------------------------------------- I motivi delle fedi
def nota(midi):
    return 440.0 * 2 ** ((midi - 69) / 12)


def arpa(f, dur=2.4, chiaro=0.996):
    """Corda pizzicata (Karplus-Strong): arpa e liuto. Pizzicata col polpastrello, quindi morbida."""
    n = int(dur * SR)
    periodo = int(SR / f)
    eccita = rng.uniform(-1, 1, periodo * 4)
    eccita = signal.lfilter(*signal.butter(2, min(f * 6, 18000) / (SR / 2)), eccita)[-periodo:]
    buf = eccita - eccita.mean()                            # niente spostamento dello zero (DC)
    buf /= np.max(np.abs(buf)) + 1e-9
    out = np.zeros(n)
    for i in range(n):
        j = i % periodo
        out[i] = buf[j]
        # filtro a tre punti: le armoniche alte si spengono prima, come in una corda vera
        buf[j] = chiaro * (0.25 * buf[j - 1] + 0.5 * buf[j] + 0.25 * buf[(j + 1) % periodo])
    return out


def flauto(f, dur=1.2, respiro=0.08):
    tt = t(dur)
    vib = 1 + 0.006 * np.sin(2 * np.pi * 5.2 * tt) * np.clip(tt * 2, 0, 1)
    fase = 2 * np.pi * f * np.cumsum(vib) / SR
    suono = np.sin(fase) + 0.25 * np.sin(2 * fase) + 0.08 * np.sin(3 * fase)
    soffio = signal.lfilter(*signal.butter(2, [f * 0.8 / (SR / 2), min(f * 4, 20000) / (SR / 2)], btype="band"), rng.standard_normal(len(tt))) * respiro
    inv = np.clip(tt / 0.08, 0, 1) * np.clip((dur - tt) / 0.25, 0, 1)
    return (suono + soffio * 3) * inv


def corno(f, dur=1.6):
    tt = t(dur)
    sega = signal.sawtooth(2 * np.pi * f * tt) + signal.sawtooth(2 * np.pi * f * 1.003 * tt)
    luce = np.clip(tt / 0.25, 0, 1)
    b, a = signal.butter(2, min(f * 5, 20000) / (SR / 2))
    inv = np.clip(tt / 0.15, 0, 1) * np.clip((dur - tt) / 0.4, 0, 1)
    return signal.lfilter(b, a, sega) * inv * (0.6 + 0.4 * luce)


def campanella(f, dur=2.0):
    tt = t(dur)
    s = np.sin(2 * np.pi * f * tt) + 0.4 * np.sin(2 * np.pi * f * 2.76 * tt) * np.exp(-tt * 3) + 0.2 * np.sin(2 * np.pi * f * 5.4 * tt) * np.exp(-tt * 6)
    return s * np.exp(-tt * 1.6) * np.clip(tt / 0.003, 0, 1)


def tamburo(dur=0.6):
    tt = t(dur)
    return np.sin(2 * np.pi * (90 + 60 * np.exp(-tt * 25)) * tt) * np.exp(-tt * 9)


def componi(eventi, durata, rev=(2.4, 0.3, 4000)):
    out = np.zeros(int(durata * SR))
    for inizio, suono, vol in eventi:
        i = int(inizio * SR)
        fine = min(len(out), i + len(suono))
        out[i:fine] += suono[: fine - i] * vol
    return riverbero(out, *rev)


MOTIVI = {
    # Solara, la luce: campanelle chiare che salgono (maggiore).
    "Solara": componi([(0.0, campanella(nota(76)), 0.8), (0.28, campanella(nota(79)), 0.8),
                       (0.56, campanella(nota(83)), 0.8), (0.84, campanella(nota(88), 2.6), 1.0)], 3.6),
    # Ignar, il fuoco: liuto basso e deciso, intervalli scuri (frigio), un colpo di tamburo.
    "Ignar": componi([(0.0, tamburo(), 0.9), (0.0, arpa(nota(50), 2.0, 0.994), 1.0), (0.3, arpa(nota(51), 2.0, 0.994), 0.9),
                      (0.6, arpa(nota(55), 2.0, 0.994), 0.9), (0.95, arpa(nota(50), 2.4, 0.995), 1.0)], 3.4, (1.6, 0.25, 2500)),
    # Nereia, l'acqua: arpa che scorre in un arpeggio (dorico), con molto riverbero.
    "Nereia": componi([(0.0, arpa(nota(62)), 0.8), (0.18, arpa(nota(65)), 0.8), (0.36, arpa(nota(69)), 0.8),
                       (0.54, arpa(nota(71)), 0.8), (0.72, arpa(nota(74), 2.8), 0.9)], 3.6, (3.2, 0.42, 5000)),
    # Torvald, la terra: corno profondo su quinte vuote, lento.
    "Torvald": componi([(0.0, corno(nota(43), 1.4), 0.9), (0.0, corno(nota(50), 1.4), 0.6),
                        (0.75, corno(nota(48), 1.8), 0.9), (0.75, corno(nota(55), 1.8), 0.6)], 3.2, (2.2, 0.3, 2200)),
    # Zefira, l'aria: flauto leggero che sale e resta sospeso (lidio).
    "Zefira": componi([(0.0, flauto(nota(72), 0.5), 0.8), (0.35, flauto(nota(76), 0.5), 0.8),
                       (0.7, flauto(nota(78), 0.5), 0.8), (1.05, flauto(nota(83), 1.4), 0.9)], 3.2, (2.8, 0.4, 6000)),
    # Vecchi Dei, il bosco: flauto di legno e tamburo, pentatonica antica.
    "VecchiDei": componi([(0.0, tamburo(0.8), 0.7), (0.05, flauto(nota(62), 0.6, 0.18), 0.8), (0.45, flauto(nota(64), 0.4, 0.18), 0.8),
                          (0.8, flauto(nota(67), 0.5, 0.18), 0.8), (1.0, tamburo(0.8), 0.5), (1.2, flauto(nota(62), 1.3, 0.18), 0.9)], 3.4, (2.0, 0.3, 3500)),
    # Nessuna fede: una sola corda, sobria.
    "Nessuna": componi([(0.0, arpa(nota(57), 2.6, 0.995), 0.9)], 3.0),
}
for nome, suono in MOTIVI.items():
    salva("S_Fede_" + nome, suono, 0.8)
