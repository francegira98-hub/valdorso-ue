# Il launcher di Valdorso

Scritto da Claude il 07/11/2026. Il launcher si apre prima del gioco. Fa quattro cose:

- mostra le notizie della valle e lo stato del server (ora, tempo, coloni);
- installa il gioco e lo aggiorna;
- controlla che i file non siano rovinati;
- avvia Valdorso con l'indirizzo del server giusto.

È un programma Windows a parte, non fa parte di Unreal. Funziona con il .NET Framework 4, che c'è già in ogni Windows 10 e 11, quindi non serve installare niente.

## I file

| File | Cosa contiene |
| --- | --- |
| `src/Base.cs` | Configurazione, registro, JSON e confronto delle versioni |
| `src/Aggiornamenti.cs` | Manifesto, scaricamento che riprende da dove si era fermato, controllo SHA-256, installazione sicura, stato del mondo |
| `src/Finestra.cs` | La finestra "Oro e brace", disegnata tutta a mano |
| `Compila launcher.cmd` | Compila con il `csc.exe` di Windows. Il risultato va in `Build\Launcher` (fuori da GitHub) |
| `launcher.json` | La configurazione di esempio, da mettere accanto al launcher |
| `prepara_aggiornamento.py` | Prepara lo zip del gioco pacchettizzato e il `manifesto.json` da caricare sul sito |
| `../Generatori/launcher_immagini.py` | Crea lo sfondo (la valle al tramonto, dipinta da zero) e l'icona, e copia i caratteri in `risorse` |

## Come si prova (la prima volta)

1. Crea le immagini:
   `python Tools\Generatori\launcher_immagini.py V:\Font V:\Valdorso\Build\Launcher\risorse`
2. Compila: fai doppio clic su `Tools\Launcher\Compila launcher.cmd`. Alla fine deve dire "LAUNCHER PRONTO".
3. Apri `Build\Launcher\ValdorsoLauncher.exe`.
   - Il manifesto sul sito non c'è ancora, quindi il launcher dice "Non raggiungo il sito degli aggiornamenti". È giusto così.

## Come si pubblica una versione

1. In Unreal, pacchettizza il gioco: Piattaforme → Windows → Pacchettizza progetto, per esempio nella cartella `V:\Pacchetti`.
2. Prepara lo zip e il manifesto:
   `python Tools\Launcher\prepara_aggiornamento.py V:\Pacchetti\Windows 0.1.3 https://dove-sta-lo-zip/ --server IP:7777 --notizie notizie.json`
3. Carica i file prodotti in `Build\Aggiornamento`:
   - lo zip all'indirizzo che hai dato al passo 2;
   - `manifesto.json` all'indirizzo scritto in `launcher.json` (di serie `https://valdorso.altervista.org/launcher/manifesto.json`).

   Lo zip è grande (GB): va su un servizio adatto, per esempio le Release di GitHub o itch.io. Altervista va bene per il manifesto, che è piccolo.

## `launcher.json`

| Voce | Cosa fa |
| --- | --- |
| `manifesto` | Dove leggere il manifesto |
| `cartella_gioco` | Dove installare il gioco (relativa al launcher) |
| `server` / `stato` | Valgono più di quelli del manifesto (per le prove) |
| `sito` / `discord` | I collegamenti in basso. Se una voce è vuota, il collegamento non si vede |
| `chiudi_dopo_avvio` | `true` chiude il launcher quando parte il gioco |
| `argomenti` | Argomenti in più per il gioco |
| `caratteri_propri` | `false` usa i caratteri di Windows invece di Cinzel ed EB Garamond. Serve se nella finestra si vedono dei quadratini |

## Cosa passa al gioco

Il launcher avvia il gioco con questi argomenti:

`-ValdorsoServer=host:7777 -ValdorsoStato=http://host:7780/stato -DalLauncher`

`UValdorsoGameInstance::Init` li legge: il menu si collega a quel server e chiede lì lo stato del mondo.
