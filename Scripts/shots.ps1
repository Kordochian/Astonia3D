# Takes development screenshots of the built game at given spots (uses -AstoniaGoto / -AstoniaShot).
#   powershell -File Scripts\shots.ps1 name=X,Y,Z,Yaw [name=X,Y,Z,Yaw ...]
# Images land in Saved\Screenshots\WindowsEditor\<name>.png
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Views)

$Editor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$Project = Join-Path $PSScriptRoot "..\AstoniaUE5.uproject" | Resolve-Path
$Dir = Join-Path $PSScriptRoot "..\Saved\Screenshots\WindowsEditor"

foreach ($View in $Views) {
    $Name, $Spot = $View.Split("=", 2)
    Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue | Remove-Item
    $Args = "`"$Project`" -game -windowed -resx=1280 -resy=720 -nosound -log=Shot_$Name.log -AstoniaGoto=$Spot -AstoniaShot"
    $Proc = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru -WindowStyle Minimized
    for ($i = 0; $i -lt 90 -and -not (Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue); $i++) { Start-Sleep -Seconds 1 }
    Start-Sleep -Seconds 2
    Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue
    $Shot = Get-ChildItem $Dir -Filter "Highres*.png" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Shot) { Move-Item -Force $Shot.FullName (Join-Path $Dir "$Name.png"); "$Name.png" } else { "${Name}: no screenshot" }
}
