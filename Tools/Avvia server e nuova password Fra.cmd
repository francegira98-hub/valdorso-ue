@echo off
rem Valdorso - Avvia il server E da' una password temporanea all'account Fra (12/11/2026, Claude).
rem Nella finestra nera compare la riga "Password temporanea per Fra: XXXX-XXXX-XXXX".
rem Si entra con quella; il gioco chiede subito di sceglierne una nuova. Se la riga e' sparita in alto: V:\Valdorso\Saved\Logs\Valdorso.log, cerca "Password temporanea".
rem Da usare solo quando serve: ogni volta che lo apri, la password di Fra cambia.
start "Valdorso - server" "V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "V:\Valdorso\Valdorso.uproject" /Game/Prove/L_Valle -server -log -port=7777 -dpcvars=net.SkipMissingLevelDisconnect=1 -ExecCmds="net.SkipMissingLevelDisconnect 1, Valdorso.Account.ReimpostaPassword Fra"
