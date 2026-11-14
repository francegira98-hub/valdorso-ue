@echo off
rem Valdorso - Pubblica una versione per il launcher. Scritto da Claude l'08/11/2026.
rem Prepara il gioco a parti (da caricare nelle Release di GitHub) e la cartella del sito (da caricare su Altervista).
rem Prima: "Tools\Pacchettizza Valdorso.cmd" (il gioco va in V:\Pacchetti\Windows) e "Tools\Launcher\Compila launcher.cmd".
rem I passi uno per uno sono in Tools\Launcher\LEGGIMI.md.

rem ===== DA SCRIVERE UNA VOLTA SOLA =====
rem Dove stanno le Release su GitHub: https://github.com/UTENTE/REPOSITORY/releases/download
rem (il repository deve essere PUBBLICO, se no il launcher dei giocatori non puo' scaricare le parti)
set RILASCI=https://github.com/francegira98-hub/valdorso-ue/releases/download
rem L'indirizzo del server per i giocatori, per esempio 93.40.12.7:7777. Vuoto = resta quello di prima
rem (la prima volta nessuno: il launcher dice "La valle dorme").
rem (12/11) il PC di Fra dentro Tailscale (desktop-s3mdko4): lo raggiunge solo chi ha la condivisione
set SERVER=100.124.215.111:7777
rem =====================================

cd /d V:\Valdorso
rem (08/11) il Python che viene con Unreal: c'e' sempre, e a questo script basta (usa solo la libreria standard).
rem Il "python" di Windows senza installazione apre solo il Microsoft Store.
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
set SERVERARG=
if not "%SERVER%"=="" set SERVERARG=--server %SERVER%

echo.
echo   1 = una versione nuova: il gioco a parti per GitHub e la cartella del sito
echo   2 = solo il sito: notizie, note delle patch, roadmap, Cronache (il gioco resta quello di prima)
echo.
set SCELTA=
set /p SCELTA=Cosa preparo? Scrivi 1 o 2 e premi Invio: 
if "%SCELTA%"=="2" goto SITO
if not "%SCELTA%"=="1" (
    echo Scrivi 1 o 2.
    pause
    exit /b 1
)

echo %RILASCI% | find "CAMBIAMI" >nul
if not errorlevel 1 (
    echo.
    echo Prima apri questo file con il Blocco note e scrivi in RILASCI il tuo utente e repository di GitHub.
    pause
    exit /b 1
)
if not exist V:\Pacchetti\Windows\Valdorso.exe (
    echo.
    echo Non trovo V:\Pacchetti\Windows\Valdorso.exe: prima pacchettizza il gioco con "Tools\Pacchettizza Valdorso.cmd".
    pause
    exit /b 1
)
if not exist Build\Launcher\ValdorsoLauncher.exe (
    echo.
    echo Non trovo il launcher compilato: prima fai doppio clic su "Tools\Launcher\Compila launcher.cmd".
    pause
    exit /b 1
)
set VERSIONE=
set /p VERSIONE=Che versione e'? (Invio = quella scritta nel gioco, ValdorsoVersione.h): 
if "%VERSIONE%"=="" set VERSIONE=auto
"%PY%" Tools\Launcher\prepara_aggiornamento.py V:\Pacchetti\Windows %VERSIONE% %RILASCI%/v{versione}/ %SERVERARG%
if errorlevel 1 goto ERRORE
rem (14/11) le parti vanno su GitHub da sole: si lancia "Carica su GitHub.cmd" della versione appena preparata
set CARICA=
for /f "usebackq delims=" %%P in (`%PY% -c "import glob,os;c=glob.glob(r'Build\Aggiornamento\*\Carica su GitHub.cmd');print(max(c,key=os.path.getmtime) if c else '')"`) do set CARICA=%%P
start "" explorer Build\Altervista
if "%CARICA%"=="" goto MANO
echo.
set SU=
set /p SU=Carico adesso le parti su GitHub? (S = si, Invio = no): 
if /i not "%SU%"=="S" goto MANO
call "%CARICA%"
if errorlevel 1 goto MANO
echo.
echo PRONTO. Adesso con FileZilla trascina Build\Altervista\launcher nella cartella /launcher/ del sito.
pause
exit /b 0

:MANO
start "" explorer Build\Aggiornamento
echo.
echo PRONTO. Adesso: 1) carica le parti su GitHub ("Carica su GitHub.cmd" nella cartella della versione;
echo                    la prima volta serve "Tools\Installa GitHub CLI.cmd")
echo                 2) con FileZilla trascina Build\Altervista\launcher nella cartella /launcher/ del sito.
pause
exit /b 0

:SITO
"%PY%" Tools\Launcher\prepara_aggiornamento.py --solo-sito %SERVERARG%
if errorlevel 1 goto ERRORE
start "" explorer Build\Altervista
echo.
echo PRONTO. Con FileZilla trascina Build\Altervista\launcher nella cartella /launcher/ del sito.
pause
exit /b 0

:ERRORE
echo.
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
