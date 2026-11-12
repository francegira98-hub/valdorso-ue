@echo off
rem Valdorso - Mette in Content\Python\usati gli script gia' usati una volta e che non servono piu' tutti i giorni.
rem Scritto da Claude il 12/11/2026 (chiesto da Fra). Usa git mv, cosi' la storia dei file resta. Poi si fa il push.
rem Restano in nuovi: quelli del menu Valdorso e ripara_corde.py (lo usa Ripara e pacchettizza Valdorso.cmd).
cd /d V:\Valdorso
if exist "Content\Python\nuovi\crea_acque_nuoto.py" git mv "Content/Python/nuovi/crea_acque_nuoto.py" "Content/Python/usati/crea_acque_nuoto.py"
if exist "Content\Python\nuovi\crea_mappa.py" git mv "Content/Python/nuovi/crea_mappa.py" "Content/Python/usati/crea_mappa.py"
if exist "Content\Python\nuovi\crea_suoni_valle.py" git mv "Content/Python/nuovi/crea_suoni_valle.py" "Content/Python/usati/crea_suoni_valle.py"
if exist "Content\Python\nuovi\crea_tempo.py" git mv "Content/Python/nuovi/crea_tempo.py" "Content/Python/usati/crea_tempo.py"
if exist "Content\Python\nuovi\crea_valle_viva.py" git mv "Content/Python/nuovi/crea_valle_viva.py" "Content/Python/usati/crea_valle_viva.py"
if exist "Content\Python\nuovi\porta_movenze.py" git mv "Content/Python/nuovi/porta_movenze.py" "Content/Python/usati/porta_movenze.py"
if exist "Content\Python\nuovi\rocce_collisione_vera.py" git mv "Content/Python/nuovi/rocce_collisione_vera.py" "Content/Python/usati/rocce_collisione_vera.py"
echo.
echo Fatto. Ecco cosa e' cambiato (deve dire renamed per ognuno):
git status --short Content/Python
echo.
echo Ora il push: git commit -m "Script vecchi in usati, menu delle armi" e git push
pause
