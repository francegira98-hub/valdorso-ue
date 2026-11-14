@echo off
rem Valdorso - Approva la bozza corretta della Gazzetta (Tools\Gazzetta\gazzetta.py). Scritto da Claude il 13/11/2026.
rem Il Banditore della Corona la proclama nel gioco; con Pubblica Valdorso (scelta 2) va sul sito e nel launcher.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" gazzetta.py approva
pause
