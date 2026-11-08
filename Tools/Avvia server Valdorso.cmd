@echo off
rem Valdorso - Avvia il server sul PC (porta 7777), con la finestra del registro.
rem Al primo avvio crea le chiavi in Saved\Server\Chiavi e legge l'archivio in Saved\Server\Archivio.
rem Se Windows chiede il permesso per la rete, consenti solo le reti private.
rem (08/11) Il server apre la valle vera (/Game/Prove/L_Valle): i giocatori ci arrivano da soli collegandosi.
start "Valdorso - server" "V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "V:\Valdorso\Valdorso.uproject" /Game/Prove/L_Valle -server -log -port=7777
