@echo off
rem Valdorso - Un annuncio dello staff senza entrare nel gioco (Tools\Gazzetta\gazzetta.py). Scritto da Claude il 13/11/2026.
rem (Nel gioco si fa anche dalla finestra F9, scheda Bacheche.) Il Banditore lo proclama; Pubblica (2) lo mette sul sito.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
set TESTO=
set /p TESTO=Scrivi l'annuncio (al massimo 190 lettere) e premi Invio: 
"%PY%" gazzetta.py annuncio "%TESTO%"
pause
