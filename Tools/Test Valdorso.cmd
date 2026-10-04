@echo off
rem Valdorso - I test automatici, senza aprire l'editor: avvia Unreal senza finestra, fa i test "Valdorso" e chiude.
rem Ci mette circa un minuto (quasi tutto per avviare Unreal). Il rapporto completo va in Saved\Automation\Rapporto.
rem Prima di lanciarlo: compilare (Visual Studio, Compila soluzione, con l'editor chiuso).
cd /d V:\Valdorso
set REGISTRO=Saved\Logs\Test_Valdorso.txt
echo Test automatici di Valdorso: attendi, Unreal si sta avviando senza finestra...
echo.
"V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "V:\Valdorso\Valdorso.uproject" -ExecCmds="Automation RunTests Valdorso;Quit" -TestExit="Automation Test Queue Empty" -ReportExportPath="V:\Valdorso\Saved\Automation\Rapporto" -unattended -nullrhi -nosplash -nosound -stdout -FullStdOutLogOutput > "%REGISTRO%" 2>&1
set ESITO=%ERRORLEVEL%

rem Si contano i test finiti, verdi e rossi.
set VERDI=0
set ROSSI=0
for /f %%N in ('findstr /C:"Test Completed. Result={Success}" "%REGISTRO%" ^| find /c /v ""') do set VERDI=%%N
for /f %%N in ('findstr /C:"Test Completed. Result={Fail}" "%REGISTRO%" ^| find /c /v ""') do set ROSSI=%%N
set /a TOTALE=VERDI+ROSSI

echo Risultati:
findstr /C:"Test Completed" "%REGISTRO%"
echo.
if "%TOTALE%"=="0" (
    echo NESSUN TEST E' PARTITO ^(codice %ESITO%^). Hai compilato? Il registro completo: %REGISTRO%
) else if "%ESITO%%ROSSI%"=="00" (
    echo TUTTI E %TOTALE% I TEST SONO VERDI.
) else (
    echo %ROSSI% TEST ROSSI SU %TOTALE% ^(%VERDI% verdi, codice %ESITO%^). Il registro completo: %REGISTRO%
    findstr /C:"Error:" "%REGISTRO%"
)
echo.
pause
