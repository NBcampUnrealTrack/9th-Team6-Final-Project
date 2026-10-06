param(
    [string]$EngineRoot = (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'),
    [switch]$Editor,
    [switch]$Visible
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'IsekaiClass.uproject'
if (-not (Test-Path -LiteralPath $exe)) { throw 'UnrealEditor.exe was not found. Supply -EngineRoot.' }
$arguments = @(('"' + $project + '"'), '/Game/PlayerPrototype/L_PlayerPrototype')
if (-not $Editor) { $arguments += @('-game', '-d3d11', '-sm5', '-windowed', '-ResX=1280', '-ResY=800') }
$windowStyle = if ($Visible) { 'Normal' } else { 'Hidden' }
Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle $windowStyle
Write-Output ('Opened prototype: ' + $project)
