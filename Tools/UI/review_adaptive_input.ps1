param([string]$EngineRoot = (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'), [switch]$IncludeGameplayChecks, [switch]$SaveOnly, [switch]$InputOnly)
$ErrorActionPreference = 'Stop'
$reviewRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$reviewEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$reviewProject = Join-Path $reviewRoot 'IsekaiClass.uproject'
$reviewOutput = Join-Path $reviewRoot 'Saved\AdaptiveInputUI_20261105'
if (-not (Test-Path -LiteralPath $reviewEditor)) { throw 'UnrealEditor.exe was not found.' }
$reviewAlreadyRunning = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match '^UnrealEditor(?:-Cmd)?\.exe$' -and $_.CommandLine -like ('*' + $reviewProject + '*')
}
if ($reviewAlreadyRunning) { throw 'Close this project editor before running the isolated input review.' }
New-Item -ItemType Directory -Path $reviewOutput -Force | Out-Null
$reviewPrefsBackup = Join-Path $reviewOutput ('PreferencesBefore-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $reviewPrefsBackup -Force | Out-Null
$reviewPreferences = @(
    'Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini',
    'Saved\Config\WindowsEditor\GameUserSettings.ini',
    'Saved\Config\Windows\GameUserSettings.ini'
)
$reviewPreferenceManifest = @()
foreach ($reviewRelative in $reviewPreferences) {
    $reviewOriginal = Join-Path $reviewRoot $reviewRelative
    $reviewBackup = Join-Path $reviewPrefsBackup ($reviewRelative.Replace('\', '_'))
    $reviewExists = Test-Path -LiteralPath $reviewOriginal
    if ($reviewExists) { Copy-Item -LiteralPath $reviewOriginal -Destination $reviewBackup }
    $reviewPreferenceManifest += [pscustomobject]@{ Original = $reviewOriginal; Backup = $reviewBackup; Existed = $reviewExists }
}
$reviewPreferenceManifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $reviewPrefsBackup 'Manifest.json') -Encoding utf8
$reviewFilters = 'IC.UI.InputDevices+IC.UI.SaveSessionInteraction+IC.UI.ScaleBoxResizeAndClick'
$reviewExpectedCount = 4
if ($IncludeGameplayChecks) {
    $reviewFilters += '+IC.Presentation.MonsterSizes+IC.Combat.NormalAttack+IC.Input.WeaponSwitchKeepsLocomotion+IC.Input.MovementAndCooldown'
    $reviewExpectedCount = 13
}
if ($SaveOnly) {
    $reviewFilters = 'IC.UI.SaveSessionInteraction'
    $reviewExpectedCount = 1
}
if ($InputOnly) {
    $reviewFilters = 'IC.UI.InputDevices'
    $reviewExpectedCount = 2
}
$reviewArguments = @(
    ('"' + $reviewProject + '"'), '-RenderOffscreen', '-unattended', '-nosound', '-nop4', '-NoSplash',
    '-NoSaveConfig', '-NoAssetRegistryCache', '-windowed', '-ResX=1600', '-ResY=900',
    ('"-ExecCmds=Automation RunTests ' + $reviewFilters + '"'),
    '"-TestExit=Automation Test Queue Empty"',
    ('"-ReportExportPath=' + (Join-Path $reviewOutput 'Automation') + '"'),
    ('"-abslog=' + (Join-Path $reviewOutput 'Review.log') + '"')
)
$reviewProcess = $null
try {
    $reviewProcess = Start-Process -FilePath $reviewEditor -ArgumentList $reviewArguments -WindowStyle Hidden -PassThru
    Write-Output ('Input review PID: ' + $reviewProcess.Id)
    if (-not $reviewProcess.WaitForExit(480000)) {
        Stop-Process -Id $reviewProcess.Id
        $reviewProcess.WaitForExit()
        throw 'The owned input review editor exceeded its timeout.'
    }
    if ($reviewProcess.ExitCode -ne 0) { throw ('Input review editor exited with code ' + $reviewProcess.ExitCode) }
    $reviewReport = Get-Content -Raw -LiteralPath (Join-Path $reviewOutput 'Automation\index.json') | ConvertFrom-Json
    if ($reviewReport.failed -ne 0 -or $reviewReport.notRun -ne 0 -or @($reviewReport.tests).Count -ne $reviewExpectedCount) {
        throw ('Input/gameplay review did not pass all ' + $reviewExpectedCount + ' scenarios. See ' + (Join-Path $reviewOutput 'Review.log'))
    }
    $reviewReport.tests | Select-Object fullTestPath, state, warnings, errors | ConvertTo-Json
}
finally {
    if (-not $reviewProcess -or $reviewProcess.HasExited) {
        foreach ($reviewPreference in $reviewPreferenceManifest) {
            if ($reviewPreference.Existed) {
                Copy-Item -LiteralPath $reviewPreference.Backup -Destination $reviewPreference.Original
                if ((Get-FileHash -LiteralPath $reviewPreference.Original).Hash -ne (Get-FileHash -LiteralPath $reviewPreference.Backup).Hash) {
                    throw ('Could not restore preferences: ' + $reviewPreference.Original)
                }
            }
            elseif (Test-Path -LiteralPath $reviewPreference.Original) { Remove-Item -LiteralPath $reviewPreference.Original }
        }
        Write-Output 'Editor and game preferences restored.'
    }
}
