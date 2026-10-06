param([string]$EngineRoot = '', [ValidateSet('Editor','Game')][string]$Target = 'Editor')
$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
if (-not $EngineRoot) { $EngineRoot = Join-Path $env:ProgramFiles 'Epic Games\UE_5.8' }
$taskBuild = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $taskBuild)) { throw 'Build.bat not found. Supply -EngineRoot.' }
$taskTargetName = if ($Target -eq 'Editor') { 'IsekaiClassEditor' } else { 'IsekaiClass' }
$taskProject = Join-Path $taskProjectRoot 'IsekaiClass.uproject'
& $taskBuild $taskTargetName 'Win64' 'Development' "-Project=$taskProject" '-WaitMutex' '-NoHotReloadFromIDE'
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed: $LASTEXITCODE" }
