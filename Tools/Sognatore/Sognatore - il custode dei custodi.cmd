@echo off
rem Valdorso - Quanto il Battito ha dato e cambiato negli ultimi giorni, e se e' frenato. Scritto da Claude il 13/11/2026.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" sognatore.py custode
if errorlevel 1 goto ERRORE
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
