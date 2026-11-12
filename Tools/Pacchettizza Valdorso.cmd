@echo off
rem Valdorso - Pacchettizza il gioco (Windows, Development) in V:\Pacchetti. Scritto da Claude il 08/11/2026.
rem Prima chiudere Unreal e Visual Studio. Ci vuole a lungo (la prima volta anche ore): la valle e' grande.
rem Il registro completo va in V:\Valdorso\Saved\Claude\pacchetto.log, l'esito in pacchetto_esito.txt.
rem Il server non si pacchettizza: con Unreal installato dal Launcher il server dedicato si avvia con
rem "Avvia server Valdorso.cmd" (l'editor in modalita' server).
setlocal
set UE=V:\UE_5.8
set PROGETTO=V:\Valdorso\Valdorso.uproject
set USCITA=V:\Pacchetti
set REGISTRO=V:\Valdorso\Saved\Claude\pacchetto.log
set ESITO=V:\Valdorso\Saved\Claude\pacchetto_esito.txt
if not exist "V:\Valdorso\Saved\Claude" mkdir "V:\Valdorso\Saved\Claude"
echo INIZIATO %date% %time%> "%ESITO%"
title Valdorso - pacchetto in corso (non chiudere)
echo.
echo  VALDORSO - pacchetto del gioco in corso. Non chiudere questa finestra.
echo  Il registro: %REGISTRO%
echo.
call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROGETTO%" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Game/Menu/Lvl_Menu+/Game/Prove/L_Valle -stage -pak -iostore -compressed -prereqs -archive -archivedirectory="%USCITA%" -utf8output -unattended -nocompileeditor > "%REGISTRO%" 2>&1
set CODICE=%ERRORLEVEL%
if "%CODICE%"=="0" (
  echo FATTO %date% %time% codice 0>> "%ESITO%"
  title Valdorso - pacchetto FATTO
  echo  PACCHETTO FATTO: %USCITA%\Windows
  rem (12/11, v0.2.1) le scatole del gioco e quanto pesano, per Claude
  dir /-c "%USCITA%\Windows\Valdorso\Content\Paks" >> "%ESITO%"
) else (
  echo ERRORE %date% %time% codice %CODICE%>> "%ESITO%"
  title Valdorso - pacchetto con ERRORE
  echo  ERRORE %CODICE%: guarda %REGISTRO%
)
endlocal
pause >nul
