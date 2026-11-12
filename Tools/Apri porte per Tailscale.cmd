@echo off
rem Valdorso - Apre nel firewall di Windows le porte del server, SOLO per chi arriva da Tailscale (indirizzi 100.x).
rem Scritto da Claude il 12/11/2026. Da lanciare una volta sola. Chiede il permesso di amministratore (si', e' normale).
rem   UDP 7777 = il gioco    TCP 7780 = la pagina dello stato del mondo (ora, tempo, coloni) per il menu e il launcher
net session >nul 2>&1
if errorlevel 1 (
  echo Chiedo il permesso di amministratore...
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)
netsh advfirewall firewall delete rule name="Valdorso server (Tailscale)" >nul 2>&1
netsh advfirewall firewall delete rule name="Valdorso stato (Tailscale)" >nul 2>&1
netsh advfirewall firewall add rule name="Valdorso server (Tailscale)" dir=in action=allow protocol=UDP localport=7777 remoteip=100.64.0.0/10 profile=any
netsh advfirewall firewall add rule name="Valdorso stato (Tailscale)" dir=in action=allow protocol=TCP localport=7780 remoteip=100.64.0.0/10 profile=any
echo.
echo FATTO: le porte 7777 (gioco) e 7780 (stato) sono aperte solo per Tailscale.
pause
