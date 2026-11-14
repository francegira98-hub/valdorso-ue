@echo off
rem Valdorso - La bozza della Gazzetta del passo dagli ultimi 7 giorni (Tools\Gazzetta\gazzetta.py). Scritto da Claude il 13/11/2026.
rem Poi si apre nel Blocco note: correggila, salva, chiudi, e lancia "Gazzetta 2 - approva".
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" gazzetta.py componi 7
if errorlevel 1 goto ERRORE
start "" notepad "..\..\Saved\Server\Gazzetta\Bozza.txt"
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
