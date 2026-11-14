@echo off
rem Valdorso - Il Sognatore scrive 3 storie personali per ogni colono passato dalla valle negli ultimi 30 giorni
rem (Tools\Sognatore\sognatore.py). Scritto da Claude il 13/11/2026. Poi si apre la bozza nel Blocco note: leggi,
rem scrivi no dove non ti piace, correggi se vuoi, salva, chiudi e lancia "Sognatore 2 - approva".
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" sognatore.py sogna 3
if errorlevel 1 goto ERRORE
start "" notepad "..\..\Saved\Server\Sognatore\Bozza.txt"
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
