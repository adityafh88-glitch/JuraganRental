---
name: jr-android
description: Build Juragan Rental untuk Android — AAB + APK rilis yang ditandatangani untuk Play Store, atau APK Development untuk dicoba di HP. Pakai saat diminta "build android", "buat apk", "buat aab", "siapkan rilis play store", atau naik versi.
---

# Build Android Juragan Rental

Skrip: `Tools/build_android.ps1 -Mode Rilis|Dev` (PowerShell). Skrip ini mengatur JDK 21 dan `TEMP=D:\Tmp`, menjalankan RunUAT, menyalin hasil, mengecek tanda tangan, memindai dengan Defender, dan mencetak SHA-256.

## Langkah

1. **Naik versi dulu** bila build ini akan diunggah ke Play Console. Di `Config/DefaultEngine.ini`:
   - `StoreVersion=` naikkan 1. Wajib untuk setiap unggahan, karena Play menolak kode versi yang sama.
   - `VersionDisplayName=` ubah sesuai versinya (misalnya 0.9.1).
2. Pastikan kode sudah terkompilasi tanpa error (skill `jr-build`).
3. Jalankan di background, karena prosesnya memakan waktu 30–60 menit:
   ```powershell
   & 'D:\Dev\JuraganRental\Tools\build_android.ps1' -Mode Rilis
   ```
   Pakai `-Mode Dev` untuk APK Development, misalnya untuk mencoba fitur di HP.
4. Hasil:
   - Rilis: `Build\PlayStore\JuraganRental-v<versi>.aab` (unggah ke Play Console) dan `JuraganRental-v<versi>-rilis.apk` (universal, untuk dipasang langsung).
   - Dev: `Build\Android_ASTC\`.
5. Laporkan ukuran file, baris tanda tangan (harus `CN=KAH Gamedev`), hasil Defender, dan SHA-256.

## Aturan

- **Menu developer:** `Config/DefaultGame.ini` → `[JuraganRental] bDevMenu=True` menyalakan cheat tersembunyi (HP > Atur > ketuk baris versi 7x: lompat era, kas, level, reputasi). Boleh menyala untuk uji internal/tertutup, tetapi **wajib `bDevMenu=False`** sebelum build yang diunggah ke jalur **produksi**. Tanyakan pengguna bila ragu.

- **Jangan pernah membaca, mencetak, atau meng-commit** `Config/Android/AndroidEngine.ini` (berisi kata sandi kunci) maupun file `*.keystore`. Kalau kunci hilang, pengguna sendiri yang menjalankan `Tools/buat_kunci_upload.ps1`.
- Nama paket `com.juraganrental.game` **permanen** setelah unggahan pertama ke Play Console. Jangan ubah `PackageName` tanpa persetujuan pengguna.
- Konfigurasi penting yang tidak boleh diubah sembarangan: `bBuildForES31=True` (cadangan untuk HP yang crash di Vulkan), `bEnableBundle=True`, plugin AFS dimatikan.
- Bila build gagal: baca 40 baris terakhir `D:\Tmp\android_<Mode>.log`, cari `ERROR:` atau `error:` pertama.
