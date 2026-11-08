@echo off
rem Valdorso - Prima ripara le corde rotte della tenda (il pacchetto si fermava per loro), poi pacchettizza.
rem Scritto da Claude il 08/11/2026. Unreal e Visual Studio devono essere chiusi.
title Valdorso - riparo le corde della tenda
echo  VALDORSO - riparo le corde della tenda del mercato...
"V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "V:\Valdorso\Valdorso.uproject" -run=pythonscript -script="V:\Valdorso\Content\Python\nuovi\ripara_corde.py" -unattended -nosplash -nullrhi > "V:\Valdorso\Saved\Claude\ripara_corde.log" 2>&1
call "V:\Valdorso\Tools\Pacchettizza Valdorso.cmd"
