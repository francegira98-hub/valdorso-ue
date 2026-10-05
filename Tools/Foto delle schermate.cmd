@echo off
rem Valdorso - Le foto delle schermate (05/10/2026): avvia Unreal con la grafica ma senza finestra, misura tutte le
rem schermate come Test Valdorso.cmd e in piu' le salva come immagini in Saved\Schermate (1280x720, 1920x1080, 2560x1080).
rem Alla fine apre la cartella. Ci mette un paio di minuti. Prima di lanciarlo: compilare, con l'editor chiuso.
cd /d V:\Valdorso
set REGISTRO=Saved\Logs\Foto_Schermate.txt
if exist "Saved\Schermate\*.png" del /q "Saved\Schermate\*.png"
echo Foto delle schermate di Valdorso: attendi, Unreal si sta avviando senza finestra...
echo.
"V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "V:\Valdorso\Valdorso.uproject" -ExecCmds="Automation RunTests Valdorso.Schermate;Quit" -TestExit="Automation Test Queue Empty" -unattended -RenderOffscreen -ValdorsoFoto -nosplash -nosound -stdout -FullStdOutLogOutput > "%REGISTRO%" 2>&1

echo Risultati:
findstr /C:"Test Completed" "%REGISTRO%"
echo.
echo Le foto e i rapporti sono in V:\Valdorso\Saved\Schermate
start "" "V:\Valdorso\Saved\Schermate"
echo.
pause
