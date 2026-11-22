param(
    # 1 = opening garasi; 2..5, 7 (turnamen), 8 (Pak RT) = skenario video di Source/JuraganRental/JRPromo.cpp (6 = foto, lihat -jrpromo=6)
    [int]$Id = 2,
    [int]$Timeout = 900
)
# Rekam frame video promosi (1280x720, TSR, tanpa suara, langkah waktu tetap 30 fps) ke Saved\Capture.
# Lumen/TSR tetap utuh karena yang ditangkap viewport yang sedang berjalan (bukan HighResShot).
$Proj = 'D:\Dev\JuraganRental'
New-Item -ItemType Directory -Force "$Proj\Saved\Capture" | Out-Null
$prefix = if ($Id -eq 1) { 'op' } else { "promo$Id" }
Remove-Item "$Proj\Saved\Capture\$($prefix)_*" -ErrorAction SilentlyContinue
$scene = if ($Id -eq 1) { '-jropening' } else { "-jrpromo=$Id" }
$args = "$scene -jrcapture -jrindo -benchmark -fps=30 -nosound -ExecCmds=`"r.SetRes 1280x720w, r.AntiAliasingMethod 4`""
& "$PSScriptRoot\run_autotest.ps1" -GameArgs $args -Timeout $Timeout | Select-String 'JR_PROMO|JR_CAPTURE|CRASH|Fatal|Durasi|WAKTU'
$n = (Get-ChildItem "$Proj\Saved\Capture\$($prefix)_*.png" -ErrorAction SilentlyContinue).Count
"frame: $n ($([math]::Round($n / 30, 1)) dtk)"
