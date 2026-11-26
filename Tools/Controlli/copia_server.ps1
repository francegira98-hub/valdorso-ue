# Valdorso - Copia del server (26/11/2026, scritto da Claude; v0.2.8 "Le fondamenta", server vero con copie notturne).
# Fa una copia di tutto Saved\Server (account, personaggi, chiavi, bacheche, stato del mondo, eventi) in un file zip
# con la data, in V:\Copie\Valdorso. Tiene le ultime 14 copie e, piu' vecchia, una copia al mese per 6 mesi.
# Se esiste Tools\Controlli\copie_seconda_cartella.txt (una riga con il percorso di una cartella, per esempio su un
# altro disco o in OneDrive), mette li' anche un doppione dello zip, con le stesse regole.
# Non tocca mai Saved\Server: copia prima in una cartella di passaggio (robocopy riprova se un file e' occupato),
# poi fa lo zip, poi lo riapre e controlla che dentro ci siano tutti i file.
# Scrive cosa ha fatto in Saved\Claude\copie_server.log. Esce con 0 se e' andata bene, 1 se no.

$sorgente = 'V:\Valdorso\Saved\Server'
$destinazione = 'V:\Copie\Valdorso'
$fileSeconda = 'V:\Valdorso\Tools\Controlli\copie_seconda_cartella.txt'
$registro = 'V:\Valdorso\Saved\Claude\copie_server.log'
$tieniUltime = 14
$tieniMesi = 6

function Scrivi([string]$testo) {
    $riga = (Get-Date -Format 'dd/MM/yyyy HH:mm:ss') + '  ' + $testo
    Write-Host ('  ' + $testo)
    try {
        $cartella = Split-Path -Parent $registro
        if (-not (Test-Path -LiteralPath $cartella)) { New-Item -ItemType Directory -Path $cartella -Force | Out-Null }
        Add-Content -LiteralPath $registro -Value $riga -Encoding UTF8
    } catch { }
}

function Sfoltisci([string]$cartella) {
    # tiene le ultime $tieniUltime copie; delle piu' vecchie tiene la prima di ogni mese, per $tieniMesi mesi
    $copie = @(Get-ChildItem -LiteralPath $cartella -Filter 'Server_*.zip' -File | Sort-Object Name -Descending)
    if ($copie.Count -le $tieniUltime) { return }
    $vecchie = @($copie | Select-Object -Skip $tieniUltime)
    $limite = (Get-Date).AddMonths(-$tieniMesi)
    $mesiTenuti = @{}
    # le piu' vecchie in ordine di data crescente: la prima di ogni mese resta
    foreach ($c in ($vecchie | Sort-Object Name)) {
        $mese = $c.Name.Substring(7, 7)   # Server_AAAA-MM-...
        $data = $c.LastWriteTime
        if ($data -ge $limite -and -not $mesiTenuti.ContainsKey($mese)) { $mesiTenuti[$mese] = $true; continue }
        try { Remove-Item -LiteralPath $c.FullName -Force; Scrivi ('Tolta la copia vecchia ' + $c.Name) } catch { Scrivi ('Non riesco a togliere ' + $c.Name) }
    }
}

$esito = 1
$passaggio = Join-Path $env:TEMP ('valdorso_copia_' + (Get-Date -Format 'yyyyMMdd_HHmmss'))
try {
    if (-not (Test-Path -LiteralPath $sorgente)) { Scrivi ('ERRORE: non trovo ' + $sorgente); exit 1 }
    if (-not (Test-Path -LiteralPath $destinazione)) { New-Item -ItemType Directory -Path $destinazione -Force | Out-Null }

    # 1) copia di passaggio (robocopy: riprova 3 volte se un file e' occupato dal server)
    & robocopy $sorgente $passaggio /E /R:3 /W:2 /NP /NFL /NDL /NJH /NJS | Out-Null
    $codice = $LASTEXITCODE
    if ($codice -ge 8) { Scrivi ('ERRORE: robocopy non ha copiato tutto (codice ' + $codice + ')'); exit 1 }
    $quanti = @(Get-ChildItem -LiteralPath $passaggio -Recurse -File).Count
    $peso = (@(Get-ChildItem -LiteralPath $passaggio -Recurse -File) | Measure-Object -Property Length -Sum).Sum
    if ($quanti -eq 0) { Scrivi 'ERRORE: la cartella del server e'' vuota, non faccio una copia vuota'; exit 1 }

    # 2) lo zip con la data
    $nome = 'Server_' + (Get-Date -Format 'yyyy-MM-dd_HHmm') + '.zip'
    $zip = Join-Path $destinazione $nome
    if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($passaggio, $zip, [System.IO.Compression.CompressionLevel]::Optimal, $false)

    # 3) lo riapro e conto i file
    $archivio = [System.IO.Compression.ZipFile]::OpenRead($zip)
    try { $dentro = @($archivio.Entries | Where-Object { $_.Name -ne '' }).Count } finally { $archivio.Dispose() }
    if ($dentro -ne $quanti) { Scrivi ('ERRORE: nello zip ci sono ' + $dentro + ' file invece di ' + $quanti); exit 1 }
    Scrivi ('Copia fatta: ' + $zip + ' (' + $quanti + ' file, ' + [math]::Round($peso / 1KB) + ' KB)')
    Sfoltisci $destinazione

    # 4) il doppione nella seconda cartella, se Fra l'ha scelta
    if (Test-Path -LiteralPath $fileSeconda) {
        $seconda = (@(Get-Content -LiteralPath $fileSeconda | Where-Object { $_.Trim() -ne '' -and -not $_.Trim().StartsWith('#') }) | Select-Object -First 1)
        if ($seconda) {
            $seconda = $seconda.Trim()
            try {
                if (-not (Test-Path -LiteralPath $seconda)) { New-Item -ItemType Directory -Path $seconda -Force | Out-Null }
                Copy-Item -LiteralPath $zip -Destination (Join-Path $seconda $nome) -Force
                if ((Get-Item -LiteralPath (Join-Path $seconda $nome)).Length -ne (Get-Item -LiteralPath $zip).Length) { throw 'misura diversa' }
                Scrivi ('Doppione fatto in ' + $seconda)
                Sfoltisci $seconda
            } catch { Scrivi ('ATTENZIONE: il doppione in ' + $seconda + ' non e'' riuscito (' + $_ + ')') }
        }
    }
    $esito = 0
}
catch { Scrivi ('ERRORE: ' + $_) }
finally {
    if (Test-Path -LiteralPath $passaggio) { Remove-Item -LiteralPath $passaggio -Recurse -Force -ErrorAction SilentlyContinue }
}
exit $esito
