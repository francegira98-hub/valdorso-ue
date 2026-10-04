@echo off
rem Valdorso - I test automatici, senza aprire l'editor: avvia Unreal senza finestra, fa i test "Valdorso" e chiude.
rem Ci mette circa un minuto (quasi tutto per avviare Unreal). Il rapporto completo va in Saved\Automation\Rapporto.
cd /d V:\Valdorso
echo Test automatici di Valdorso: attendi, Unreal si sta avviando senza finestra...
echo.
"V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "V:\Valdorso\Valdorso.uproject" -ExecCmds="Automation RunTests Valdorso;Quit" -TestExit="Automation Test Queue Empty" -ReportExportPath="V:\Valdorso\Saved\Automation\Rapporto" -unattended -nullrhi -nosplash -nosound -stdout -FullStdOutLogOutput > "Saved\Logs\Test_Valdorso.txt" 2>&1
set ESITO=%ERRORLEVEL%
echo Risultati:
findstr /C:"Test Completed" "Saved\Logs\Test_Valdorso.txt"
echo.
if "%ESITO%"=="0" (
    echo TUTTI I TEST SONO VERDI.
) else (
    echo QUALCHE TEST NON E' PASSATO ^(codice %ESITO%^). Il registro completo: Saved\Logs\Test_Valdorso.txt
    findstr /C:"Error:" "Saved\Logs\Test_Valdorso.txt"
)
echo.
pause
