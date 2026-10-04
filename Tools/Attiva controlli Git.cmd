@echo off
rem Valdorso - Attiva il controllo prima del commit (una volta sola per questo PC).
rem Da qui in poi ogni "git commit" passa da .githooks\pre-commit, che chiama Tools\Controlli\prima_del_commit.py.
cd /d V:\Valdorso
git config core.hooksPath .githooks
if errorlevel 1 (
    echo Non sono riuscito ad attivare il controllo: Git e' installato?
    pause
    exit /b 1
)
echo Controllo prima del commit attivato.
echo.
echo Prova generale su tutto il repository:
"V:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" Tools\Controlli\prima_del_commit.py --tutto
echo.
pause
