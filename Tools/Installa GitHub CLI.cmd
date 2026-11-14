@echo off
rem Valdorso - Installa GitHub CLI (gh) e lo collega al tuo GitHub. Scritto da Claude il 14/11/2026.
rem Serve una volta sola: dopo, "Pubblica Valdorso.cmd" carica da solo le parti del gioco nella Release di GitHub
rem (prima andavano trascinate a mano dal browser, perche' su questo PC gh non c'era).
rem Il collegamento si fa nel browser: GitHub ti mostra un codice, tu lo confermi. Nessuna password passa da qui.
set GH=gh
if exist "%ProgramFiles%\GitHub CLI\gh.exe" set GH="%ProgramFiles%\GitHub CLI\gh.exe"
%GH% --version >nul 2>nul
if not errorlevel 1 goto COLLEGA
echo Installo GitHub CLI con winget (Windows potrebbe chiederti il permesso)...
winget install --id GitHub.cli -e --source winget --accept-package-agreements --accept-source-agreements
if exist "%ProgramFiles%\GitHub CLI\gh.exe" set GH="%ProgramFiles%\GitHub CLI\gh.exe"
%GH% --version >nul 2>nul
if errorlevel 1 (
    echo.
    echo Non sono riuscito a installarlo. Scaricalo da https://cli.github.com e installalo, poi rilancia questo file.
    pause
    exit /b 1
)
:COLLEGA
%GH% auth status >nul 2>nul
if not errorlevel 1 goto FATTO
echo.
echo Adesso si apre il browser: accedi a GitHub con l'account di Valdorso e conferma il codice che vedi qui sotto.
%GH% auth login --hostname github.com --git-protocol https --web
%GH% auth status >nul 2>nul
if errorlevel 1 (
    echo.
    echo Il collegamento non e' riuscito: rilancia questo file.
    pause
    exit /b 1
)
:FATTO
echo.
echo FATTO: gh e' installato e collegato a GitHub. Da ora Pubblica Valdorso.cmd carica le parti da solo.
pause
