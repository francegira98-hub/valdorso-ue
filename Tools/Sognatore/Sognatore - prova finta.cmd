@echo off
rem Valdorso - Un giro finto del Sognatore con tre coloni inventati: non tocca niente del server.
rem Scritto da Claude il 13/11/2026.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" sognatore.py prova
if errorlevel 1 goto ERRORE
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
