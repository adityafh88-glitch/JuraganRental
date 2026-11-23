# Trailer Play Store (~35 detik, 1920x1080) dari video promosi Build\Promo\postN_landscape.mp4.
#   -Rekam : rekam ulang semua adegan dulu (Tools\rekam_promo.ps1 + make_promo.ps1), sekitar 30-60 menit.
# Hasil: Build\Promo\Store\trailer_1080p.mp4 (unggah ke YouTube, lalu tempel link-nya di Play Console).
param([switch]$Rekam)
$ErrorActionPreference = 'Stop'
$Proj = 'D:\Dev\JuraganRental'
$Promo = "$Proj\Build\Promo"
$Store = "$Promo\Store"
New-Item -ItemType Directory -Force $Store | Out-Null

if ($Rekam) {
    foreach ($id in 1, 2, 3, 4, 5, 7, 8) {
        "== rekam adegan $id"
        & "$PSScriptRoot\rekam_promo.ps1" -Id $id
        & "$PSScriptRoot\make_promo.ps1" -Id $id | Out-Null
    }
}

# Potongan terbaik tiap video: @(nomor, mulai, durasi)
$cuts = @(@(1, 0.0, 5.5), @(4, 0.0, 6.0), @(3, 0.0, 8.0), @(2, 3.5, 5.0), @(7, 3.0, 6.0), @(8, 2.5, 6.0))
$args = @()
$filter = ''
$i = 0
foreach ($c in $cuts) {
    $args += @('-ss', $c[1], '-t', $c[2], '-i', "$Promo\post$($c[0])_landscape.mp4")
    $filter += "[$($i):v]scale=1920:1080,setsar=1,fps=30,format=yuv420p[v$i];[$($i):a]aresample=48000,aformat=channel_layouts=stereo[a$i];"
    $i++
}
# Kartu penutup 3,5 detik: feature graphic + ajakan
$args += @('-loop', '1', '-t', '3.5', '-i', "$Store\feature_1024x500.png", '-f', 'lavfi', '-t', '3.5', '-i', 'anullsrc=r=48000:cl=stereo')
$filter += "[$($i):v]scale=1920:-2,pad=1920:1080:(ow-iw)/2:(oh-ih)/2:color=0x3a2414,setsar=1,fps=30,format=yuv420p,drawtext=fontfile='C\:/Windows/Fonts/seguibl.ttf':text='Main sekarang di Google Play':fontcolor=0xFFC73A:fontsize=64:x=(w-tw)/2:y=h-170[v$i];[$($i + 1):a]anull[a$i];"
$n = $i + 1
for ($k = 0; $k -lt $n; $k++) { $filter += "[v$k][a$k]" }
$filter += "concat=n=$($n):v=1:a=1[v][a]"
$out = "$Store\trailer_1080p.mp4"
& ffmpeg -v error -y @args -filter_complex $filter -map '[v]' -map '[a]' -c:v libx264 -crf 20 -preset medium -c:a aac -b:a 160k -movflags +faststart $out
$d = ffprobe -v error -show_entries format=duration -of csv=p=0 $out
"trailer: $out ($([math]::Round([double]$d, 1)) detik)"
