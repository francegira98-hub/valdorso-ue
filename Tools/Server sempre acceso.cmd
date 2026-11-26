@echo off
rem Valdorso - Server sempre acceso (26/11/2026, scritto da Claude; v0.2.8 "Le fondamenta").
rem Avvia il server come "Avvia server Valdorso.cmd" e, se si chiude da solo (un crash), fa una copia del server
rem e lo riaccende dopo 20 secondi. Se si chiude 5 volte in 15 minuti si ferma e lo dice: c'e' un difetto da guardare.
rem Ogni chiusura va in V:\Valdorso\Saved\Claude\server_guardiano.log (Claude lo legge da li').
rem PER SPEGNERE DAVVERO: chiudi PRIMA questa finestra (la X), POI la finestra del server.
rem Se chiudi solo il server, questa finestra lo riaccende: e' fatta apposta.
setlocal EnableDelayedExpansion
set REG=V:\Valdorso\Saved\Claude\server_guardiano.log
if not exist "V:\Valdorso\Saved\Claude" mkdir "V:\Valdorso\Saved\Claude"
title Valdorso - guardiano del server (chiudi QUESTA per prima)
set CADUTE=0
set PRIMA=
:giro
title Valdorso - guardiano del server (chiudi QUESTA per prima)
>> "%REG%" echo %date% %time% server acceso
echo.
echo  VALDORSO - guardiano del server. Server acceso alle %time:~0,5%.
echo  Per spegnere davvero: chiudi PRIMA questa finestra, POI quella del server.
"V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "V:\Valdorso\Valdorso.uproject" /Game/Prove/L_Valle -server -log -port=7777 -dpcvars=net.SkipMissingLevelDisconnect=1 -ExecCmds="net.SkipMissingLevelDisconnect 1"
set CODICE=!ERRORLEVEL!
>> "%REG%" echo !date! !time! server chiuso, codice !CODICE!
echo  Il server si e' chiuso (codice !CODICE!). Faccio una copia e lo riaccendo tra 20 secondi...
call "%~dp0Copia del server.cmd" notte
rem conto le cadute degli ultimi 15 minuti (minuti dalla mezzanotte; a cavallo della mezzanotte si riparte da zero)
for /f "tokens=1,2 delims=:" %%a in ("!time: =0!") do set /a ORA=1%%a-100, MIN=1%%b-100
set /a ADESSO=ORA*60+MIN
if not defined PRIMA set PRIMA=!ADESSO!
set /a PASSATI=ADESSO-PRIMA
if !PASSATI! LSS 0 (set PRIMA=!ADESSO!& set CADUTE=0& set PASSATI=0)
if !PASSATI! GTR 15 (set PRIMA=!ADESSO!& set CADUTE=0)
set /a CADUTE+=1
if !CADUTE! GEQ 5 (
  >> "%REG%" echo !date! !time! FERMO: 5 chiusure in 15 minuti
  title Valdorso - guardiano FERMO
  echo.
  echo  FERMO: il server si e' chiuso 5 volte in 15 minuti. Dillo a Claude: legge lui i registri.
  pause
  exit /b 1
)
timeout /t 20 /nobreak >nul
goto giro
