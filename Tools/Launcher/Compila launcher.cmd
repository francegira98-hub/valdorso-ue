@echo off
rem Valdorso - Compila il launcher con il compilatore C# che c'e gia' in Windows (.NET Framework 4).
rem Niente da installare. Il launcher pronto va in V:\Valdorso\Build\Launcher (fuori da GitHub).
rem Prima, una volta: python Tools\Generatori\launcher_immagini.py V:\Font V:\Valdorso\Build\Launcher\risorse
cd /d V:\Valdorso
set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
set NET=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319
if not exist "%CSC%" set CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe
if not exist "%CSC%" (
    echo Non trovo csc.exe del .NET Framework 4. Strano: c'e in ogni Windows 10 e 11.
    pause
    exit /b 1
)
if not exist Build\Launcher mkdir Build\Launcher
set ICONA=
if exist Build\Launcher\risorse\icona.ico set ICONA=/win32icon:Build\Launcher\risorse\icona.ico
"%CSC%" /nologo /target:winexe /optimize+ /out:Build\Launcher\ValdorsoLauncher.exe %ICONA% /r:System.dll /r:System.Core.dll /r:System.Drawing.dll /r:System.Windows.Forms.dll /r:"%NET%\System.Web.Extensions.dll" /r:"%NET%\System.IO.Compression.dll" /r:"%NET%\System.IO.Compression.FileSystem.dll" Tools\Launcher\src\*.cs
if errorlevel 1 (
    echo.
    echo IL LAUNCHER NON SI E' COMPILATO: manda uno screenshot a Claude.
    pause
    exit /b 1
)
if not exist Build\Launcher\launcher.json copy Tools\Launcher\launcher.json Build\Launcher\launcher.json >nul
echo.
echo LAUNCHER PRONTO: Build\Launcher\ValdorsoLauncher.exe
pause
