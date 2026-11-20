---
name: jr-uji
description: Menjalankan uji otomatis Juragan Rental di PC (bot bermain sendiri, simulasi sentuh, menu, tutorial) dan memotret jendela game. Pakai setelah compile untuk memastikan fitur jalan, saat diminta "tes", "uji", "cek apakah berhasil", atau "screenshot game".
---

# Uji otomatis

```powershell
& 'D:\Dev\JuraganRental\Tools\run_autotest.ps1' -GameArgs '-jrautotest -jrera3 -jrpaket1' -Timeout 240
```

Skrip ini menjalankan game (selalu `-unattended -nosplash`), menunggu game keluar sendiri (`JR_AUTOTEST_DONE` lalu `RequestExit`) atau sampai batas waktu, lalu mencetak semua baris `JR_*` dan tanda crash. Kode harus sudah dikompilasi dulu (skill `jr-build`).

## Memotret

Tambahkan `-Shot 'D:\Tmp\nama.png' -ShotAfter 30`. Yang dipotret hanya **jendela game** (lewat PrintWindow). **Jangan pernah memotret seluruh layar**, karena layar pengguna bisa berisi browser atau data pribadi. Lihat hasilnya dengan Read untuk mengecek tampilan.

## Flag yang tersedia

| Tujuan | Flag |
| --- | --- |
| Bot bermain sehari | `-jrautotest` (+ `-jrera2` / `-jrera3` untuk era) |
| OB / staf | `-jrstaff` (log `JR_OBTEST`) |
| Level, upgrade, bisnis sampingan | `-jrera3 -jrpaket1` (log `JR_PAKET1*`) |
| Tab HP | `-jrphonetab=N` (0 Toko, 1 Unit, 2 Kelola, 3 Viralin, 4 Info, 5 Atur) |
| Kontrol sentuh | `-jrtouch` (mouse = jari); `-jrtouchflow` menguji menu, swipe, jari nyangkut, dan tombol AKSI (log `JR_TOUCHFLOW`) |
| Menu utama / tutorial | `-jrmenutest`, `-jrtuttest` |
| Opening garasi | `-jropening`, `-jropeningat=<detik>` (bekukan untuk dipotret), `-jrnoopening` |
| Splash | `-jrnosplash`, `-jrsplashat=<detik>` |
| Sosial | `-jrnametest` (dialog nama), `-jrmoment` (Kartu Momen), log `JR_SOSIAL`, `JR_UPSELL` |
| Lainnya | `-jrfasad`, `-jrluar`, `-jrdecor`, `-jrnet`, `-jrbilling`, `-jrenglish`, `-jrdemo` |
| Bot ekonomi | `-jrbalance -jrbaldays=N -nullrhi` |

## Membaca hasil

- Lulus: muncul `JR_AUTOTEST_DONE ...` dengan angka pelanggan/pendapatan yang masuk akal, dan tidak ada bagian `--- CRASH ---`.
- Crash: baca `Saved\Logs\JuraganRental.log` di sekitar `Fatal error` / `Assertion failed`.
- Untuk fitur baru, tambahkan log `UE_LOG(LogTemp, Display, TEXT("JR_<NAMA> ..."))` supaya bisa diverifikasi dari skrip ini.
