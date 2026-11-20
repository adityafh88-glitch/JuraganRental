# Membuat logo final Juragan Rental bergaya kartun (garis tepi tebal, huruf memantul) dengan GDI+ bawaan Windows.
# Hasil di ArtSource/Logo: logo judul transparan, ikon aplikasi 512, header Play Store 1024x500,
# ubin K/A/H dan tulisan "Gamedev" untuk splash KAH Gamedev, serta ikon Android di Build/Android/res.
# Pakai: powershell -ExecutionPolicy Bypass -File Tools/make_logo.ps1

Add-Type -AssemblyName System.Drawing
$Root = Split-Path $PSScriptRoot -Parent
$Out = Join-Path $Root 'ArtSource\Logo'
New-Item -ItemType Directory -Force $Out | Out-Null

$Fonts = New-Object System.Drawing.Text.PrivateFontCollection
$Fonts.AddFontFile((Join-Path $Root 'ArtSource\Fonts\LuckiestGuy-Regular.ttf'))
$Fam = $Fonts.Families[0]

function C([string]$hex, [int]$a = 255) { $v = [Convert]::ToInt32($hex, 16); [System.Drawing.Color]::FromArgb($a, ($v -shr 16) -band 255, ($v -shr 8) -band 255, $v -band 255) }
$Ink = C '3a1d0b'

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

function Poly([float[]]$pts) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $arr = @(); for ($i = 0; $i -lt $pts.Length; $i += 2) { $arr += New-Object System.Drawing.PointF $pts[$i], $pts[$i + 1] }
    $p.AddPolygon([System.Drawing.PointF[]]$arr); return $p
}

function Star($g, [float]$cx, [float]$cy, [float]$r, $fill, [float]$w) {
    $pts = @()
    for ($i = 0; $i -lt 10; $i++) {
        $a = [Math]::PI / 2 + $i * [Math]::PI / 5; $rr = if ($i % 2 -eq 0) { $r } else { $r * 0.45 }
        $pts += $cx + $rr * [Math]::Cos($a); $pts += $cy - $rr * [Math]::Sin($a)
    }
    Fill-Outline $g (Poly $pts) $fill $w
}

# Teks per huruf: tiap huruf naik-turun (memantul) dan berganti warna, garis tepi tebal.
function Bouncy($g, [string]$text, [float]$size, [float]$cx, [float]$baseY, $colors, [float[]]$bounce, [float]$outline, [float]$spacing = 0.075) {
    $fmt = [System.Drawing.StringFormat]::GenericTypographic
    $paths = @(); $widths = @()
    foreach ($ch in $text.ToCharArray()) {
        $p = New-Object System.Drawing.Drawing2D.GraphicsPath
        $p.AddString([string]$ch, $Fam, 0, $size, (New-Object System.Drawing.PointF 0, 0), $fmt)
        $paths += $p; $widths += $p.GetBounds().Width
    }
    $gap = $size * $spacing
    $total = ($widths | Measure-Object -Sum).Sum + $gap * ($paths.Count - 1)
    $x = $cx - $total / 2
    for ($i = 0; $i -lt $paths.Count; $i++) {
        $p = $paths[$i]; $bb = $p.GetBounds()
        $m = New-Object System.Drawing.Drawing2D.Matrix
        $m.Translate($x - $bb.X, $baseY - $size * 0.8 + $bounce[$i % $bounce.Length])
        $p.Transform($m)
        Fill-Outline $g $p $colors[$i % $colors.Length] $outline
        $x += $widths[$i] + $gap
    }
}

function Plain($g, [string]$text, [float]$size, [float]$cx, [float]$cy, $fill, [float]$outline) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $fmt = [System.Drawing.StringFormat]::GenericTypographic
    $p.AddString($text, $Fam, 0, $size, (New-Object System.Drawing.PointF 0, 0), $fmt)
    $bb = $p.GetBounds(); $m = New-Object System.Drawing.Drawing2D.Matrix
    $m.Translate($cx - $bb.X - $bb.Width / 2, $cy - $bb.Y - $bb.Height / 2); $p.Transform($m)
    Fill-Outline $g $p $fill $outline
}

# Stik PS kecil (dipakai di logo dan maskot). Skala S, pusat (cx, cy).
function Pad($g, [float]$cx, [float]$cy, [float]$S, [float]$w, [bool]$face) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $p.AddBezier($cx - 40*$S, $cy, $cx - 40*$S, $cy - 26*$S, $cx - 30*$S, $cy - 26*$S, $cx - 16*$S, $cy - 26*$S)
    $p.AddLine($cx - 16*$S, $cy - 26*$S, $cx + 16*$S, $cy - 26*$S)
    $p.AddBezier($cx + 16*$S, $cy - 26*$S, $cx + 30*$S, $cy - 26*$S, $cx + 40*$S, $cy - 26*$S, $cx + 40*$S, $cy)
    $p.AddBezier($cx + 40*$S, $cy, $cx + 40*$S, $cy + 26*$S, $cx + 24*$S, $cy + 30*$S, $cx + 16*$S, $cy + 12*$S)
    $p.AddLine($cx + 16*$S, $cy + 12*$S, $cx - 16*$S, $cy + 12*$S)
    $p.AddBezier($cx - 16*$S, $cy + 12*$S, $cx - 24*$S, $cy + 30*$S, $cx - 40*$S, $cy + 26*$S, $cx - 40*$S, $cy)
    $p.CloseFigure()
    Fill-Outline $g $p (C 'f4f1e8') $w
    if ($face) {
        $br = New-Object System.Drawing.SolidBrush $Ink
        $g.FillEllipse($br, $cx - 15*$S, $cy - 12*$S, 9*$S, 9*$S); $g.FillEllipse($br, $cx + 6*$S, $cy - 12*$S, 9*$S, 9*$S)
        $pink = New-Object System.Drawing.SolidBrush (C 'ff9aa2')
        $g.FillEllipse($pink, $cx - 27*$S, $cy - 2*$S, 10*$S, 7*$S); $g.FillEllipse($pink, $cx + 17*$S, $cy - 2*$S, 10*$S, 7*$S)
        $pen = New-Object System.Drawing.Pen $Ink, (4*$S); $pen.StartCap = 'Round'; $pen.EndCap = 'Round'
        $g.DrawArc($pen, $cx - 9*$S, $cy - 6*$S, 18*$S, 12*$S, 20, 140)
    } else {
        $g.FillEllipse((New-Object System.Drawing.SolidBrush (C '2f7bff')), $cx - 30*$S, $cy - 8*$S, 12*$S, 12*$S)
        $g.FillEllipse((New-Object System.Drawing.SolidBrush (C 'ff4d3d')), $cx + 18*$S, $cy - 12*$S, 9*$S, 9*$S)
        $g.FillEllipse((New-Object System.Drawing.SolidBrush (C '3ccf6e')), $cx + 26*$S, $cy - 3*$S, 9*$S, 9*$S)
    }
}

function Crown($g, [float]$cx, [float]$cy, [float]$S, [float]$w) {
    $pts = @(($cx - 27*$S), ($cy + 14*$S), ($cx - 21*$S), ($cy - 10*$S), ($cx - 9*$S), ($cy + 4*$S), $cx, ($cy - 16*$S), ($cx + 9*$S), ($cy + 4*$S), ($cx + 21*$S), ($cy - 10*$S), ($cx + 27*$S), ($cy + 14*$S))
    Fill-Outline $g (Poly $pts) (C 'ffc93c') $w
    $g.FillEllipse((New-Object System.Drawing.SolidBrush (C 'ff4d3d')), $cx - 4*$S, $cy - 2*$S, 8*$S, 8*$S)
}

# ---------- Logo judul (transparan) ----------
function Draw-Title($g, [float]$ox, [float]$oy, [float]$k) {
    function P([float]$x, [float]$y) { @(($ox + $x * $k), ($oy + $y * $k)) }
    # Tiang
    foreach ($px in 150, 1270) { Fill-Outline $g (RoundRect ($ox + $px*$k) ($oy + 560*$k) (44*$k) (240*$k) (10*$k)) (C '7a4a24') (12*$k) }
    # Papan miring sedikit
    $outer = Poly ((P 70 120) + (P 1530 92) + (P 1548 650) + (P 82 676))
    Fill-Outline $g $outer (C 'ffc93c') (30*$k)
    $shine = Poly ((P 110 150) + (P 1500 124) + (P 1503 180) + (P 113 206))
    Fill-Outline $g $shine (C 'ffe08a') 0
    foreach ($b in @(@(120,160), @(1488,136), @(128,624), @(1500,600))) {
        $e = New-Object System.Drawing.Drawing2D.GraphicsPath; $e.AddEllipse($ox + ($b[0]-14)*$k, $oy + ($b[1]-14)*$k, 28*$k, 28*$k)
        Fill-Outline $g $e (C 'fff6c9') (7*$k)
    }
    Bouncy $g 'JURAGAN' (215*$k) ($ox + 806*$k) ($oy + 360*$k) @((C 'ff4d3d'), (C 'ff7a2f')) @(0, (-24*$k), (30*$k), (-22*$k), (22*$k), (-18*$k), (18*$k)) (34*$k)
    Bouncy $g 'RENTAL' (180*$k) ($ox + 800*$k) ($oy + 578*$k) @((C '2f7bff'), (C '38b6ff')) @(0, (-20*$k), (22*$k), (-22*$k), (18*$k), (-15*$k)) (30*$k)
    Star $g ($ox + 250*$k) ($oy + 70*$k) (62*$k) (C 'ffffff') (11*$k)
    Star $g ($ox + 1430*$k) ($oy + 690*$k) (52*$k) (C 'ffffff') (11*$k)
    Pad $g ($ox + 420*$k) ($oy + 730*$k) (2.4*$k) (12*$k) $false
    Plain $g 'RENTAL PS & WARNET' (70*$k) ($ox + 930*$k) ($oy + 742*$k) (C 'ffffff') (18*$k)
}

$cv = New-Canvas 1600 900; $b = $cv[0]; $g = $cv[1]
$g.Clear([System.Drawing.Color]::Transparent)
Draw-Title $g 0 30 1.0
$b.Save((Join-Path $Out 'Logo_Judul.png'), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $b.Dispose()

# ---------- Ikon aplikasi (maskot stik bermahkota) ----------
function Draw-Icon([int]$size, [bool]$transparentCorners) {
    $cv = New-Canvas $size $size; $b = $cv[0]; $g = $cv[1]
    $g.Clear([System.Drawing.Color]::Transparent)
    $k = $size / 512.0
    $bg = RoundRect (10*$k) (10*$k) (492*$k) (492*$k) (110*$k)
    Fill-Outline $g $bg (C '2f7bff') (20*$k)
    $hl = New-Object System.Drawing.Drawing2D.GraphicsPath
    $hl.AddArc(40*$k, 40*$k, 120*$k, 120*$k, 180, 90); $hl.AddLine(100*$k, 40*$k, 412*$k, 40*$k)
    $hl.AddArc(352*$k, 40*$k, 120*$k, 120*$k, 270, 90); $hl.AddLine(472*$k, 100*$k, 472*$k, 130*$k)
    $hl.AddBezier(472*$k, 130*$k, 330*$k, 100*$k, 180*$k, 120*$k, 40*$k, 175*$k); $hl.CloseFigure()
    Fill-Outline $g $hl (C '5d9bff') 0
    Pad $g (256*$k) (318*$k) (4.6*$k) (16*$k) $true
    Crown $g (256*$k) (170*$k) (4.0*$k) (14*$k)
    return @($b, $g)
}
$ic = Draw-Icon 512 $true
$ic[0].Save((Join-Path $Out 'Ikon_512.png'), [System.Drawing.Imaging.ImageFormat]::Png); $ic[1].Dispose()

# Ikon Android (dikecilkan dari 512 dengan kualitas tinggi), ditimpa per kepadatan layar.
$AndroidRes = Join-Path $Root 'Build\Android\res'
foreach ($d in @(@('drawable', 192), @('drawable-ldpi', 36), @('drawable-mdpi', 48), @('drawable-hdpi', 72), @('drawable-xhdpi', 96), @('drawable-xxhdpi', 144), @('drawable-xxxhdpi', 192))) {
    $dir = Join-Path $AndroidRes $d[0]; New-Item -ItemType Directory -Force $dir | Out-Null
    $s = [int]$d[1]; $cv = New-Canvas $s $s; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawImage($ic[0], 0, 0, $s, $s)
    $cv[0].Save((Join-Path $dir 'icon.png'), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $cv[0].Dispose()
}

# ---------- Header Play Store 1024x500 ----------
$cv = New-Canvas 1024 500; $b = $cv[0]; $g = $cv[1]
$g.Clear((C '8fd3ff'))
$g.FillEllipse((New-Object System.Drawing.SolidBrush (C 'fff6c9')), 40, 30, 110, 110)
$cloud = New-Object System.Drawing.SolidBrush (C 'ffffff')
foreach ($c in @(@(760, 40, 180, 60), @(820, 20, 120, 60), @(600, 400, 200, 70), @(60, 400, 160, 60))) { $g.FillEllipse($cloud, $c[0], $c[1], $c[2], $c[3]) }
$g.FillRectangle((New-Object System.Drawing.SolidBrush (C '7cc96b')), 0, 455, 1024, 45)
Draw-Title $g 20 40 0.47
$g.DrawImage($ic[0], 790, 175, 210, 210)
$b.Save((Join-Path $Out 'Header_1024x500.png'), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $b.Dispose()

# ---------- Splash KAH Gamedev: ubin huruf terpisah (dianimasikan pop-up di game) ----------
foreach ($t in @(@('K', 'ffc93c'), @('A', 'ff4d3d'), @('H', '2f7bff'))) {
    $cv = New-Canvas 256 256; $b = $cv[0]; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
    Fill-Outline $g (RoundRect 14 14 228 228 44) (C $t[1]) 16 (C 'f4f1e8')
    Plain $g $t[0] 170 128 136 (C 'ffffff') 22
    $b.Save((Join-Path $Out ("KAH_" + $t[0] + ".png")), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $b.Dispose()
}
$cv = New-Canvas 640 180; $b = $cv[0]; $g = $cv[1]; $g.Clear([System.Drawing.Color]::Transparent)
Plain $g 'GAMEDEV' 120 320 92 (C 'f4f1e8') 18
$b.Save((Join-Path $Out 'KAH_Gamedev.png'), [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $b.Dispose()

$ic[0].Dispose()
"Logo selesai: $Out"
Get-ChildItem $Out | Select-Object Name, Length | Format-Table -AutoSize | Out-String
