@echo off
rem Valdorso - Programma la copia notturna del server (26/11/2026, scritto da Claude). Da lanciare una volta sola.
rem Chiede a Windows (Utilita' di pianificazione) di lanciare "Copia del server.cmd" ogni notte alle 4:00.
rem Gira solo se il PC e' acceso e Fra ha fatto l'accesso a Windows. Per toglierla: questo file con la parola togli.
if /i "%~1"=="togli" (
  schtasks /delete /tn "Valdorso - copia notturna del server" /f
  pause
  exit /b
)
schtasks /create /tn "Valdorso - copia notturna del server" /tr "\"%~dp0Copia del server.cmd\" notte" /sc daily /st 04:00 /f
if errorlevel 1 (
  echo.
  echo  ERRORE: Windows non ha accettato la copia notturna. Manda a Claude una foto di questa finestra.
) else (
  echo.
  echo  FATTO: ogni notte alle 4:00 Windows fa la copia del server in V:\Copie\Valdorso.
  echo  Il registro delle copie: V:\Valdorso\Saved\Claude\copie_server.log
)
pause
