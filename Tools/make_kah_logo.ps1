# Set logo lengkap KAH Gamedev (ubin K-A-H + tulisan GAMEDEV) resolusi tinggi, gaya sama dengan splash game.
# Hasil: ArtSource/Logo/KAH_Lengkap/
#   KAH_Gamedev_Utama.png        2400x1500  transparan, ubin di atas + GAMEDEV di bawah
#   KAH_Gamedev_Horizontal.png   3150x1000  transparan, ubin di kiri + GAMEDEV di kanan
#   KAH_Gamedev_Profil.png       2048x2048  latar gelap (foto profil Threads/IG/FB, aman dipotong bulat)
#   KAH_Gamedev_Profil_Terang.png 2048x2048 latar krem
#   KAH_Gamedev_Sampul_FB.png    1640x624   sampul halaman Facebook
#   KAH_Ikon.png                 1024x1024  transparan, ubin KAH saja (favicon, watermark)
# Pakai: powershell -ExecutionPolicy Bypass -File Tools/make_kah_logo.ps1

Add-Type -AssemblyName System.Drawing
$Root = Split-Path $PSScriptRoot -Parent
$Out = Join-Path $Root 'ArtSource\Logo\KAH_Lengkap'
New-Item -ItemType Directory -Force $Out | Out-Null

$Fonts = New-Object System.Drawing.Text.PrivateFontCollection
$Fonts.AddFontFile((Join-Path $Root 'ArtSource\Fonts\LuckiestGuy-Regular.ttf'))
$Fam = $Fonts.Families[0]

function C([string]$hex, [int]$a = 255) { $v = [Convert]::ToInt32($hex, 16); [System.Drawing.Color]::FromArgb($a, ($v -shr 16) -band 255, ($v -shr 8) -band 255, $v -band 255) }
$Ink = C '3a1d0b'; $Cream = C 'f4f1e8'; $Dark = C '1b1e24'

function New-Canvas([int]$w, [int]$h) {
    $b = New-Object System.Drawing.Bitmap $w, $h, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($b)
    $g.SmoothingMode = 'AntiAlias'; $g.PixelOffsetMode = 'HighQuality'; $g.InterpolationMode = 'HighQualityBicubic'
    $g.CompositingQuality = 'HighQuality'; $g.TextRenderingHint = 'AntiAliasGridFit'
    return @($b, $g)
}
function Fill-Outline($g, $path, $fill, [float]$width, $ink = $Ink) {
    if ($width -gt 0) {
        $pen = New-Object System.Drawing.Pen $ink, $width
        $pen.LineJoin = 'Round'; $pen.StartCap = 'Round'; $pen.EndCap = 'Round'
        $g.DrawPath($pen, $path); $pen.Dispose()
    }
    $br = New-Object System.Drawing.SolidBrush $fill
    $g.FillPath($br, $path); $br.Dispose()
}
function RoundRect([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $p.AddArc($x, $y, $d, $d, 180, 90); $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90); $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure(); return $p
}
function TextPath([string]$text, [float]$size, [float]$cx, [float]$cy) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $p.AddString($text, $Fam, 0, $size, (New-Object System.Drawing.PointF 0, 0), [System.Drawing.StringFormat]::GenericTypographic)
    $bb = $p.GetBounds(); $m = New-Object System.Drawing.Drawing2D.Matrix
    $m.Translate($cx - $bb.X - $bb.Width / 2, $cy - $bb.Y - $bb.Height / 2); $p.Transform($m)
    return $p
}

# Satu ubin huruf (seperti splash): kotak warna bertepi krem, huruf putih bergaris tepi cokelat. Ukuran sisi $s, pusat (cx, cy), miring $rot derajat.
function Tile($g, [string]$ch, [string]$col, [float]$cx, [float]$cy, [float]$s, [float]$rot) {
    $st = $g.Save()
    $g.TranslateTransform($cx, $cy); $g.RotateTransform($rot)
    # bayangan lembut
    $sh = RoundRect (-$s / 2 + $s * 0.035) (-$s / 2 + $s * 0.06) $s $s ($s * 0.19)
    $g.FillPath((New-Object System.Drawing.SolidBrush (C '000000' 70)), $sh)
    Fill-Outline $g (RoundRect (-$s / 2) (-$s / 2) $s $s ($s * 0.19)) (C $col) ($s * 0.07) $Cream
    Fill-Outline $g (TextPath $ch ($s * 0.74) 0 ($s * 0.03)) (C 'ffffff') ($s * 0.095)
    $g.Restore($st)
}
function Tiles($g, [float]$cx, [float]$cy, [float]$s) {
    $gap = $s * 1.12
    Tile $g 'K' 'ffc93c' ($cx - $gap) ($cy + $s * 0.04) $s -7
    Tile $g 'A' 'ff4d3d' $cx ($cy - $s * 0.06) $s 3
    Tile $g 'H' '2f7bff' ($cx + $gap) ($cy + $s * 0.03) $s 8
}
function Gamedev($g, [float]$cx, [float]$cy, [float]$size) {
    Fill-Outline $g (TextPath 'GAMEDEV' $size $cx $cy) $Cream ($size * 0.15)
}
function Save($b, $g, [string]$name) { $b.Save((Join-Path $Out $name), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $b.Dispose() }
function Lockup($g, [float]$cx, [float]$cy, [float]$s) {
    # ubin di atas, GAMEDEV di bawah; tinggi total kira-kira 2,1 x s
    Tiles $g $cx ($cy - $s * 0.45) $s
    Gamedev $g $cx ($cy + $s * 0.78) ($s * 0.62)
}

# Utama (transparan)
$cv = New-Canvas 2400 1500; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
Lockup $g 1200 740 560
Save $cv[0] $g 'KAH_Gamedev_Utama.png'

# Horizontal (transparan)
$cv = New-Canvas 3150 1000; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
Tiles $g 900 500 430
Gamedev $g 2380 510 300
Save $cv[0] $g 'KAH_Gamedev_Horizontal.png'

# Profil gelap dan terang (isi di tengah 70% agar aman dipotong lingkaran)
foreach ($v in @(@('KAH_Gamedev_Profil.png', $Dark), @('KAH_Gamedev_Profil_Terang.png', (C 'fff6dd')))) {
    $cv = New-Canvas 2048 2048; $g = $cv[1]; $g.Clear($v[1])
    if ($v[1] -eq $Dark) {
        $glow = New-Object System.Drawing.Drawing2D.GraphicsPath; $glow.AddEllipse(224, 224, 1600, 1600)
        $pb = New-Object System.Drawing.Drawing2D.PathGradientBrush $glow
        $pb.CenterColor = (C '2c3340'); $pb.SurroundColors = @($Dark); $g.FillPath($pb, $glow)
    }
    Lockup $g 1024 1010 430
    Save $cv[0] $g $v[0]
}

# Sampul Facebook 1640x624 (isi penting di tengah, karena foto profil menutupi kiri bawah)
$cv = New-Canvas 1640 624; $g = $cv[1]; $g.Clear($Dark)
$stripe = New-Object System.Drawing.SolidBrush (C '232833')
for ($x = -600; $x -lt 1640; $x += 120) { $g.FillPolygon($stripe, [System.Drawing.PointF[]]@((New-Object System.Drawing.PointF $x, 624), (New-Object System.Drawing.PointF ($x + 60), 624), (New-Object System.Drawing.PointF ($x + 684), 0), (New-Object System.Drawing.PointF ($x + 624), 0))) }
Tiles $g 515 280 150
Gamedev $g 1125 285 150
$tag = TextPath 'GAME INDIE DARI INDONESIA' 40 820 470
Fill-Outline $g $tag (C 'ffc93c') 0
Save $cv[0] $g 'KAH_Gamedev_Sampul_FB.png'

# Ikon (ubin saja, transparan)
$cv = New-Canvas 1024 1024; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
Tiles $g 512 512 270
Save $cv[0] $g 'KAH_Ikon.png'

"Logo KAH Gamedev selesai: $Out"
Get-ChildItem $Out | Select-Object Name, @{n='KB'; e={[int]($_.Length / 1KB)}} | Format-Table -AutoSize | Out-String
