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

**Audio dan visual**
- 55 suara karakter dari ElevenLabs (Bocil dinaikkan nadanya 1,15), 5 lagu radio di meja kasir, efek suara hasil sintesis Python.
- Model low-poly dibuat di Blender (`ArtSource/Props/JR_RentalProps.blend`).
- Pemandangan jalan kampung di luar, dan tampak depan gedung (papan nama, kanopi, jendela, spanduk) untuk setiap era.

## Cara kerja teknis (penting)
- **Build editor** (Unreal Editor harus tertutup):
  `& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' JuraganRentalEditor Win64 Development '-Project=D:\Dev\JuraganRental\JuraganRental.uproject' -WaitMutex`
- **Paket `.exe` rilis:** RunUAT BuildCookRun `-platform=Win64 -clientconfig=Shipping`, hasilnya ke `Build\Windows`. ZIP terakhir: `Build\JuraganRental_PC_Demo_v0.2.zip`, versi sebelum tahap B, jadi perlu dibuat ulang.
- **APK Android:** RunUAT BuildCookRun `-platform=Android -cookflavor=ASTC`, dengan `JAVA_HOME=D:\Dev\Tools\jdk-21`, `TEMP` dan `JAVA_TOOL_OPTIONS` mengarah ke `D:\Tmp`, dan NDK 27.2. APK terakhir sudah lama, sebelum radio.
- **Uji otomatis** (`UnrealEditor.exe <uproject> -game -windowed ...`):
  - Dasar: `-jrautotest`.
  - Tambahan untuk `-jrautotest`: `-jrera2`, `-jrera3`, `-jrstaff`, `-jrdecor`, `-jrtiers`, `-jrnet`, `-jrloak`, `-jrguests`, `-jrtuttest`, `-jrdemo`, `-jrbilling`, `-jrtour`, `-jrmovetest`, `-jrfasad`, `-jrluar`, `-jrenglish`.
  - Uji berdiri sendiri: `-jrmenutest`, `-jrmysterytest`, dan bot ekonomi `-jrbalance -jrbaldays=N` (sebaiknya dengan `-nullrhi`).
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
1. **Uji main langsung oleh kamu** untuk tahap B (Era 3, billing, karyawan, misi, sampah, turnamen, misteri bab 2), lalu perbaikan dari masukan.
2. **Latar belakang** di belakang gedung masih bukit gurun bawaan level. Ganti dengan rumah, pohon, dan tiang listrik.
3. **Keseimbangan ekonomi Era 3** belum diuji dengan bot.
4. **Berikutnya: versi Android setara PC.**
   - Build APK baru.
   - Uji kontrol sentuh untuk fitur baru: billing, tangga, menu, misi.
   - Performa di HP.
   - Tata letak UI di layar kecil.
5. **Tahap C (Steam)** ditunda: achievement, cloud save, halaman toko, trailer.
