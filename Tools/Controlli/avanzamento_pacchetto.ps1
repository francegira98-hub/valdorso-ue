# Valdorso - Avanzamento del pacchetto (25/11/2026, scritto da Claude).
# Guarda soltanto: legge il registro del pacchetto ogni 5 secondi e dice a che punto e'. Non tocca niente.
# Si apre da "Avanzamento del pacchetto.cmd" (anche a pacchetto gia' partito). Si puo' chiudere quando si vuole:
# il pacchetto va avanti lo stesso.
$registro = 'V:\Valdorso\Saved\Claude\pacchetto.log'
$esito = 'V:\Valdorso\Saved\Claude\pacchetto_esito.txt'
$fasi = @(
    @('BUILD COMMAND STARTED', '1 di 4   Compilo il gioco'),
    @('COOK COMMAND STARTED', '2 di 4   Cuocio la valle (la parte lunga)'),
    @('STAGE COMMAND STARTED', '3 di 4   Riempio le scatole'),
    @('PACKAGE COMMAND STARTED', '4 di 4   Chiudo il pacchetto'),
    @('ARCHIVE COMMAND STARTED', '4 di 4   Copio il gioco in V:\Pacchetti')
)
$tempiFile = 'V:\Valdorso\Saved\Claude\pacchetto_fasi.txt'
# (26/11, v0.2.8) i minuti di ogni fase: la finestra segna l'ora in cui vede comparire ogni fase nel registro.
# Le fasi gia' passate quando la finestra si apre non hanno l'ora (si apre da sola all'inizio del pacchetto).
$inizioFase = @{}
$script:fineVista = $null
$primoGiro = $true
try { $host.UI.RawUI.WindowTitle = 'Valdorso - avanzamento del pacchetto' } catch { }

function Minuti([TimeSpan]$t) {
    if ($t.TotalMinutes -lt 1) { return 'meno di 1 min' }
    return ('{0} min' -f [int][math]::Floor($t.TotalMinutes))
}

function TempiDelleFasi([bool]$finito) {
    # una riga per fase vista: quanto e' durata (o da quanto e' in corso)
    $out = @()
    $chiavi = @($fasi | ForEach-Object { $_[0] } | Where-Object { $inizioFase.ContainsKey($_) })
    for ($i = 0; $i -lt $chiavi.Count; $i++) {
        $k = $chiavi[$i]
        $nome = $k
        foreach ($f in $fasi) { if ($f[0] -eq $k) { $nome = $f[1]; break } }
        $da = $inizioFase[$k]
        $a = $null
        if ($i + 1 -lt $chiavi.Count) { $a = $inizioFase[$chiavi[$i + 1]] } elseif ($finito) { $a = $script:fineVista } else { $a = Get-Date }
        if ($null -eq $da) { $t = 'gia'' passata quando si e'' aperta la finestra' }
        elseif ($null -eq $a) { $t = '?' }
        elseif ($i + 1 -lt $chiavi.Count -or $finito) { $t = Minuti ($a - $da) }
        else { $t = 'da ' + (Minuti ($a - $da)) }
        $out += ('{0,-44} {1}' -f $nome, $t)
    }
    return $out
}

function Barra([double]$parte) {
    if ($parte -lt 0) { $parte = 0 }
    if ($parte -gt 1) { $parte = 1 }
    $pieni = [int][math]::Floor($parte * 40)
    return ('[' + ('#' * $pieni) + ('.' * (40 - $pieni)) + '] ' + [int][math]::Floor($parte * 100) + '%')
}

while ($true) {
    $righe = @()
    if (Test-Path -LiteralPath $registro) {
        try { $righe = @(Get-Content -LiteralPath $registro -Encoding UTF8 -ErrorAction Stop) } catch { $righe = @() }
    }
    $testoEsito = ''
    if (Test-Path -LiteralPath $esito) {
        try { $testoEsito = [string]::Join("`n", @(Get-Content -LiteralPath $esito -ErrorAction Stop)) } catch { $testoEsito = '' }
    }

    $fase = 'Sto partendo...'
    $cotti = -1; $restano = -1; $totale = -1
    $fatti = -1; $daFare = -1
    $errori = 0
    $ultima = ''
    $uscita = -1
    # al primo giro le fasi gia' nel registro sono passate prima che la finestra si aprisse, tranne quando il registro
    # e' appena cominciato (la finestra aperta da sola dal pacchetto)
    $vivo = (-not $primoGiro) -or ($righe.Count -lt 60)
    foreach ($r in $righe) {
        foreach ($f in $fasi) {
            if ($r.Contains($f[0])) {
                $fase = $f[1]; $fatti = -1; $daFare = -1
                if (-not $inizioFase.ContainsKey($f[0])) { if ($vivo) { $inizioFase[$f[0]] = Get-Date } else { $inizioFase[$f[0]] = $null } }
            }
        }
        if ($r -match 'Cooked packages (\d+) Packages Remain (\d+) Total (\d+)') {
            $cotti = [int]$Matches[1]; $restano = [int]$Matches[2]; $totale = [int]$Matches[3]
        }
        elseif ($r -match '^\s*\[(\d+)/(\d+)\]') {
            $fatti = [int]$Matches[1]; $daFare = [int]$Matches[2]
        }
        if ($r -match '(: error |Error: | error C\d+)') { $errori++ }
        if ($r -match 'AutomationTool exiting with ExitCode=(-?\d+)') { $uscita = [int]$Matches[1] }
        if ($r.Trim().Length -gt 0) { $ultima = $r.Trim() }
    }
    $primoGiro = $false
    if ($ultima.Length -gt 110) { $ultima = $ultima.Substring(0, 110) + '...' }

    $passato = ''
    if ($testoEsito -match 'INIZIATO\s+(\d{1,2})/(\d{1,2})/(\d{4})\s+(\d{1,2}):(\d{2}):(\d{2})') {
        try {
            $inizio = Get-Date -Year ([int]$Matches[3]) -Month ([int]$Matches[2]) -Day ([int]$Matches[1]) -Hour ([int]$Matches[4]) -Minute ([int]$Matches[5]) -Second ([int]$Matches[6])
            $d = (Get-Date) - $inizio
            if ($d.TotalSeconds -ge 0) { $passato = ('{0} h {1:00} min' -f [int][math]::Floor($d.TotalHours), $d.Minutes) }
        } catch { $passato = '' }
    }

    $finito = $false
    Clear-Host
    Write-Host ''
    Write-Host '  VALDORSO - avanzamento del pacchetto'
    Write-Host '  ------------------------------------'
    if ($passato -ne '') { Write-Host ('  Tempo passato:  ' + $passato) }
    # la fine si legge dal registro (l'ultima riga di Unreal) oppure dal file dell'esito
    if ($uscita -eq 0 -or $testoEsito -match '(?m)^FATTO') {
        Write-Host ''
        Write-Host '  PACCHETTO FATTO. Il gioco e'' in V:\Pacchetti\Windows' -ForegroundColor Green
        $finito = $true
    }
    elseif ($uscita -gt 0 -or $uscita -lt -1 -or $testoEsito -match '(?m)^ERRORE') {
        Write-Host ''
        Write-Host '  PACCHETTO CON ERRORE. Dillo a Claude: il registro lo legge lui.' -ForegroundColor Red
        $finito = $true
    }
    else {
        Write-Host ('  Fase:           ' + $fase)
        if ($fase.StartsWith('1 di 4') -and $daFare -gt 0) {
            Write-Host ('  File compilati: ' + $fatti + ' di ' + $daFare)
            Write-Host ('  ' + (Barra ($fatti / $daFare)))
        }
        elseif ($fase.StartsWith('2 di 4') -and $totale -gt 0) {
            Write-Host ('  Cose cotte:     ' + $cotti + ' di ' + $totale + '  (ne restano ' + $restano + ')')
            Write-Host ('  ' + (Barra ($cotti / $totale)))
            Write-Host '  (il totale puo'' crescere mentre cuoce: e'' normale)'
        }
        elseif ($fase.StartsWith('2 di 4')) {
            Write-Host '  Sta caricando la valle: i numeri arrivano tra poco.'
        }
        if ($errori -gt 0) { Write-Host ('  Righe con errori viste finora: ' + $errori) -ForegroundColor Yellow }
        Write-Host ''
        Write-Host '  Ultima riga del registro:'
        Write-Host ('  ' + $ultima) -ForegroundColor DarkGray
        Write-Host ''
        Write-Host '  Si aggiorna da solo ogni 5 secondi. Puoi chiudere questa finestra: il pacchetto va avanti.'
    }
    if ($finito -and $null -eq $script:fineVista) { $script:fineVista = Get-Date }
    $tempi = @(TempiDelleFasi $finito)
    if ($tempi.Count -gt 0) {
        Write-Host ''
        Write-Host '  Minuti delle fasi:'
        foreach ($t in $tempi) { Write-Host ('  ' + $t) }
    }
    if ($finito) {
        # si scrive solo se la finestra ha visto davvero almeno una fase (aperta dopo la fine non sa i tempi)
        $visteDavvero = @($inizioFase.Values | Where-Object { $null -ne $_ }).Count
        if ($visteDavvero -gt 0) { try { @(('Minuti delle fasi del pacchetto, ' + (Get-Date -Format 'dd/MM/yyyy HH:mm'))) + $tempi | Set-Content -LiteralPath $tempiFile -Encoding UTF8 } catch { } }
        break
    }
    Start-Sleep -Seconds 5
}
Write-Host ''
Write-Host '  Premi Invio per chiudere.'
[void][Console]::ReadLine()
