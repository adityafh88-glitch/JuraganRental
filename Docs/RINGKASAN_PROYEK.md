# Ringkasan Proyek Juragan Rental (per 20 November 2026)

Dokumen ini merangkum seluruh pekerjaan sejauh ini, untuk melanjutkan di sesi baru.

## Tentang game
- **Juragan Rental**: simulator rental PS + warnet ala Indonesia, terinspirasi Warung Simulator (neopawstudio). Gaya visual low-poly kartun, sudut pandang orang pertama.
- **Unreal Engine 5.8, C++**, proyek di `D:\Dev\JuraganRental`, modul `JuraganRental`. Repo GitHub privat `adityafh88-glitch/JuraganRental`, branch `main`, Git LFS untuk aset.
- **Platform:** dibuat untuk PC dan Android sejak awal. **Keputusan terbaru: Android kemungkinan rilis duluan.** Setelah tahap B, berhenti sebelum urusan Steam (tahap C) dan fokus menyamakan versi Android dengan PC.
- Commit terakhir: `3bd65c5`.

## Isi game sekarang
**Tiga era**
- **Era 1, Garasi:** 4 TV dan 4 PC, daya 1.300 VA. Gerbang 1 terbuka di kas Rp 600.000 + reputasi 40, pindah Rp 450.000.
- **Era 2, Ruko Kecil:** 6 TV dan 6 PC, daya 2.900 VA (upgrade 4.400 VA), sewa Rp 75.000 + internet per hari. Gerbang 2 terbuka di kas Rp 3 juta + reputasi 60 + 10 unit, pindah Rp 1,5 juta.
- **Era 3, Game Center 2 lantai:** lantai 1 berisi 8 TV dan 8 PC, tangga 14 anak tangga di sisi kiri, lantai 2 VIP Lounge (sampai 6 booth VIP: sofa, TV besar, PlayStasiun 3, tarif Rp 8.000/jam), sewa Rp 150.000. Pelanggan VIP naik tangga. Target akhir "Juragan Sejati": Rp 10 juta + reputasi 85.

**Gameplay inti**
- Menyalakan unit, menagih di kasir, MCB yang turun kalau daya berlebih, unit rusak atau virus (tahan E untuk memperbaiki), kantin mi instan, kurir mengantar kardus besok pagi, buku utang, tarif bisa diatur.

**Sistem tambahan**
- **Sistem billing:** komputer di samping kasir untuk menyalakan semua unit.
- **Karyawan (Era 2+):** semuanya berjalan sungguhan.
  - kasir;
  - teknisi, yang naik tangga ke VIP kalau perlu;
  - koki, yang mengantar mi;
  - OB, yang memungut sampah.
- **Kualitas dan suasana:**
  - kursi busa dan gaming;
  - upgrade PC dan PS2;
  - dekorasi: poster, tanaman, kipas, neon, AC;
  - provider internet (NetRakyat, KilatNet, FiberNusa);
  - barang bekas dari loak.
- **Misi harian (3) dan mingguan (1)**, kebersihan dan sampah, turnamen akhir pekan (hari ke-6, 13, dan seterusnya, Era 2+).
- **Kejadian:**
  - Emak mencari bocil;
  - rage quit;
  - ngutang;
  - Pak RT;
  - joki;
  - berantem;
  - tamu spesial: streamer Bang Jago, Pak Lurah, juragan sebelah.
- **Cerita misteri:**
  - **Bab 1 "TV 1"** (Era 1+): 5 bab tentang Bayu, hadiah Kaset Emas.
  - **Bab 2 "Naga Online"** (Era 2+): 3 bab, hadiah server Naga.
- **Tutorial** 5 misi di hari pertama, dengan panah hijau. Simpan otomatis setiap toko tutup. Menu utama, menu jeda (Esc/P), dan pengaturan grafis, suara, kontrol, serta bahasa.
- **Mode demo Steam**: `bDemo` di `Config/DefaultGame.ini`. Berhenti di hari ke-7 atau saat Gerbang 1 terbuka, lalu ajakan wishlist (`DemoStoreURL` masih kosong).
- **Bahasa Indonesia dan Inggris**: semua teks lewat `JRT()`, sekitar 517 terjemahan, otomatis mengikuti bahasa Windows.

**Paket 1 permainan lanjutan** (lihat `Docs/USULAN_GAMEPLAY_LANJUTAN.md`, kode di `JRProgression.cpp` dan `JRService.*`)
- **Level Juragan (XP):** XP dari pelanggan bayar (+2, VIP +4), mi (+1), misi (hadiah/2.000, mingguan +120), turnamen (+40), dan layanan sampingan (+1). Rumus XP level: `60n² + 90n`, dengan Lv 2 di 150 XP, Lv 7 di 2.700 XP, Lv 13 di 9.720 XP, dan maksimal Lv 20. Barang toko terkunci level lewat `ItemMinLevel`. Level tampil di HUD, tab HP "Unit", tab Info, dan laporan harian. Simpanan lama mendapat XP sesuai era.
- **Upgrade unit Lv 1–5** (tab HP "Unit"): unit Lv 2/3/4/5 butuh Level Juragan 4/7/10/13.
  - Efek per level: tarif +Rp 500 (VIP +Rp 2.000), kepuasan +3, peluang rusak −12%, pengunjung +5% per rata-rata level.
  - Biaya perawatan Rp 1.500 per level per hari.
  - Pelanggan memilih unit kosong dengan level tertinggi.
  - Tampilan: TV PS jadi TV datar mulai Lv 2, PC pakai monitor LCD mulai Lv 4, plus lis LED berwarna.
  - Level disimpan per tipe dan indeks (`LvPS`/`LvPC`/`LvVIP`).
- **Bisnis sampingan Era 3**, berjajar di depan tangga lantai 1:
  - **kulkas minuman** (Lv 7): dibeli pelanggan saat bayar, diisi krat dari kurir;
  - **konter pulsa** (Lv 8): pakai saldo, untung dari biaya admin;
  - **printer** (Lv 10): Rp 500 per lembar, diisi kertas dari kurir.
  - Pembeli konter dan printer datang khusus dan dilayani dengan tahan E. Kasir ikut melayani bila sudah direkrut.

**Versi Android (layar sentuh)**
- **Kontrol:** joystick kiri untuk jalan, **geser jari di sisi kanan layar untuk menoleh** (`UJRLookPad`, tanpa joystick kanan). Tombol AKSI dan LANJUT ada di pojok kanan bawah. Tombol jeda (ikon dua batang) dan HP (berbentuk HP jadul) ada di kiri atas.
- **Tombol sentuh** diatur tampil/sembunyi oleh HUD tiap frame (`UJRTouchWidget::SyncVisibility`), karena widget yang Collapsed tidak menjalankan NativeTick.
- **Ukuran tampilan** (Kecil/Sedang/Besar/Sangat besar, bawaan Besar 1,8) mengatur `ApplicationScale` UMG. HUD Canvas ikut diskalakan lewat `UIS`. Pilihan **ukuran joystick** dan **kepekaan geser layar** ada di Pengaturan dan HP > Atur.
- **Splash KAH Gamedev:** ubin K/A/H muncul pop-up, lalu logo Juragan Rental membal masuk (`UJRSplashWidget`). Ketuk untuk melewati. Logo judul juga tampil di menu utama.
- **Logo dan ikon:** dibuat oleh `Tools/make_logo.ps1` (GDI+ dan font Luckiest Guy di `ArtSource/Fonts`, lisensi Apache 2.0). Hasilnya ada di `ArtSource/Logo`: logo judul, ikon 512, header Play Store 1024x500, dan ubin KAH. Ikon Android ditulis ke `Build/Android/res/drawable*/icon.png`. Logo diimpor ke `/Game/UI/Logo` dengan `Tools/import_logo.py`, dan `/Game/UI` selalu ikut dikemas.

**Audio dan visual**
- 55 suara karakter dari ElevenLabs (Bocil dinaikkan nadanya 1,15), 5 lagu radio di meja kasir, efek suara hasil sintesis Python.
- Model low-poly dibuat di Blender (`ArtSource/Props/JR_RentalProps.blend`).
- Pemandangan jalan kampung di luar, dan tampak depan gedung (papan nama, kanopi, jendela, spanduk) untuk setiap era.

## Cara kerja teknis (penting)
- **Build editor** (Unreal Editor harus tertutup):
  `& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' JuraganRentalEditor Win64 Development '-Project=D:\Dev\JuraganRental\JuraganRental.uproject' -WaitMutex`
- **Paket `.exe` rilis:** RunUAT BuildCookRun `-platform=Win64 -clientconfig=Shipping`, hasilnya ke `Build\Windows`. ZIP terakhir: `Build\JuraganRental_PC_Demo_v0.2.zip`, versi sebelum tahap B, jadi perlu dibuat ulang.
- **APK Android:** RunUAT BuildCookRun `-platform=Android -cookflavor=ASTC`, dengan `JAVA_HOME=D:\Dev\Tools\jdk-21`, `TEMP` dan `JAVA_TOOL_OPTIONS` mengarah ke `D:\Tmp`, dan NDK 27.2. Hasilnya di `Build\Android_ASTC\JuraganRental-arm64.apk` (build Development). Pindai dengan Windows Defender sebelum dikirim ke HP.
- **Uji otomatis** (`UnrealEditor.exe <uproject> -game -windowed ...`):
  - Dasar: `-jrautotest`.
  - Tambahan untuk `-jrautotest`: `-jrera2`, `-jrera3`, `-jrstaff` (juga uji OB, log `JR_OBTEST`), `-jrpaket1` (bersama `-jrera3`: level, upgrade, dan bisnis sampingan, log `JR_PAKET1*`), `-jrphonetab=N` (0 Toko, 1 Unit, 2 Kelola, 3 Info, 4 Atur), `-jrdecor`, `-jrtiers`, `-jrnet`, `-jrloak`, `-jrguests`, `-jrtuttest`, `-jrdemo`, `-jrbilling`, `-jrtour`, `-jrmovetest`, `-jrfasad`, `-jrluar`, `-jrenglish`.
  - Uji berdiri sendiri: `-jrmenutest`, `-jrmysterytest`, dan bot ekonomi `-jrbalance -jrbaldays=N` (sebaiknya dengan `-nullrhi`).
  - Mode sentuh di PC: `-jrtouch` (mouse dipakai sebagai jari). `-jrtouchflow` menguji alur menu utama, tombol sentuh, dan simulasi geser layar (log `JR_TOUCHFLOW`).
  - Splash: `-jrnosplash` untuk melewati splash, `-jrsplashat=detik` untuk membekukan animasi supaya bisa dipotret.
  - Selalu tambahkan `-unattended`, supaya dialog crash reporter tidak muncul di layar pengguna.
  - Hasil: log `JR_*` di `Saved/Logs`, screenshot di `Saved/AutoTest`.
  - Untuk main langsung di era tertentu: `-jrera2` atau `-jrera3`.
- **Teks baru:**
  1. Bungkus dengan `JRT(TEXT("..."))`, dan pakai `JRPrintf` untuk format.
  2. Jalankan `python Tools/wrap_strings.py`.
  3. Tambahkan baris `teks_indonesia<TAB>english` di `Tools/strings_en.tsv`.
  4. Jalankan `python Tools/make_lang_table.py`.
- **Aturan kode:**
  - Modul memakai `bUseUnity = false`.
  - Jangan memanggil `JRT` di inisialisasi statis global, karena build rilis crash.
  - Aset yang disimpan di variabel `static` wajib `TStrongObjectPtr`, karena raw pointer bisa dihapus garbage collector dan membuat game crash.
- **Pipeline audio dan aset:**
  - Suara: `Tools/import_audio.py`.
  - Props baru: `Tools/import_new_props.py`.
  - Konversi MP3 ke WAV memakai modul `aud` milik Blender.
  - Bot uji dan uji otomatis memakai slot simpanan terpisah `JRSave_Test`.

## Yang belum dan berikutnya
1. **Uji main langsung oleh kamu** untuk tahap B dan Paket 1. Masukan pertama sudah ditangani: OB yang jarang memungut sampah sudah diperbaiki (memilih sampah lewat jarak jalan sebenarnya dan berjalan lebih gesit), dan Paket 1 dibuat karena isi toko cepat habis. Paket 2 dan 3 ada di usulan.
2. **Latar belakang** di belakang gedung masih bukit gurun bawaan level. Ganti dengan rumah, pohon, dan tiang listrik.
3. **Keseimbangan ekonomi Era 3** belum diuji dengan bot.
4. **Versi Android (sedang dikerjakan, Android rilis duluan).**
   - Sudah: skala UI di layar kecil, tombol sentuh, geser layar, splash, logo, dan ikon.
   - Belum: tes performa di HP sungguhan (FPS, panas), opening adegan garasi, dan build Shipping dengan kunci rilis untuk Play Store.
5. **Tahap C (Steam)** ditunda: achievement, cloud save, halaman toko, trailer.
