param([string]$EngineRoot = 'F:\moria\UE_5.8', [switch]$Editor)
$projectRoot = Split-Path $PSScriptRoot -Parent
$link = 'F:\IsekaiPlayerLink'
if (Test-Path -LiteralPath $link) {
    $item = Get-Item -LiteralPath $link
    if ($item.LinkType -eq 'Junction' -and $item.Target -eq $projectRoot) { $projectRoot = $link }
}
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'IsekaiClass.uproject'
if (-not (Test-Path -LiteralPath $exe)) { throw 'UnrealEditor.exe was not found. Supply -EngineRoot.' }
$arguments = @(('"' + $project + '"'), '/Game/PlayerPrototype/L_PlayerPrototype')
if (-not $Editor) { $arguments += @('-game', '-d3d11', '-sm5', '-windowed', '-ResX=1280', '-ResY=800') }
Start-Process -FilePath $exe -ArgumentList $arguments
