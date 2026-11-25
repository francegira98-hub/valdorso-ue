@echo off
rem Valdorso - Avanzamento del pacchetto (25/11/2026, scritto da Claude). Guarda soltanto: si puo' aprire e chiudere
rem quando si vuole, anche a pacchetto gia' partito. Legge V:\Valdorso\Saved\Claude\pacchetto.log.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Controlli\avanzamento_pacchetto.ps1"
