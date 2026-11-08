@echo off
rem Valdorso - Compila il launcher con il compilatore C# che c'e gia' in Windows (.NET Framework 4).
rem Niente da installare. Il launcher pronto va in V:\Valdorso\Build\Launcher (fuori da GitHub).
rem Le immagini e i caratteri (risorse) li fa Tools\Generatori\launcher_immagini.py: se mancano, li fa da solo qui sotto.
rem Le notizie, le note, la roadmap e le Cronache (Tools\Launcher\contenuti) vanno DENTRO il launcher: senza rete si vedono lo stesso.
cd /d V:\Valdorso
set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
set NET=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319
if not exist "%CSC%" (
    set CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe
    set NET=%WINDIR%\Microsoft.NET\Framework\v4.0.30319
)
if not exist "%CSC%" (
    echo Non trovo csc.exe del .NET Framework 4. Strano: c'e in ogni Windows 10 e 11.
    pause
    exit /b 1
)
if not exist Build\Launcher mkdir Build\Launcher
if not exist Build\Launcher\risorse\immagini\cronache.jpg (
    echo Preparo le immagini e i caratteri del launcher...
    python Tools\Generatori\launcher_immagini.py V:\Font Build\Launcher\risorse
    if errorlevel 1 echo ATTENZIONE: immagini non fatte. Il launcher funziona lo stesso, ma senza figure.
)
set ICONA=
if exist Build\Launcher\risorse\icona.ico set ICONA=/win32icon:Build\Launcher\risorse\icona.ico
set C=Tools\Launcher\contenuti
set DENTRO=/resource:%C%\notizie.json,notizie.json /resource:%C%\patch.json,patch.json /resource:%C%\roadmap.json,roadmap.json /resource:%C%\cronache.json,cronache.json
"%CSC%" /nologo /target:winexe /optimize+ /out:Build\Launcher\ValdorsoLauncher.exe %ICONA% %DENTRO% /r:System.dll /r:System.Core.dll /r:System.Drawing.dll /r:System.Windows.Forms.dll /r:"%NET%\System.Web.Extensions.dll" /r:"%NET%\System.IO.Compression.dll" /r:"%NET%\System.IO.Compression.FileSystem.dll" Tools\Launcher\src\*.cs
if errorlevel 1 (
    echo.
    echo IL LAUNCHER NON SI E' COMPILATO: manda uno screenshot a Claude.
    pause
    exit /b 1
)
if not exist Build\Launcher\launcher.json copy Tools\Launcher\launcher.json Build\Launcher\launcher.json >nul
if not exist Build\Launcher\risorse\contenuti mkdir Build\Launcher\risorse\contenuti
copy /y %C%\*.json Build\Launcher\risorse\contenuti\ >nul
echo.
echo LAUNCHER PRONTO: Build\Launcher\ValdorsoLauncher.exe
pause
