@echo off
rem Valdorso - Avvia il server E crea subito un codice d'invito (12/11/2026, Claude).
rem La finestra del server non accetta comandi scritti: il comando parte da solo appena la valle e' caricata.
rem Il codice compare nella finestra nera, in una riga "Invito creato: VALD-...". Vale 7 giorni e una volta sola.
rem Se la finestra scorre troppo: lo stesso codice e' anche in V:\Valdorso\Saved\Logs\Valdorso.log (cerca "Invito creato").
rem (12/11 sera) net.SkipMissingLevelDisconnect=1: il server (editor) e il gioco pacchettizzato danno nomi diversi ai pezzi della valle (World Partition); senza questo il server butta fuori chi entra dal launcher ("connessione interrotta").
start "Valdorso - server" "V:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "V:\Valdorso\Valdorso.uproject" /Game/Prove/L_Valle -server -log -port=7777 -dpcvars=net.SkipMissingLevelDisconnect=1 -ExecCmds="net.SkipMissingLevelDisconnect 1, Valdorso.Invito.Crea 7 amico"
