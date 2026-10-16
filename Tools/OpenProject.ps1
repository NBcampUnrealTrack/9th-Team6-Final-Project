param(
    [string]$EngineRoot = (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'),
    [string]$Map,
    [switch]$Game,
    [switch]$Visible
)
$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
$taskEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$taskProject = Join-Path $taskProjectRoot 'IsekaiClass.uproject'
if (-not (Test-Path -LiteralPath $taskEditor)) { throw 'UnrealEditor.exe was not found. Supply -EngineRoot.' }
$taskArguments = @(('"' + $taskProject + '"'))
if ($Map) { $taskArguments += $Map }
if ($Game) { $taskArguments += @('-game', '-windowed', '-ResX=1280', '-ResY=800') }
$taskWindowStyle = if ($Visible) { 'Normal' } else { 'Hidden' }
Start-Process -FilePath $taskEditor -ArgumentList $taskArguments -WindowStyle $taskWindowStyle
Write-Output ('Opened project: ' + $taskProject)
