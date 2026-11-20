param(
    # Flag uji tambahan, mis. '-jrautotest -jrera3 -jrpaket1' atau '-jrtouchflow -jrtouch'
    [string]$GameArgs = '-jrautotest',
    # Batas waktu (detik) sebelum game dimatikan paksa
    [int]$Timeout = 240,
    # Opsional: potret jendela game (BUKAN seluruh layar) setelah $ShotAfter detik
    [string]$Shot = '',
    [int]$ShotAfter = 35
)
# Jalankan uji otomatis Juragan Rental dan tampilkan baris log JR_*.
$Proj = 'D:\Dev\JuraganRental'
$log = "$Proj\Saved\Logs\JuraganRental.log"
$exe = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$start = Get-Date
# -unattended wajib: mencegah dialog crash reporter muncul di layar pengguna
$p = Start-Process -FilePath $exe -ArgumentList ("`"$Proj\JuraganRental.uproject`" -game -windowed -ResX=1600 -ResY=720 -nosplash -unattended " + $GameArgs) -PassThru

if ($Shot) {
    Start-Sleep -Seconds $ShotAfter
    & "$PSScriptRoot\capwin.ps1" -Process $p -Out $Shot
}
if (-not $p.WaitForExit($Timeout * 1000)) { Stop-Process -Id $p.Id -Force; "WAKTU HABIS setelah $Timeout detik (game dimatikan)" }
'Durasi: {0:N0} detik, exit {1}' -f ((Get-Date) - $start).TotalSeconds, $p.ExitCode
if (Test-Path $log) {
    Select-String -Path $log -Pattern 'JR_[A-Z0-9_]+' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
    $err = Select-String -Path $log -Pattern 'Fatal error|Assertion failed|Unhandled Exception' | Select-Object -First 5
    if ($err) { '--- CRASH ---'; $err | ForEach-Object { $_.Line } }
}
