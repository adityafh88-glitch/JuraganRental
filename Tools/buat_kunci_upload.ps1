# Membuat kunci upload Google Play untuk Juragan Rental (KAH Gamedev).
# Jalankan SENDIRI di PowerShell:  powershell -ExecutionPolicy Bypass -File D:\Dev\JuraganRental\Tools\buat_kunci_upload.ps1
#
# - Kata sandi diketik tersembunyi dan hanya disimpan di Config\Android\AndroidEngine.ini (tidak ikut ke GitHub).
# - File kunci: Build\Android\kahgamedev-upload.keystore (juga tidak ikut ke GitHub).
# - SIMPAN CADANGAN file kunci + kata sandinya di tempat aman (flashdisk / cloud pribadi).
#   Tanpa kunci ini kamu tidak bisa memperbarui game di Play Store.

$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$KeyFile = Join-Path $Root 'Build\Android\kahgamedev-upload.keystore'
$Alias = 'kahgamedev'
$Keytool = 'D:\Dev\Tools\jdk-21\bin\keytool.exe'
$IniDir = Join-Path $Root 'Config\Android'
$Ini = Join-Path $IniDir 'AndroidEngine.ini'

if (Test-Path $KeyFile) {
    Write-Host "Kunci sudah ada: $KeyFile" -ForegroundColor Yellow
    Write-Host 'Tidak dibuat ulang (kunci lama tetap dipakai). Hapus manual dulu kalau memang mau membuat baru.'
    exit 1
}

function Read-Plain([string]$Prompt) {
    $s = Read-Host -Prompt $Prompt -AsSecureString
    $b = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($s)
    try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($b) } finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($b) }
}

Write-Host '=== Kunci upload Juragan Rental (KAH Gamedev) ===' -ForegroundColor Cyan
Write-Host 'Buat kata sandi minimal 8 karakter. Ketikan tidak terlihat di layar.'
do {
    $Pw = Read-Plain 'Kata sandi baru'
    $Pw2 = Read-Plain 'Ulangi kata sandi'
    if ($Pw.Length -lt 8) { Write-Host 'Minimal 8 karakter, coba lagi.' -ForegroundColor Red; continue }
    if ($Pw -ne $Pw2) { Write-Host 'Tidak sama, coba lagi.' -ForegroundColor Red }
} while ($Pw.Length -lt 8 -or $Pw -ne $Pw2)

New-Item -ItemType Directory -Force (Split-Path $KeyFile) | Out-Null
& $Keytool -genkeypair -keystore $KeyFile -alias $Alias -keyalg RSA -keysize 2048 -validity 10000 `
    -storepass $Pw -keypass $Pw -dname 'CN=KAH Gamedev, OU=Games, O=KAH Gamedev, C=ID' | Out-Null
if (-not (Test-Path $KeyFile)) { throw 'Gagal membuat kunci.' }

# Kata sandi hanya di file lokal ini (diabaikan git). Lokasi & alias kunci ada di Config\DefaultEngine.ini.
New-Item -ItemType Directory -Force $IniDir | Out-Null
@"
; FILE RAHASIA - jangan dibagikan dan jangan di-commit (sudah ada di .gitignore).
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
KeyStorePassword=$Pw
KeyPassword=$Pw
"@ | Set-Content -Path $Ini -Encoding ASCII

$Pw = $null; $Pw2 = $null
Write-Host ''
Write-Host "Selesai! Kunci: $KeyFile" -ForegroundColor Green
Write-Host 'Sekarang SALIN file kunci itu ke flashdisk/cloud pribadi, dan catat kata sandinya di tempat aman.'
Write-Host 'Lalu kabari Claude bahwa kunci sudah dibuat (tanpa menyebut kata sandinya).'
