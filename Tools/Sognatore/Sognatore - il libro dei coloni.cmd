@echo off
rem Valdorso - La storia di ogni colono scritta come una cronaca (Saved\Server\Sognatore\Libri). Scritto da Claude il 13/11/2026.
cd /d "%~dp0"
set PY=V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
if not exist "%PY%" set PY=python
"%PY%" sognatore.py libro
if errorlevel 1 goto ERRORE
start "" "..\..\Saved\Server\Sognatore\Libri"
pause
exit /b 0
:ERRORE
echo NON E' ANDATA: manda uno screenshot a Claude.
pause
exit /b 1
