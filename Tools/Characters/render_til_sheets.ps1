param([string]$Plan = 'HistoricRenderPlan.json', [int]$Start = 0, [int]$Count = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$outputRoot = Join-Path $projectRoot 'Docs/Characters/TIL/VRoidLighting_2026-11-03_Final'
$renderPlan = Get-Content -LiteralPath (Join-Path $outputRoot $Plan) -Raw | ConvertFrom-Json
$profile = Join-Path $projectRoot 'Saved/CharacterLighting/AllCharacterOutline_20261103/ChromeRenderProfile'
$chrome = 'C:\Program Files\Google\Chrome\Application\chrome.exe'
$last = [Math]::Min($Start + $Count, $renderPlan.Count)
for ($i = $Start; $i -lt $last; $i++) {
    $row = $renderPlan[$i]
    $page = [Uri]::new($row.html).AbsoluteUri
    $arguments = @('--headless', '--disable-gpu', '--no-sandbox', '--hide-scrollbars', '--no-first-run', '--no-default-browser-check', '--allow-file-access-from-files', '--run-all-compositor-stages-before-draw', '--virtual-time-budget=1500', ('--user-data-dir="' + $profile + '"'), ('--window-size=' + $row.width + ',' + $row.height), ('--screenshot="' + $row.png + '"'), $page)
    $process = Start-Process -FilePath $chrome -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(30000)) { throw "Owned Chrome render timed out: $($row.name), PID $($process.Id)" }
    if (-not (Test-Path -LiteralPath $row.png)) { throw "Missing render: $($row.png)" }
    Write-Output "$($row.name): $((Get-Item -LiteralPath $row.png).Length) bytes"
}
