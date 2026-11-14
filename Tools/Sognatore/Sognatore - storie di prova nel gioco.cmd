@echo off
rem Valdorso - Tre storie del Battito di prova per un colono (v0.2.4, scritto da Claude il 14/11/2026): le lucine della
rem palude (segui la lucina), il pane dei morti (porta, offerta, veglia), l'anguana del lago (lo spirito). Si aprono
rem appena il colono entra nella valle: servono a provare la finestra al centro dello schermo e i mattoni nuovi.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
set /p NOME=Nome del colono (come nel gioco): 
"%PY%" sognatore.py prova_gioco "%NOME%"
if errorlevel 1 goto ERRORE
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
