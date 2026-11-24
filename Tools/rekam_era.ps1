# Foto tiap era dengan SEMUA barang terbeli (-jrfull) untuk postingan penjelasan era.
# Hasil: Build\Promo\Era\raw\era<N>_<foto>.png (1920x1080, tanpa HUD, Bahasa Indonesia), lalu Tools\make_era_posts.py.
$Proj = 'D:\Dev\JuraganRental'
$Raw = "$Proj\Build\Promo\Era\raw"
New-Item -ItemType Directory -Force $Raw | Out-Null
$eras = [ordered]@{ '1' = ''; '2' = '-jrera2'; '3' = '-jrera3' }
foreach ($e in $eras.Keys) {
    foreach ($mode in '', '-jrfasad') {
        Remove-Item "$Proj\Saved\AutoTest\*.png" -ErrorAction SilentlyContinue
        $args = "-jrautotest -jrfull $($eras[$e]) $mode -jrindo -ExecCmds=`"r.SetRes 1920x1080f`""
        & "$PSScriptRoot\run_autotest.ps1" -GameArgs $args -Timeout ($(if ($mode) { 150 } else { 200 })) 2>$null | Select-String 'CRASH|Fatal' | ForEach-Object { $_.Line }
        Get-ChildItem "$Proj\Saved\AutoTest\JR_Shot_*.png", "$Proj\Saved\AutoTest\JR_Fasad_*.png" -ErrorAction SilentlyContinue |
            ForEach-Object { Copy-Item $_.FullName "$Raw\era$($e)_$($_.Name -replace '^JR_','')" -Force }
    }
    "era $e : $((Get-ChildItem "$Raw\era$($e)_*.png").Count) foto"
}
