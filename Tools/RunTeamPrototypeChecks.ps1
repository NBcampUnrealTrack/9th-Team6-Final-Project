param(
    [string]$EngineRoot = (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'),
    [string]$PythonExe,
    [switch]$SkipBuild,
    [switch]$SkipSmoke,
    [string[]]$Maps=@("/Game/PlayerPrototype/L_PlayerPrototype","/Game/IsekaiClass/World/Maps/MainWorld/L_TestMap"),
    [int]$TimeoutSeconds = 300
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if ($TimeoutSeconds -lt 30) { throw 'TimeoutSeconds must be at least 30.' }
if (-not $PythonExe) {
    $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
    if ($pythonCommand -and $pythonCommand.Source -notlike '*WindowsApps*') { $PythonExe = $pythonCommand.Source }
    else { $PythonExe = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' }
}
if (-not (Test-Path -LiteralPath $PythonExe)) { throw 'Supply -PythonExe with a Python 3 interpreter.' }
& $PythonExe (Join-Path $projectRoot 'Tools\ValidateIds.py') --self-test
if ($LASTEXITCODE -ne 0) { throw 'ID validation failed.' }
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'BuildTeamPrototype.ps1') -EngineRoot $EngineRoot }
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'UnrealEditor-Cmd.exe was not found. Supply -EngineRoot.' }
$project = Join-Path $projectRoot 'IsekaiClass.uproject'
$runName = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
$runRoot = Join-Path $projectRoot ('Saved\TeamPrototype\' + $runName)
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null
function Invoke-CheckedProcess([string[]]$Arguments) {
    $process = Start-Process -FilePath $exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not $process.WaitForExit(1000)) {
        if ($timer.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
            $process.Kill()
            throw ('Unreal check timed out after ' + $TimeoutSeconds + ' seconds.')
        }
    }
    if ($process.ExitCode -ne 0) { throw ('Unreal check failed with exit code ' + $process.ExitCode + '. See ' + $runRoot) }
}
$common = @(('"' + $project + '"'), '-unattended', '-nop4', '-nosound')
$reportRoot = Join-Path $runRoot 'Automation'
$automationLog = Join-Path $runRoot 'Automation.log'
Invoke-CheckedProcess ($common + @('-d3d11', '-sm5', '-windowed', '-ResX=1280', '-ResY=800', '-ExecCmds="Automation RunTests Isekai."', '-TestExit="Automation Test Queue Empty"', ('-ReportExportPath="' + $reportRoot + '"'), ('-abslog="' + $automationLog + '"')))
$reportFile = Join-Path $reportRoot 'index.json'
if (-not (Test-Path -LiteralPath $reportFile)) { throw 'No automation report was produced.' }
$report = Get-Content -LiteralPath $reportFile -Raw | ConvertFrom-Json
$expected = @(
    'Isekai.Core.WorldStateRules', 'Isekai.Core.ParticipantRegistration',
    'Isekai.Core.TimeRules', 'Isekai.Core.SaveSlotsAndRoundTrip',
    'Isekai.Core.SaveRecoveryAndVersions', 'Isekai.Core.LegacySaveConversion',
    'Isekai.Core.AutoSaveSafetyAndDeferral', 'Isekai.Core.SaveDeletion', 'Isekai.Player.InventoryRules',
    'Isekai.Player.InteractionTarget', 'Isekai.Player.PIESession.PrototypeMap', 'Isekai.Player.PIESession.ExistingTestMap'
)
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $report.inProcess -gt 0) { throw ('Automation failures. See ' + $reportFile) }
foreach ($name in $expected) {
    $result = @($report.tests | Where-Object { $_.fullTestPath -eq $name })
    if ($result.Count -ne 1 -or $result[0].state -ne 'Success') { throw ('Missing or unsuccessful test: ' + $name) }
}
Write-Output ('Automation passed: ' + @($report.tests | Where-Object {$_.state -eq 'Success'}).Count + ' tests.')
if (-not $SkipSmoke) {
    foreach ($map in $Maps) {
        $mapName=($map -split '/')[-1]
        $mapRoot=Join-Path $runRoot $mapName
        New-Item -ItemType Directory -Path $mapRoot -Force | Out-Null
        foreach ($stage in @('Save','Reload')) {
            $smokeLog=Join-Path $mapRoot ($stage+'.log')
            $smokeStarted=Get-Date
            Invoke-CheckedProcess ($common + @($map,'-game','-PlayerPrototypeSmoke',('-SmokeStage='+$stage),'-d3d11','-sm5','-windowed','-ResX=1280','-ResY=800',('-abslog="'+$smokeLog+'"')))
            $logText=Get-Content -LiteralPath $smokeLog -Raw
            $marker=if ($stage -eq 'Save') {'PLAYER_SMOKE_SAVE_READY'} else {'PLAYER_SMOKE_RELOAD_PASSED'}
            if ($logText -notmatch $marker -or $logText -match 'PLAYER_SMOKE_FAILED' -or $logText -notmatch 'PLAYER_SESSION_QUIT_COMPLETE') {
                throw ('Save/restart/quit check failed. See '+$smokeLog)
            }
            $names=if ($stage -eq 'Save') {@('01_Creation.png','02_Play.png','03_Inventory.png','04_Storage.png','05_FemaleExpanded.png','06_Save.png','07_Quit.png')} else {@('08_Reload.png','09_DeleteConfirm.png','10_Quit.png')}
            foreach ($name in $names) {
                $screenshot=Join-Path $projectRoot ('Saved\PlayerSmoke\'+$name)
                if (-not (Test-Path -LiteralPath $screenshot) -or (Get-Item -LiteralPath $screenshot).LastWriteTime -lt $smokeStarted) {
                    throw ('Current smoke screenshot missing: '+$screenshot)
                }
                Copy-Item -LiteralPath $screenshot -Destination $mapRoot
            }
        }
        Write-Output ($mapName+': F input, UI slot keys, save/exit/restart/load/delete passed.')
    }
}
Write-Output ('Results: ' + $runRoot)
