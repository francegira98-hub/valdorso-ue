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
rem (12/11, v0.2.1) le impronte delle scatole, per vedere quali cambiano da un pacchetto all'altro
set IMPRONTE=V:\Valdorso\Saved\Claude\scatole_impronte.txt
set IMPRONTE_PRIMA=V:\Valdorso\Saved\Claude\scatole_impronte_prima.txt
set PAKS=%USCITA%\Windows\Valdorso\Content\Paks
if not exist "V:\Valdorso\Saved\Claude" mkdir "V:\Valdorso\Saved\Claude"
echo INIZIATO %date% %time%> "%ESITO%"
title Valdorso - pacchetto in corso (non chiudere)
echo.
echo  VALDORSO - pacchetto del gioco in corso. Non chiudere questa finestra.
echo  Il registro: %REGISTRO%
echo.
rem (12/11, v0.2.1) via le scatole del pacchetto di prima: Unreal nell'archivio non cancella niente, e i file
rem vecchi (come lo scatolone Valdorso-Windows.pak) finirebbero nel gioco insieme a quelli nuovi
if exist "%PAKS%" rd /s /q "%PAKS%"
call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROGETTO%" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Game/Menu/Lvl_Menu+/Game/Prove/L_Valle -stage -pak -iostore -compressed -prereqs -archive -archivedirectory="%USCITA%" -utf8output -unattended -nocompileeditor > "%REGISTRO%" 2>&1
set CODICE=%ERRORLEVEL%
if "%CODICE%"=="0" (
  echo FATTO %date% %time% codice 0>> "%ESITO%"
  title Valdorso - pacchetto FATTO
  echo  PACCHETTO FATTO: %USCITA%\Windows
  rem (12/11, v0.2.1) le scatole del gioco e quanto pesano, per Claude
  dir /-c "%PAKS%" >> "%ESITO%"
  if exist "%IMPRONTE%" copy /y "%IMPRONTE%" "%IMPRONTE_PRIMA%" >nul
  echo Impronte delle scatole %date% %time%> "%IMPRONTE%"
  for %%f in ("%PAKS%\*") do certutil -hashfile "%%f" SHA256 >> "%IMPRONTE%"
) else (
  echo ERRORE %date% %time% codice %CODICE%>> "%ESITO%"
  title Valdorso - pacchetto con ERRORE
  echo  ERRORE %CODICE%: guarda %REGISTRO%
)
endlocal
pause >nul
