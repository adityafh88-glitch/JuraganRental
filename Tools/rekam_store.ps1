# Rekam bahan screenshot Play Store 1920x1080 (layar penuh, tombol sentuh HP, Bahasa Indonesia).
# Hasil disalin ke Build\Promo\Store\raw\<nama>_<foto>.png lalu diolah Tools\make_store.py.
$Proj = 'D:\Dev\JuraganRental'
$Raw = "$Proj\Build\Promo\Store\raw"
New-Item -ItemType Directory -Force $Raw | Out-Null
$runs = [ordered]@{
    'garasi'   = '-jrautotest -jrfamily'
    'ruko'     = '-jrautotest -jrera2 -jrcdjam'
    'gc'       = '-jrautotest -jrera3 -jrdream'
    'berkah'   = '-jrautotest -jrera2 -jrberkah'
}
foreach ($k in $runs.Keys) {
    Remove-Item "$Proj\Saved\AutoTest\*.png" -ErrorAction SilentlyContinue
    & "$PSScriptRoot\run_autotest.ps1" -GameArgs ($runs[$k] + ' -jrtouch -jrindo -ExecCmds="r.SetRes 1920x1080f"') -Timeout 260 2>$null | Select-String 'CRASH|Fatal' | ForEach-Object { $_.Line }
    Get-ChildItem "$Proj\Saved\AutoTest\*.png" | ForEach-Object { Copy-Item $_.FullName "$Raw\$($k)_$($_.Name)" -Force }
    "$k : $((Get-ChildItem "$Raw\$($k)_*.png").Count) foto"
}
