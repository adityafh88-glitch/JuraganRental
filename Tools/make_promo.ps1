# Susun video promosi dari frame Saved\Capture (hasil Tools\rekam_promo.ps1) + teks + suara.
# Hasil: Build\Promo\postN_vertikal.mp4 (1080x1920) dan postN_landscape.mp4 (1920x1080).
#   & Tools\make_promo.ps1 -Id 2
param([int]$Id = 2, [switch]$ShowFilter)
$ErrorActionPreference = 'Stop'
function Invoke-FF([string[]]$A, [string]$Name) {
  # Lewat berkas filter + Start-Process: aman dari aturan kutip PowerShell 5.1 dan tidak menunggu keyboard
  $log = "D:\Tmp\ff_$Name.log"
  $p = Start-Process -FilePath $ff -ArgumentList $A -NoNewWindow -PassThru -Wait -RedirectStandardError $log
  if ($p.ExitCode -ne 0) { Get-Content $log -Tail 15; throw "FFmpeg gagal ($Name)" }
}
$Proj = 'D:\Dev\JuraganRental'
$ff = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -Expand FullName
$out = "$Proj\Build\Promo"; New-Item -ItemType Directory -Force $out | Out-Null
$prefix = if ($Id -eq 1) { 'op' } else { "promo$Id" }
$frames = (Get-ChildItem "$Proj\Saved\Capture\$($prefix)_*.png").Count
if ($frames -eq 0) { throw "Belum ada frame $prefix. Jalankan Tools\rekam_promo.ps1 -Id $Id dulu." }
$font = "C\:/Windows/Fonts/impact.ttf"; $fontS = "C\:/Windows/Fonts/arialbd.ttf"
function Esc([string]$t) { $t -replace '\\', '\\\\' -replace "'", [string][char]0x2019 -replace ':', '\:' -replace '%', '\%' }
function Fade([double]$a) { "alpha='min(1,max(0,(t-$a)/0.3))'" }

# ---- Pengaturan tiap video ----
$speed = 1.0; $tail = 0.0; $music = $null; $zoom = 1.0
$black = @()        # @(mulai, selesai) layar hitam
$texts = @()        # @{T=teks; A=mulai; B=selesai; Size; Color; Y='tengah'|'atas'}
$endCard = $null    # @{Dur=detik; Text='...'}
$extraCues = @()    # @(detik, nama_suara) tambahan di luar isyarat game
switch ($Id) {
  1 { $speed = 1.5; $tail = 0.5
      $texts = @(@{T='Dulu dijewer emak di rental...'; A=0.3; B=4.7; Size=76; Color='white'; Y='atas'},
                 @{T='sekarang kamu juragannya'; A=5.1; B=99; Size=92; Color='0xFFD43B'; Y='atas'})
      $extraCues = @(@(0.5, 'SFX_Motor_Lewat'), @(7.4, 'SFX_Konsol_Boot')) }
  2 { $black = @(4.5, 7.0)
      $texts = @(@{T='JEGLEK.'; A=5.5; B=7.0; Size=170; Color='0xFFD43B'; Y='tengah'}) }
  3 { $texts = @(@{T='"Bang, nambah sejam!"'; A=0.2; B=3.9; Size=64; Color='white'; Y='atas'},
                 @{T='Emak datang...'; A=4.2; B=7.9; Size=72; Color='white'; Y='atas'},
                 @{T='Kalah penalti, stik melayang'; A=8.2; B=99; Size=62; Color='white'; Y='atas'}) }
  4 { $music = 'Music\MUS_Radio_2.wav'
      $texts = @(@{T='ERA 1 - GARASI'; A=0.0; B=1.4; Size=70; Color='white'; Y='atas'},
                 @{T='ERA 2 - RUKO'; A=1.4; B=2.8; Size=70; Color='white'; Y='atas'},
                 @{T='ERA 3 - GAME CENTER'; A=2.8; B=4.4; Size=70; Color='0xFFD43B'; Y='atas'},
                 @{T='naik ke VIP Lounge...'; A=6.4; B=10.0; Size=60; Color='white'; Y='atas'}) }
  5 { $zoom = 1.8; $endCard = @{Dur = 3.2; Text = 'Cari juragan pertama.'; Text2 = 'Komen JURAGAN.'}
      $extraCues = @(,@(0.15, 'SFX_Ding')) }
}

# ---- Durasi ----
$clip = [math]::Round($frames / 30 / $speed + $tail, 3)
$dur = $clip + $(if ($endCard) { $endCard.Dur } else { 0 })

# ---- Daftar suara: isyarat dari game (frame) + tambahan ----
$cues = @()
$cueFile = "$Proj\Saved\Capture\$($prefix)_cues.txt"
if (Test-Path $cueFile) { foreach ($l in Get-Content $cueFile) { if ($l -match '^(\d+)\s+(\S+)') { $cues += , @(([int]$Matches[1] / 30 / $speed), $Matches[2]) } } }
$cues += $extraCues
if ($endCard) { $cues += , @(($clip + 0.1), 'SFX_Konsol_Boot') }
function Find-Wav([string]$n) {
  $c = Get-ChildItem "$Proj\ArtSource\Audio" -Recurse -Filter "$n.wav" | Select-Object -First 1
  if (-not $c) { throw "Suara tidak ditemukan: $n" }; $c.FullName
}

# ---- Video dasar ----
$v = "[0:v]setpts=PTS/$speed,fps=30"
if ($tail -gt 0) { $v += ",tpad=stop_mode=clone:stop_duration=$tail" }
if ($zoom -gt 1) { $v += ",crop=iw/$($zoom):ih/$($zoom):(iw-iw/$($zoom))/2:(ih-ih/$($zoom))/2" }
$v += ",scale=1920:1080:flags=lanczos,format=yuv420p"
if ($black.Count -eq 2) { $v += ",drawbox=x=0:y=0:w=iw:h=ih:color=black:t=fill:enable='between(t,$($black[0]),$($black[1]))'" }
if ($endCard) {
  $v += ",tpad=stop_mode=add:stop_duration=$($endCard.Dur):color=0x15171c"
}
$v += "[base]"
$inputs = @('-framerate', '30', '-i', "$Proj\Saved\Capture\$($prefix)_%05d.png")
$idx = 1
$logoIdx = -1
if ($endCard) { $inputs += @('-loop', '1', '-t', ([string]$dur), '-i', "$Proj\ArtSource\Logo\Logo_Judul.png"); $logoIdx = $idx; $idx++ }

# ---- Audio: dicampur dulu ke satu WAV (FFmpeg tersendat bila banyak input audio diproses bersama gambar) ----
$mix = "D:\Tmp\mix_promo$Id.wav"
$ain = @(); $aParts = @(); $aLabels = @(); $ai = 0
foreach ($c in $cues) {
  $ain += @('-i', (Find-Wav $c[1])); $ms = [int]([double]$c[0] * 1000)
  $aParts += "[$($ai):a]aformat=channel_layouts=stereo,adelay=$($ms):all=1[a$ai]"; $aLabels += "[a$ai]"; $ai++
}
if ($music) {
  $ain += @('-i', "$Proj\ArtSource\Audio\$music")
  $aParts += "[$($ai):a]aformat=channel_layouts=stereo,atrim=0:$dur,volume=0.35,afade=t=in:d=0.4[a$ai]"; $aLabels += "[a$ai]"; $ai++
}
$ain += @('-f', 'lavfi', '-t', ([string]$dur), '-i', 'anullsrc=r=48000:cl=stereo'); $aLabels += "[$($ai):a]"; $ai++
$afg = ($aParts -join ';') + $(if ($aParts.Count) { ';' } else { '' }) + ($aLabels -join '') + "amix=inputs=$($aLabels.Count):normalize=0:duration=longest,atrim=0:$dur,afade=t=out:st=$([math]::Max(0, $dur - 0.5)):d=0.5[aout]"
Set-Content -Path "D:\Tmp\fg_audio$Id.txt" -Value $afg -Encoding ascii
Invoke-FF (@('-y', '-v', 'error', '-nostdin') + ($ain | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }) + @('-/filter_complex', "D:\Tmp\fg_audio$Id.txt", '-map', '[aout]', '-ar', '48000', $mix)) 'audio'
$inputs += @('-i', $mix); $mixIdx = $idx; $idx++

function Text-Filters([string]$mode) {
  $f = @()
  foreach ($t in $texts) {
    $y = if ($mode -eq 'vert') { if ($t.Y -eq 'tengah') { '(h-th)/2' } else { '470' } } else { if ($t.Y -eq 'tengah') { '(h-th)/2' } else { 'h-190' } }
    $size = if ($mode -eq 'vert') { $t.Size } else { [int]($t.Size * 1.15) }
    $f += "drawtext=fontfile='$font':text='$(Esc $t.T)':fontsize=$($size):fontcolor=$($t.Color):borderw=6:bordercolor=black:x=(w-tw)/2:y=$($y):enable='between(t,$($t.A),$($t.B))':$(Fade $t.A)"
  }
  $f
}
function EndCard-Filters([string]$mode) {
  if (-not $endCard) { return @() }
  $s = $clip + 0.15
  $y1 = if ($mode -eq 'vert') { 1250 } else { 760 }
  @("drawtext=fontfile='$font':text='$(Esc $endCard.Text)':fontsize=$(if ($mode -eq 'vert') { 78 } else { 84 }):fontcolor=white:borderw=5:bordercolor=black:x=(w-tw)/2:y=$($y1):enable='gte(t,$s)':$(Fade $s)",
    "drawtext=fontfile='$font':text='$(Esc $endCard.Text2)':fontsize=$(if ($mode -eq 'vert') { 104 } else { 110 }):fontcolor=0xFFD43B:borderw=6:bordercolor=black:x=(w-tw)/2:y=$($y1 + $(if ($mode -eq 'vert') { 100 } else { 105 })):enable='gte(t,$($s + 0.5))':$(Fade ($s + 0.5))")
}
function Render([string]$fg, [string]$name) {
  Set-Content -Path "D:\Tmp\fg_$name$Id.txt" -Value $fg -Encoding ascii
  $q = $inputs | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }
  Invoke-FF (@('-y', '-v', 'error', '-nostdin') + $q + @('-/filter_complex', "D:\Tmp\fg_$name$Id.txt", '-map', '[vout]', '-map', "$($mixIdx):a", '-t', ([string]$dur),
    '-c:v', 'libx264', '-crf', '18', '-preset', 'medium', '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-b:a', '192k', '-movflags', '+faststart', "$out\post$($Id)_$name.mp4")) $name
}

# ---- Landscape 1920x1080 ----
$land = "$v;"
if ($endCard) { $land += "[$($logoIdx):v]scale=1100:-1,format=rgba[logo];[base][logo]overlay=(W-w)/2:110:enable='gte(t,$clip)'[b2];" } else { $land += "[base]null[b2];" }
$lf = @(Text-Filters 'land') + @(EndCard-Filters 'land')
$land += "[b2]" + $(if ($lf.Count) { $lf -join ',' } else { 'null' }) + "[vout]"
if ($ShowFilter) { $land; $afg; return }
Render $land 'landscape'

# ---- Vertikal 1080x1920: latar buram + video di tengah + teks atas + nama game bawah ----
$vert = "$v;[base]split=2[s1][s2];[s1]scale=-2:1920,crop=1080:1920,boxblur=24:2,eq=brightness=-0.22[bg];[s2]scale=1080:-2[fg];[bg][fg]overlay=0:(H-h)/2[b1];"
if ($endCard) { $vert += "[$($logoIdx):v]scale=960:-1,format=rgba[logo];[b1][logo]overlay=(W-w)/2:650:enable='gte(t,$clip)'[b2];" } else { $vert += "[b1]null[b2];" }
$brand = @("drawtext=fontfile='$font':text='JURAGAN RENTAL':fontsize=88:fontcolor=white:borderw=6:bordercolor=black:x=(w-tw)/2:y=1360:enable='lt(t,$clip)'",
           "drawtext=fontfile='$fontS':text='simulator rental PS dan warnet':fontsize=38:fontcolor=white@0.85:x=(w-tw)/2:y=1470:enable='lt(t,$clip)'")
$vert += "[b2]" + ((@(Text-Filters 'vert') + $brand + @(EndCard-Filters 'vert')) -join ',') + "[vout]"
Render $vert 'vertikal'

Get-ChildItem "$out\post$($Id)_*.mp4" | ForEach-Object { '{0}  {1:N1} MB  {2:N1} dtk' -f $_.Name, ($_.Length / 1MB), $dur }