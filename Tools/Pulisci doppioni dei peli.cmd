@echo off
chcp 65001 >nul
rem Valdorso - Pulisci i doppioni dei peli (14/11/2026, scritto da Claude).
rem Toglie i legami con l'"1" in fondo che l'editor non riesce a cancellare mentre sono caricati (sopracciglia del 02/11,
rem capelli del primo giro del 14/11) e la copia di sicurezza Content\Peli\_Prima. Il gioco non li usa: usa quelli in
rem Content\Peli\Nuovi. Si lancia con Unreal CHIUSO. L'elenco viene da Saved\Claude\sopracciglia.txt.
cd /d "%~dp0.."
tasklist /fi "imagename eq UnrealEditor.exe" | find /i "UnrealEditor.exe" >nul
if not errorlevel 1 (
  echo Unreal e' aperto: chiudilo e rilancia questo file.
  pause
  exit /b 1
)
set TOLTI=0
if exist "Content\Peli\Celeste\Eyebrows_M_Fine1.uasset" del /q "Content\Peli\Celeste\Eyebrows_M_Fine1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Celeste\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Celeste\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Celeste\Eyebrows_M_SlightArch1.uasset" del /q "Content\Peli\Celeste\Eyebrows_M_SlightArch1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Celeste\Eyebrows_M_Swept1.uasset" del /q "Content\Peli\Celeste\Eyebrows_M_Swept1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Celeste\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Celeste\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Dominic\Eyebrows_M_Full1.uasset" del /q "Content\Peli\Dominic\Eyebrows_M_Full1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Dominic\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Dominic\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Dominic\Eyebrows_M_Thick1.uasset" del /q "Content\Peli\Dominic\Eyebrows_M_Thick1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Dominic\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Dominic\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Dominic\Eyebrows_M_Unruly1.uasset" del /q "Content\Peli\Dominic\Eyebrows_M_Unruly1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Etta\Eyebrows_M_Fine1.uasset" del /q "Content\Peli\Etta\Eyebrows_M_Fine1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Etta\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Etta\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Etta\Eyebrows_M_SlightArch1.uasset" del /q "Content\Peli\Etta\Eyebrows_M_SlightArch1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Etta\Eyebrows_M_Swept1.uasset" del /q "Content\Peli\Etta\Eyebrows_M_Swept1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Etta\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Etta\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Jelena\Eyebrows_M_Fine1.uasset" del /q "Content\Peli\Jelena\Eyebrows_M_Fine1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Jelena\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Jelena\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Jelena\Eyebrows_M_SlightArch1.uasset" del /q "Content\Peli\Jelena\Eyebrows_M_SlightArch1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Jelena\Eyebrows_M_Swept1.uasset" del /q "Content\Peli\Jelena\Eyebrows_M_Swept1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Jelena\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Jelena\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Lorenzo\Eyebrows_M_Full1.uasset" del /q "Content\Peli\Lorenzo\Eyebrows_M_Full1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Lorenzo\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Lorenzo\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Lorenzo\Eyebrows_M_Thick1.uasset" del /q "Content\Peli\Lorenzo\Eyebrows_M_Thick1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Lorenzo\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Lorenzo\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Lorenzo\Eyebrows_M_Unruly1.uasset" del /q "Content\Peli\Lorenzo\Eyebrows_M_Unruly1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Mikel\Eyebrows_M_Full1.uasset" del /q "Content\Peli\Mikel\Eyebrows_M_Full1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Mikel\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Mikel\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Mikel\Eyebrows_M_Thick1.uasset" del /q "Content\Peli\Mikel\Eyebrows_M_Thick1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Mikel\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Mikel\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Mikel\Eyebrows_M_Unruly1.uasset" del /q "Content\Peli\Mikel\Eyebrows_M_Unruly1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Eyebrows_M_Full1.uasset" del /q "Content\Peli\Walter\Eyebrows_M_Full1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Eyebrows_M_Natural1.uasset" del /q "Content\Peli\Walter\Eyebrows_M_Natural1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Eyebrows_M_Thick1.uasset" del /q "Content\Peli\Walter\Eyebrows_M_Thick1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Eyebrows_M_Thin1.uasset" del /q "Content\Peli\Walter\Eyebrows_M_Thin1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Eyebrows_M_Unruly1.uasset" del /q "Content\Peli\Walter\Eyebrows_M_Unruly1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_L_MessyClumps1.uasset" del /q "Content\Peli\Walter\Hair_L_MessyClumps1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_BaldingStubble1.uasset" del /q "Content\Peli\Walter\Hair_S_BaldingStubble1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_BrushCut1.uasset" del /q "Content\Peli\Walter\Hair_S_BrushCut1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_BuzzCut1.uasset" del /q "Content\Peli\Walter\Hair_S_BuzzCut1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_Casual1.uasset" del /q "Content\Peli\Walter\Hair_S_Casual1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_Messy1.uasset" del /q "Content\Peli\Walter\Hair_S_Messy1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_PulledBack1.uasset" del /q "Content\Peli\Walter\Hair_S_PulledBack1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_RecedeMessy1.uasset" del /q "Content\Peli\Walter\Hair_S_RecedeMessy1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_SideSweptFringe1.uasset" del /q "Content\Peli\Walter\Hair_S_SideSweptFringe1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\Walter\Hair_S_SlickBack1.uasset" del /q "Content\Peli\Walter\Hair_S_SlickBack1.uasset" && set /a TOLTI+=1 >nul
if exist "Content\Peli\_Prima" rmdir /s /q "Content\Peli\_Prima" && echo Tolta la cartella Content\Peli\_Prima
echo Tolti %TOLTI% doppioni su 45. Ora puoi riaprire Unreal.
pause
