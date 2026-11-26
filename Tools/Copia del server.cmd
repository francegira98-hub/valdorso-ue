@echo off
rem Valdorso - Copia del server (26/11/2026, scritto da Claude). Fa uno zip di Saved\Server in V:\Copie\Valdorso.
rem Si puo' lanciare a mano quando si vuole, anche con il server acceso. Di notte lo lancia Windows da solo
rem (vedi "Programma copie notturne.cmd"), con la parola notte: allora non aspetta Invio alla fine.
title Valdorso - copia del server
echo.
echo  VALDORSO - copia del server
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Controlli\copia_server.ps1"
set CODICE=%ERRORLEVEL%
echo.
if "%CODICE%"=="0" (echo  FATTO. Le copie sono in V:\Copie\Valdorso) else (echo  ERRORE: guarda V:\Valdorso\Saved\Claude\copie_server.log)
if /i "%~1"=="notte" exit /b %CODICE%
pause
