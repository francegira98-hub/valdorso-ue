@echo off
rem Valdorso - Segna com'e' andata una storia nel gioco (finche' il gioco non lo fa da solo). Scritto da Claude il 13/11/2026.
rem Il nome della storia e' l'id nel file o il nome del seme (per esempio l_ombroso).
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
set /p NOME=Nome del colono (oppure valle): 
set /p STORIA=Storia (id o seme): 
set /p COSA=Com'e' andata (accettata, rifiutata, scaduta, fallita, finale1, finale2; per la valle: quanti hanno aiutato): 
"%PY%" sognatore.py esito "%NOME%" "%STORIA%" "%COSA%"
if errorlevel 1 goto ERRORE
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
