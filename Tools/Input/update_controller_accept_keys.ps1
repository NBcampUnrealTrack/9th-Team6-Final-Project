$ErrorActionPreference='Stop'
$taskRoot=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskFiles=rg -l 'Gamepad_FaceButton_(Bottom|Right)' (Join-Path $taskRoot 'Source/IsekaiClass/Private/UI') -g '*.cpp'
foreach($taskFile in $taskFiles){
    $taskText=[IO.File]::ReadAllText($taskFile)
    $taskBefore=$taskText
    $taskExpression='(\b(?:[A-Za-z_]\w*\.GetKey\(\)|[A-Za-z_]\w*))\s*==\s*EKeys::Gamepad_FaceButton_Bottom'
    $taskText=[regex]::Replace($taskText,$taskExpression,'UICGameUserSettings::IsGamepadAcceptKey($1)')
    $taskExpression='(\b(?:[A-Za-z_]\w*\.GetKey\(\)|[A-Za-z_]\w*))\s*==\s*EKeys::Gamepad_FaceButton_Right'
    $taskText=[regex]::Replace($taskText,$taskExpression,'UICGameUserSettings::IsGamepadBackKey($1)')
    if($taskText -ne $taskBefore){
        if(-not $taskText.Contains('#include "Settings/ICGameUserSettings.h"')){$taskText='#include "Settings/ICGameUserSettings.h"'+"`r`n"+$taskText}
        [IO.File]::WriteAllText($taskFile,$taskText,[Text.UTF8Encoding]::new($false))
    }
}
