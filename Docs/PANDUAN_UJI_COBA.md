# Panduan Uji Coba Juragan Rental (Android) — versi 0.9.0

Panduan ini untuk kamu dan teman-teman yang ikut menguji game sebelum rilis di Play Store.

## 1. File yang dipakai

| File | Untuk |
|---|---|
| `Build\Rilis\JuraganRental-v0.9.0-uji-gles.apk` | **Dibagikan ke penguji.** Build Shipping: ringan, tanpa konsol debug, mendukung Vulkan dan OpenGL ES (HP yang Vulkan-nya bermasalah tidak crash lagi). |
| `Build\Android_ASTC\JuraganRental-arm64.apk` | Build Development, khusus kamu. Ketuk layar dengan 4 jari untuk membuka konsol, lalu ketik `stat fps` untuk melihat FPS. |

Sebelum membagikan, pindai dulu dengan Windows Defender (klik kanan → *Scan with Microsoft Defender*).

## 2. Syarat HP penguji
- Android 8.0 atau lebih baru, dengan chip 64-bit (hampir semua HP sejak 2018).
- Ruang kosong sekitar 500 MB.
- Game berjalan miring (landscape).

## 3. Cara memasang
1. Kirim APK lewat Google Drive, Telegram, atau kabel. Lewat WhatsApp kadang gagal karena ukurannya besar.
2. Di HP, buka file APK. Kalau diminta, izinkan *Pasang aplikasi tidak dikenal* untuk aplikasi pembuka file tersebut.
3. Kalau muncul peringatan Play Protect, pilih **Tetap instal**. Ini wajar untuk aplikasi di luar Play Store.
4. Kalau versi lama masih terpasang dan pemasangan gagal, hapus dulu yang lama. Simpanan permainan ikut terhapus.

## 4. Kontrol singkat (sampaikan ke penguji)
- **Joystick kiri:** berjalan.
- **Geser jari di sisi kanan layar:** melihat sekitar.
- **Titik putih di tengah layar:** bidikan. Arahkan ke benda, lalu ketuk **AKSI**. Untuk memperbaiki, top-up, atau print, tahan AKSI sampai bilah penuh.
- **Tombol HP** (kiri atas): toko, upgrade unit, tarif, karyawan, dan pengaturan.
- **Tombol jeda** (kiri atas): menu jeda dan pengaturan.
- **Panel misi:** ketuk judul MISI untuk menciutkan atau membukanya.
- Kalau teks atau joystick terasa terlalu kecil atau besar, ubah di **HP > Atur** atau **Jeda > Pengaturan**: Ukuran tampilan, Ukuran joystick, dan Kepekaan geser layar.

## 5. Daftar uji
Centang yang sudah dicoba dan catat yang aneh.

**Sesi 1 — Awal permainan (± 20 menit)**
- [ ] Splash KAH Gamedev, logo, dan menu utama tampil rapi.
- [ ] "Main baru" memutar opening garasi. Tombol Lewati berfungsi.
- [ ] Tutorial 5 langkah bisa diselesaikan dengan kontrol sentuh.
- [ ] Menyalakan unit, menagih di kasir, memasak mi, dan mengantar mi.
- [ ] MCB turun lalu dinaikkan lagi; unit rusak bisa diperbaiki dengan menahan AKSI.
- [ ] Belanja di HP > Toko, mengangkat kardus kurir, memasang unit di tanda hijau.
- [ ] Toko tutup → laporan harian tampil → simpan otomatis.

**Sesi 2 — Garasi ke Ruko (± 1 jam main)**
- [ ] Gerbang 1 terbuka, lalu pindah ke Ruko.
- [ ] Rekrut karyawan (kasir, teknisi, koki, OB). Mereka benar-benar bekerja.
- [ ] Kejadian: Emak, Pak RT, ngutang, rage quit, joki, tamu spesial.
- [ ] Turnamen akhir pekan (hari ke-6).
- [ ] Naik Level Juragan dan upgrade unit di HP > Unit.

**Sesi 3 — Game Center (lanjutan)**
- [ ] Pindah ke Game Center 2 lantai, naik tangga ke VIP.
- [ ] Kulkas minuman, konter pulsa, dan printer.
- [ ] Cerita misteri bab 1 dan 2.

**Selalu perhatikan**
- [ ] Game patah-patah atau tersendat? Di mana?
- [ ] HP terasa panas? Setelah berapa menit?
- [ ] Baterai turun berapa persen per 30 menit main?
- [ ] Ada teks terpotong, tombol tertutup poni kamera, atau tombol yang sulit dijangkau?
- [ ] Game keluar sendiri (crash)?
- [ ] Keluar lalu buka lagi: lanjutan permainan masih ada?

## 6. Cara melaporkan masalah
Minta penguji mengirim laporan dengan format ini (bisa lewat WhatsApp atau Google Form):

```
HP & versi Android : (mis. Redmi Note 12, Android 13)
Sedang apa        : (mis. hari ke-4 di Ruko, mengantar mi)
Yang terjadi      : (mis. game keluar sendiri)
Bisa diulang?     : ya / tidak / kadang
Bukti             : screenshot atau rekaman layar
```

Untuk saran umum, pertanyaan Google Form yang berguna:
1. Dari 1–10, seberapa seru game ini?
2. Bagian paling seru apa? Bagian paling membosankan apa?
3. Ada yang membingungkan di 10 menit pertama?
4. Kontrol nyaman? (joystick, geser layar, tombol AKSI)
5. Mau membayar berapa untuk versi lengkap, atau lebih suka gratis dengan iklan?
6. HP apa yang dipakai, dan apakah lancar?

## 7. Uji coba lewat Google Play (sebelum rilis resmi)
Akun developer **pribadi** baru di Google Play wajib menjalankan **uji tertutup (closed testing)** sebelum bisa rilis ke publik: minimal **12 penguji** yang ikut selama **14 hari berturut-turut**. Cek angka terbarunya di Play Console karena aturan ini bisa berubah. Jadi mulailah mengumpulkan penguji dari sekarang, misalnya lewat media sosial (lihat `IDE_MARKETING.md`).

Langkah-langkahnya:
1. Daftar **Google Play Console** (biaya sekali bayar USD 25) dengan nama developer **KAH Gamedev**, lalu verifikasi identitas.
2. **Buat kunci upload** (lihat bagian 8). Aku bisa menyiapkan build **AAB** (format wajib Play Store) begitu kuncinya ada.
3. Buat aplikasi baru di Play Console, lalu isi:
   - **halaman toko:** nama, deskripsi, ikon 512×512 (`ArtSource\Logo\Ikon_512.png`), header 1024×500 (`ArtSource\Logo\Header_1024x500.png`), minimal 2 screenshot;
   - **kebijakan privasi** (wajib, berupa link). Game ini tidak mengumpulkan data pribadi; aku bisa membantu menuliskannya;
   - **Data safety:** tidak ada data yang dikumpulkan atau dibagikan;
   - **rating konten** (kuesioner IARC);
   - **target usia:** sarankan **13 tahun ke atas**. Kalau memilih untuk anak di bawah 13 tahun, berlaku aturan program Families yang lebih ketat.
4. Mulai dari **Internal testing** (maksimal 100 orang, langsung tersedia) untuk tes cepat bersama teman.
5. Lanjut ke **Closed testing** dengan 12 penguji atau lebih selama 14 hari. Penguji bergabung lewat link undangan dengan akun Google mereka.
6. Setelah syarat terpenuhi, ajukan akses **Production**, lalu rilis.

**Penting:** nama paket aplikasi saat ini `com.juraganrental.game`. Nama ini **tidak bisa diganti** setelah aplikasi pertama kali diunggah ke Play Console. Kalau mau memakai nama developer (misalnya `com.kahgamedev.juraganrental`), putuskan sebelum unggahan pertama.

## 8. Kunci upload (dibuat sendiri, jangan dibagikan)
Kunci ini membuktikan bahwa update game berasal dari kamu. Kalau hilang, kamu tidak bisa memperbarui game di Play Store (kecuali lewat proses pemulihan Google). Simpan di dua tempat: laptop dan flashdisk atau cloud pribadi.

Jalankan di PowerShell, lalu isi kata sandi dan data yang diminta (pakai kata sandimu sendiri):

```powershell
& 'D:\Dev\Tools\jdk-21\bin\keytool.exe' -genkeypair -v -keystore 'D:\Dev\JuraganRental\Build\Android\kahgamedev-upload.keystore' -alias kahgamedev -keyalg RSA -keysize 2048 -validity 10000
```

Setelah selesai, kabari aku nama file dan alias-nya (bukan kata sandinya). Aku akan menyiapkan pengaturan build AAB supaya kata sandinya kamu isi sendiri di file lokal yang tidak ikut ter-commit ke GitHub.

## 9. Sebelum rilis resmi
- [ ] Semua bug dari uji tertutup sudah diperbaiki.
- [ ] Performa diuji minimal di 1 HP kelas menengah ke bawah.
- [ ] Nomor versi dinaikkan (versi saat ini 0.9.0, kode versi 2).
- [ ] Kebijakan privasi online.
- [ ] Trailer dan screenshot siap (lihat `IDE_MARKETING.md`).
