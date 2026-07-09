# ARCHIVED — kept for reference only. Not executed by CodeForge due to "no arbitrary shell" rule.
# Real fix path: run `unreal/build` tool after editor codepage normalization, or trigger Live Coding in-editor.
#requires -Version 5.0
# Force UTF-8 codepage before invoking UE Build.bat so cmd.exe doesn't choke on
# Chinese locale / OEM codepage issues that turn into "not recognized" errors.
$ErrorActionPreference = 'Stop'
chcp 65001 | Out-Null
$env:PYTHONIOENCODING = 'utf-8'

$ueRoot   = 'C:\Program Files\Epic Games\UE_5.8'
$uproject = 'D:\ue\CropoutSampleProject\CropoutSampleProject.uproject'
$buildBat = Join-Path $ueRoot 'Engine\Build\BatchFiles\Build.bat'
$target   = 'CropoutSampleProjectEditor'
$platform = 'Win64'
$config   = 'Development'

Write-Host "[build] codepage: $((Get-Culture).Name)" -ForegroundColor Cyan
Write-Host "[build] engineRoot: $ueRoot" -ForegroundColor Cyan
Write-Host "[build] uproject  : $uproject" -ForegroundColor Cyan
Write-Host "[build] buildBat  : $buildBat (exists=$([System.IO.File]::Exists($buildBat)))" -ForegroundColor Cyan

if (-not (Test-Path $buildBat)) {
    throw "Build.bat not found at $buildBat"
}

$argList = @(
    $target,
    $platform,
    $config,
    "-Project=`"$uproject`"",
    '-WaitMutex',
    '-NoHotReload',
    '-architecture=x64'
)

Write-Host "[build] invoking: cmd.exe /c `"$buildBat`" $($argList -join ' ')" -ForegroundColor Yellow
& cmd.exe /c "`"$buildBat`" $($argList -join ' ')"
exit $LASTEXITCODE