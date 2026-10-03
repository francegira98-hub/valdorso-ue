@echo off
rem Valdorso - Avvia il gioco in una finestra (parte dal menu), come lo vedra' un giocatore.
rem Per collegarsi al server: console (tasto \) e  Valdorso.Entra <nome> <password>
start "Valdorso - gioco" "V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "V:\Valdorso\Valdorso.uproject" -game -log -windowed -resx=1600 -resy=900
