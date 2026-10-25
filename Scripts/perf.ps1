# Measures frame and GPU times at a few fixed views of Cameron (development build, 1280x720 window).
#   powershell -File Scripts\perf.ps1 [label]
# Prints one line per view and appends them to Saved\Logs\perf_history.txt.
param([string]$Label = "run")
$Views = @(
    "perf_aerial=20000,34000,3500,300,-25",
    "perf_street=24800,26500,150,270,-6",
    "perf_court=25300,37700,150,270,-8"
)
$Logs = Join-Path $PSScriptRoot "..\Saved\Logs"
$env:ASTONIA_PERF = 1
$env:ASTONIA_SHOT_DELAY = 12
foreach ($View in $Views) {
    $Name = $View.Split("=")[0]
    $env:ASTONIA_FLY = if ($Name -eq "perf_aerial") { 1 } else { "" }
    $env:ASTONIA_ARM = if ($Name -eq "perf_aerial") { 0 } else { "" }
    & (Join-Path $PSScriptRoot "shots.ps1") $View | Out-Null
    $Line = Select-String -Path (Join-Path $Logs "Shot_$Name.log") -Pattern "AstoniaPerf:" | Select-Object -Last 1
    $Out = "$Label $Name " + ($Line.Line -replace '^.*AstoniaPerf: ', '')
    $Out
    Add-Content (Join-Path $Logs "perf_history.txt") $Out
}
$env:ASTONIA_PERF = ""; $env:ASTONIA_FLY = ""; $env:ASTONIA_ARM = ""
