# Il launcher di Valdorso

Scritto da Claude il 07/11/2026, rifatto ricco l'08/11/2026. Il launcher si apre prima del gioco. Fa queste cose:

- **La valle**: le notizie (con le figure), com'è la valle adesso (l'ora, il tempo, la stagione, quanti coloni ci sono, la notizia del server; "La valle dorme" se il server è spento), le voci dalla locanda, la tua versione e quella sul sito;
- **Note delle patch**: le novità e le sistemazioni di ogni versione;
- **Roadmap**: le tappe, fatte, in corso, prossime;
- **Cronache**: la leggenda dell'Orso, gli Antichi, l'alfabeto delle undici rune disegnate, le parole con un nome, i volti della valle;
- **Opzioni**: la cartella del gioco, l'indirizzo del server, chiudere il launcher quando parte il gioco, controllare i file, i caratteri di Windows;
- il pulsante grande: **INSTALLA** (con quanto pesa) → **PAUSA**/**RIPRENDI** mentre scarica (con velocità e tempo che manca) → **GIOCA**. Prima di scaricare controlla che sul disco ci sia posto. Se c'è una versione nuova dice **AGGIORNA**;
- se sul sito c'è un launcher più nuovo, lo dice con una fascia in alto (non si sostituisce da solo).

È un programma Windows a parte, non fa parte di Unreal. Funziona con il .NET Framework 4, che c'è già in ogni Windows 10 e 11, quindi non serve installare niente.

## Dove sta cosa

- **Il gioco** (qualche GB) va nelle **Release di GitHub**, diviso in parti da 1,9 GB al massimo (GitHub ne accetta 2 per file): `Valdorso_0.1.3.zip.001`, `.002`... Il launcher le scarica una per una, riprende da dove si era fermato, controlla ogni parte (impronta SHA-256) e poi le legge una dopo l'altra come un file solo per installare.
  **Il repository delle Release deve essere pubblico**, se no i giocatori non possono scaricare. Se il repository di Valdorso è privato, fanne uno pubblico solo per le Release (per esempio `valdorso-rilasci`, vuoto).
- **Tutto il resto** (manifesto, notizie, note, roadmap, Cronache, figure, la pagina web e lo zip del launcher) va su **Altervista**, nella cartella `/launcher/`: https://valdorso.altervista.org/launcher/ . Sono file piccoli. Il resto del sito non si tocca.
- **Il server**, per ora, è il tuo PC (`Tools\Avvia server Valdorso.cmd`, porta 7777; lo stato del mondo è sulla porta 7780). L'indirizzo si scrive in `Tools\Pubblica Valdorso.cmd` (riga `SERVER=`) quando sarà il momento. Finché non c'è, il launcher dice "La valle dorme".

## Pubblicare una versione (per esempio domani, la 0.1.3)

1. **Pacchettizza** il gioco: doppio clic su `Tools\Pacchettizza Valdorso.cmd`. Il gioco finisce in `V:\Pacchetti\Windows`.
   Se hai cambiato il launcher, compilalo: doppio clic su `Tools\Launcher\Compila launcher.cmd` (deve dire "LAUNCHER PRONTO").
2. **Prepara**: doppio clic su `Tools\Pubblica Valdorso.cmd`, scrivi `1` e premi Invio; alla domanda sulla versione premi solo Invio (prende quella del gioco).
   La prima volta apri prima `Pubblica Valdorso.cmd` con il Blocco note e scrivi nella riga `RILASCI=` il tuo indirizzo, per esempio `https://github.com/TUONOME/valdorso-rilasci/releases/download`.
   Alla fine si aprono due cartelle:
   - `Build\Aggiornamento\0.1.3`: le parti del gioco, `Carica su GitHub.cmd` e `note_github.md`;
   - `Build\Altervista\launcher`: tutto quello che va sul sito.
3. **Carica le parti su GitHub**, in uno dei due modi:
   - dal browser: apri `https://github.com/TUONOME/valdorso-rilasci/releases/new`, nella casella "Choose a tag" scrivi `v0.1.3` e scegli "Create new tag", come titolo `Valdorso 0.1.3`, incolla il testo di `note_github.md`, poi **trascina tutti i file `Valdorso_0.1.3.zip.0..`** nel riquadro in basso, aspetta che finiscano e premi **Publish release**;
   - oppure, se hai `gh` (GitHub CLI) collegato con `gh auth login`: doppio clic su `Carica su GitHub.cmd` in quella cartella.
   Il nome della release (il tag) deve essere proprio `v0.1.3`: è scritto negli indirizzi del manifesto.
4. **Carica il sito**: apri FileZilla, collegati ad Altervista, a destra entra nella cartella `launcher` (se non c'è, creala), a sinistra apri `V:\Valdorso\Build\Altervista\launcher` e trascina **tutto il suo contenuto** a destra. Se chiede, scegli "Sovrascrivi".
   Va caricato **dopo** le parti di GitHub: appena il manifesto è sul sito, i launcher dei giocatori vedono la versione nuova.
5. **Prova il launcher**: apri `Build\Launcher\ValdorsoLauncher.exe` (o, meglio, scarica `https://valdorso.altervista.org/launcher/ValdorsoLauncher.zip` in un'altra cartella, come farebbe un amico). Deve dire "INSTALLA" con la misura giusta; premi, aspetta, poi "GIOCA". Apri anche https://valdorso.altervista.org/launcher/ nel browser: è la pagina "Valdorso — il launcher" con il pulsante per scaricarlo.
   Se qualcosa non va: il pulsante "Registro" in basso apre `launcher.log`: mandalo a Claude.

## Cambiare solo notizie, note, roadmap o Cronache

1. Modifica i file in `Tools\Launcher\contenuti` con il Blocco note (sono in JSON: attenzione alle virgole e alle virgolette; in cima a ogni file c'è `_come`, che spiega i campi):
   - `notizie.json`: le notizie, dalla più nuova. `immagine` è un file della cartella `immagini/` del sito (le figure nuove si mettono in `Tools\Launcher\contenuti\immagini`); `link` può essere un indirizzo web oppure `#patch`, `#roadmap`, `#cronache`. `voci` sono le frasi che girano nel riquadro "Voci dalla locanda".
   - `patch.json`: le note di ogni versione (gruppi "Novità..." e "Sistemazioni").
   - `roadmap.json`: le tappe; `stato` è `fatta`, `in_corso`, `prossima` o `futura`.
   - `cronache.json`: i racconti, le parole delle rune, i volti.
2. Doppio clic su `Tools\Pubblica Valdorso.cmd`, scrivi `2` e premi Invio.
3. Con FileZilla trascina il contenuto di `Build\Altervista\launcher` nella cartella `launcher` del sito.

I launcher dei giocatori prendono i testi nuovi la prossima volta che si aprono. Una copia dei testi è anche **dentro** il launcher (la mette `Compila launcher.cmd`): così, anche senza rete, le schede sono piene. Si usa sempre la copia più recente (il campo `aggiornato`, che mette `prepara_aggiornamento.py`).

## Mettere l'indirizzo del server

Quando il server sarà acceso per gli altri: in `Tools\Pubblica Valdorso.cmd` scrivi `SERVER=` con l'indirizzo (per esempio `SERVER=93.40.12.7:7777`), poi fai il passo "solo il sito" qui sopra (scrivi `2`). Il launcher lo passa al gioco e chiede lo stato del mondo a `http://indirizzo:7780/stato`. Sul router vanno aperte le porte 7777 (UDP, il gioco) e 7780 (TCP, lo stato).

## I file

| File | Cosa contiene |
| --- | --- |
| `src/Base.cs` | Configurazione (si salva dalle Opzioni), registro, JSON, versioni, misure e date in italiano |
| `src/Aggiornamenti.cs` | Manifesto, scaricamento a parti con ripresa (anche dopo i rinvii di GitHub), pausa, controllo SHA-256, spazio sul disco, installazione sicura, stato del mondo |
| `src/Contenuti.cs` | Notizie, note delle patch, roadmap e Cronache: dal sito, dalla cache o da dentro il launcher |
| `src/Finestra.cs` | La finestra "Oro e brace", il titolo, le schede, la barra in basso, il pulsante grande |
| `src/Pagine.cs` | Le cinque schede, disegnate a mano |
| `src/Rune.cs` | La forma delle rune. **Generato** da `Tools/Generatori/alfabeto_rune.py` (non toccarlo a mano) |
| `contenuti/*.json` | I testi delle schede (vedi sopra) |
| `Compila launcher.cmd` | Compila con il `csc.exe` di Windows. Il risultato va in `Build\Launcher` (fuori da GitHub). Se mancano le figure, le fa |
| `launcher.json` | La configurazione di serie, da mettere accanto al launcher |
| `prepara_aggiornamento.py` | Fa le parti del gioco, il manifesto, la cartella per Altervista con la pagina web e lo zip del launcher |
| `../Pubblica Valdorso.cmd` | Lancia `prepara_aggiornamento.py` con le scelte giuste |
| `../Generatori/launcher_immagini.py` | Lo sfondo, l'icona, le figure delle notizie e delle Cronache; copia i caratteri in `risorse` |

## `launcher.json`

Le Opzioni del launcher lo riscrivono da sole; a mano serve di rado.

| Voce | Cosa fa |
| --- | --- |
| `manifesto` | Dove leggere il manifesto |
| `contenuti` | Dove leggere notizie, note, roadmap e Cronache (vuoto: nella stessa cartella del manifesto) |
| `cartella_gioco` | Dove installare il gioco (relativa al launcher) |
| `server` / `stato` | Valgono più di quelli del manifesto (per le prove) |
| `sito` / `discord` | I collegamenti in basso. Se una voce è vuota, il collegamento non si vede |
| `chiudi_dopo_avvio` | `true` chiude il launcher quando parte il gioco |
| `argomenti` | Argomenti in più per il gioco |
| `caratteri_propri` | `false` usa i caratteri di Windows invece di Cinzel ed EB Garamond. Serve se nella finestra si vedono dei quadratini |

Se la cartella del launcher non si può scrivere (per esempio dentro "Programmi"), le opzioni vanno in `%LOCALAPPDATA%\Valdorso\launcher_opzioni.json` e la cache lì accanto.

## Cosa passa al gioco

Il launcher avvia il gioco con questi argomenti:

`-ValdorsoServer=host:7777 -ValdorsoStato=http://host:7780/stato -DalLauncher`

`UValdorsoGameInstance::Init` li legge: il menu si collega a quel server e chiede lì lo stato del mondo.

## Comodità

- La rotella del mouse (o Pagina su / Pagina giù) fa scorrere le schede lunghe; Ctrl+1...5 cambia scheda; Invio preme il pulsante grande.
- `ValdorsoLauncher.exe --scheda=roadmap` apre il launcher su una scheda.
- La cartella del gioco si sostituisce tutta a ogni aggiornamento: per questo il launcher non installa mai in una cartella che contiene altre cose (se la scegli, ci mette dentro una cartella `Valdorso`). Le tue impostazioni (la cartella `Saved` del gioco) passano sempre nella versione nuova.
