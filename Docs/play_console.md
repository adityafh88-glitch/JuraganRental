# Panduan isian Play Console — Juragan Rental

Dasar jawaban: game offline, tanpa akun/login, simpanan hanya di HP, tanpa pembelian dalam aplikasi,
dengan iklan berhadiah Google AdMob (Google Mobile Ads SDK + UMP). Izin: INTERNET, ACCESS_NETWORK_STATE,
ACCESS_WIFI_STATE, VIBRATE, WAKE_LOCK, MODIFY_AUDIO_SETTINGS, CHECK_LICENSE, AD_ID.

---

## 1. Menetapkan kebijakan privasi
Wajib berupa URL publik. Cara termudah: buat halaman di **Google Sites** (sites.google.com), tempel teks di bawah,
publikasikan, lalu salin URL-nya ke Play Console. Ganti `[EMAIL]` dengan email developer.

### Kebijakan Privasi — Juragan Rental
Terakhir diperbarui: 24 November 2026

Juragan Rental ("game") dibuat oleh KAH Gamedev. Halaman ini menjelaskan data apa yang dikumpulkan saat Anda bermain.

**Data yang kami simpan.** Game tidak memerlukan akun dan tidak meminta nama, email, nomor telepon, atau lokasi Anda.
Progres permainan (simpanan) dan pengaturan disimpan hanya di perangkat Anda dan tidak dikirim ke server kami.
Menghapus game akan menghapus data tersebut.

**Iklan.** Game menampilkan iklan berhadiah dari Google AdMob. Iklan hanya tampil jika Anda memilih menontonnya.
Untuk menayangkan dan mengukur iklan, Google AdMob dapat mengumpulkan dan memproses data seperti ID iklan perangkat
(Advertising ID), alamat IP dan perkiraan lokasi yang diturunkan darinya, informasi perangkat, serta interaksi dengan iklan.
Data ini dikelola oleh Google sesuai kebijakan privasinya: https://policies.google.com/privacy dan
https://policies.google.com/technologies/partner-sites . Anda dapat mereset atau menghapus ID iklan, serta menonaktifkan
iklan yang dipersonalisasi, melalui Setelan Android > Google > Iklan. Pengguna di Wilayah Ekonomi Eropa dan Inggris
akan diminta persetujuan sebelum iklan dipersonalisasi.

**Anak-anak.** Game ditujukan untuk pengguna berusia 13 tahun ke atas. Kami tidak dengan sengaja mengumpulkan data
pribadi anak di bawah 13 tahun.

**Keamanan.** Data yang dikirim oleh SDK iklan Google dienkripsi saat transit (HTTPS).

**Perubahan.** Kebijakan ini dapat diperbarui; tanggal di atas menunjukkan versi terbaru.

**Kontak.** Pertanyaan tentang privasi: [EMAIL]

---

## 2. Detail login
**"Semua fungsi di aplikasi saya tersedia tanpa batasan akses apa pun."** (tidak ada login)

## 3. Iklan
**Ya, aplikasi saya berisi iklan.**

## 4. Rating konten (kuesioner IARC)
- Email kontak: email developer. Kategori: **Game**.
- Kekerasan: **Tidak** ada kekerasan grafis. (Ada kejadian "pelanggan berantem/membanting stik" sebatas teks dan
  animasi marah tanpa luka/darah — jika ada pertanyaan "kekerasan kartun/ringan", jawab sesuai penilaianmu; aman memilih Tidak.)
- Konten seksual, ketelanjangan: **Tidak**.
- Bahasa kasar: **Tidak** (hanya bahasa santai "wkwk", "Bang").
- Narkoba, alkohol, rokok: **Tidak**.
- Perjudian/simulasi perjudian: **Tidak** (tidak ada taruhan; uang dalam game bukan uang sungguhan).
- Interaksi antarpengguna/obrolan/berbagi konten: **Tidak**.
- Berbagi lokasi pengguna: **Tidak**.
- Pembelian digital: **Tidak**.
- Konten yang dibuat pengguna: **Tidak** (nama rental hanya tersimpan di HP sendiri).

## 5. Audiens target
- Kelompok usia: centang **13–15, 16–17, 18 ke atas** (jangan centang di bawah 13 → tidak wajib Program Keluarga).
- "Apakah aplikasi Anda mungkin menarik bagi anak-anak?": **Tidak** (tema bisnis rental, target remaja–dewasa).

## 6. Keamanan Data
Jawaban berikut mengikuti panduan pengungkapan data Google Mobile Ads SDK (cek ulang di
https://developers.google.com/admob/android/privacy/play-data-disclosure — isinya bisa berubah):
- Apakah aplikasi mengumpulkan/membagikan data jenis wajib? **Ya**.
- Semua data dienkripsi saat transit? **Ya**.
- Cara meminta penghapusan data: tidak ada akun; pilih opsi yang sesuai (pengguna dapat mereset ID iklan di setelan perangkat).
- Jenis data (semua: **dikumpulkan & dibagikan**, diproses sementara: tidak, wajib/opsional: sesuai panduan AdMob):
  - **Lokasi → Perkiraan lokasi** (dari IP) — Iklan atau pemasaran, Analisis, Pencegahan penipuan.
  - **Info dan performa aplikasi → Log error/crash, Diagnostik** — Analisis, Pencegahan penipuan.
  - **ID perangkat atau lainnya** (Advertising ID) — Iklan atau pemasaran, Analisis, Pencegahan penipuan.
  - **Aktivitas aplikasi → Interaksi aplikasi** (interaksi dengan iklan) — Iklan atau pemasaran, Analisis.
- Data lain (nama, email, kontak, foto, pembayaran, dll.): **tidak dikumpulkan**.

## 7. Aplikasi pemerintah — **Tidak**
## 8. Fitur keuangan — **Aplikasi saya tidak menyediakan fitur keuangan apa pun**
## 9. Kesehatan — **Tidak ada fitur kesehatan**
## 10. Status pedagang (aturan Uni Eropa / DSA)
Karena game menghasilkan uang dari iklan, jawaban yang tepat biasanya **"Saya adalah pedagang"**; akibatnya alamat
dan nomor telepon dari profil pembayaran ditampilkan ke pengguna di Uni Eropa. Jika tidak mau, alternatifnya tidak
merilis di negara-negara Uni Eropa (atur di Negara/wilayah). Putuskan sendiri; ini bukan nasihat hukum.

---

## 11. Kategori dan detail kontak
- Aplikasi atau game: **Game** — Kategori: **Simulasi**
- Tag (pilih yang tersedia): Simulasi, Manajemen bisnis, Kasual, Offline, Satu pemain
- Email: email developer (tampil publik). Situs web: opsional (bisa URL Google Sites kebijakan privasi).

## 12. Listingan Play Store
- Nama aplikasi (maks. 30): **Juragan Rental**
- Ikon: `ArtSource/Logo/Ikon_512.png`
- Feature graphic: `Build/Promo/Store/feature_1024x500.png`
- Screenshot ponsel (8): `Build/Promo/Store/01_…png` s.d. `08_…png`
- Video: unggah `Build/Promo/Store/trailer_1080p.mp4` ke YouTube (boleh unlisted), tempel link-nya.

### Deskripsi singkat (maks. 80 karakter) — 74 karakter
Bangun rental PS & warnet dari garasi rumah sampai Game Center 2 lantai!

### Deskripsi lengkap
Dulu dijewer Emak di rental PS... sekarang kamu juragannya! 🎮

Juragan Rental adalah game simulasi rental PS dan warnet ala kampung Indonesia. Mulai dari garasi rumah dengan dua TV
tabung, satu PC bekas, dan MCB 1.300 VA yang gampang jeglek. Layani pelanggan, tagih di kasir, masak mi instan, dan
kembangkan usahamu sampai jadi Game Center 2 lantai yang jadi legenda kampung.

🏠 TIGA ERA USAHA
• Garasi rumah — semua dikerjakan sendiri: nyalakan TV, nagih, masak mi
• Ruko kecil — sistem billing, karyawan (kasir, teknisi, koki, OB), turnamen PS tiap Sabtu
• Game Center 2 lantai — lounge VIP, kulkas minuman, konter pulsa, printer

🍜 HIDUP SEHARI-HARI ANAK RENTAL
• "Bang, nambah sejam!" — tambah jam main pelanggan
• MCB jeglek pas lagi rame, CD macet (coba balik PS-nya!), stik rusak, PC kena virus
• Bocil dicari Emak, pelanggan rage quit, Pak RT negur jam 10 malam
• Mini-game kembalian pas di kasir, Jumat Berkah, sewa PS bawa pulang

👨‍👩‍👦 CERITA KELUARGA & IMPIAN
• Adik minta uang SPP, atap bocor pas hujan, Bapak pinjam TV buat sinetron
• Wujudkan impian: HP buat adik, dapur baru buat Emak, sampai umrahkan Emak
• 17 lencana dan 5 ending berbeda

✨ FITUR LAIN
• Bisa dimainkan offline, ringan di HP
• Bahasa Indonesia dan Inggris
• Simpan otomatis

Dibuat di Indonesia oleh KAH Gamedev. Ikuti kami di Threads @kahgamedev — semua masukan warga rental kami baca!

(Catatan merek: jangan tambahkan kata "PlayStation"/logo merek asli di listingan; "rental PS" sebagai istilah umum.)
