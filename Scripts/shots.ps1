# Takes development screenshots of the built game at given spots (uses -AstoniaGoto / -AstoniaShot).
#   powershell -File Scripts\shots.ps1 name=X,Y,Z,Yaw [name=X,Y,Z,Yaw ...]
# Images land in Saved\Screenshots\WindowsEditor\<name>.png
#   name=X,Y,Z,Yaw,Pitch also works; $env:ASTONIA_ARM = camera distance in cm (0 = eye view)
#   $env:ASTONIA_SPAWN = "<class path>,X,Y,Z,Yaw" spawns an extra actor (e.g. a MetaHuman Blueprint)
#   $env:ASTONIA_FLY = 1 hovers at the spot (aerial views)
#   $env:ASTONIA_SHOT_DELAY = seconds to wait before the screenshot (default 3; longer while shaders compile)
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Views)
$Delay = if ($env:ASTONIA_SHOT_DELAY) { $env:ASTONIA_SHOT_DELAY } else { 3 }

$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$Project = Join-Path $PSScriptRoot "..\AstoniaUE5.uproject" | Resolve-Path
$Dir = Join-Path $PSScriptRoot "..\Saved\Screenshots\WindowsEditor"

foreach ($View in $Views) {
    $Name, $Spot = $View.Split("=", 2)
    Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue | Remove-Item
    $Args = "`"$Project`" -game -windowed -resx=1280 -resy=720 -nosound -log=Shot_$Name.log -AstoniaGoto=$Spot -AstoniaShot -AstoniaShotDelay=$Delay"
    if ($env:ASTONIA_ARM) { $Args += " -AstoniaArm=$($env:ASTONIA_ARM)" }
    if ($env:ASTONIA_FLY) { $Args += " -AstoniaFly" }
    if ($env:ASTONIA_SPAWN) { $Args += " -AstoniaSpawn=$($env:ASTONIA_SPAWN)" }
    $Proc = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru -WindowStyle Minimized
    for ($i = 0; $i -lt (90 + [int]$Delay) -and -not (Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue); $i++) { Start-Sleep -Seconds 1 }
    Start-Sleep -Seconds 2
    Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue
    $Shot = Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Shot) { Move-Item -Force $Shot.FullName (Join-Path $Dir "$Name.png"); "$Name.png" } else { "${Name}: no screenshot" }
}
