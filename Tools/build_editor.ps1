# Compile kode C++ untuk editor/uji otomatis. Unreal Editor HARUS tertutup.
$ErrorActionPreference = 'Stop'
$ed = Get-Process UnrealEditor -ErrorAction SilentlyContinue
if ($ed) { throw 'Unreal Editor masih terbuka. Tutup dulu (simpan pekerjaan), lalu jalankan lagi.' }
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' JuraganRentalEditor Win64 Development '-Project=D:\Dev\JuraganRental\JuraganRental.uproject' -WaitMutex
if ($LASTEXITCODE -ne 0) { throw "Compile gagal (exit $LASTEXITCODE)" }
'Compile berhasil.'
