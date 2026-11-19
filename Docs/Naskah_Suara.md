# Naskah Suara — Juragan Rental

Naskah dialog untuk dibuat di ElevenLabs. Setiap baris sudah punya **nama file**. Game otomatis memakai file yang namanya cocok, dan baris yang belum ada filenya dilewati tanpa error, jadi kamu bisa mengisinya bertahap.

## Cara membuat di ElevenLabs

1. **Model:** pakai *Eleven v3* atau *Eleven Multilingual v2*. Keduanya mendukung bahasa Indonesia.
2. **Suara:** buat lewat **Voice Design** dari deskripsi karakter di bawah, atau pilih dari Voice Library. Jangan meng-clone suara orang sungguhan (artis, YouTuber, kenalan) tanpa izin tertulis.
3. **Pengaturan awal:** Stability 35–45% supaya ekspresif, Similarity 75%, Style 30–50% untuk karakter heboh (Bocil, Gamer, Emak).
4. **Satu baris = satu file.** Buat 2–3 versi, lalu pilih yang paling lucu.
5. **Format unduhan:** pilih **WAV** kalau tersedia. Kalau hanya MP3, tidak masalah, nanti saya konversi.
6. **Simpan ke:** `D:\Dev\JuraganRental\ArtSource\Audio\Voice\` dengan nama file persis seperti di tabel, misalnya `VO_Bocil_Datang_1.wav`.
7. Kabari saya, lalu saya impor ke Unreal.

Tanda `[...]` di kolom arahan bisa ditulis langsung di teks Eleven v3 sebagai *audio tag*, misalnya `[excited]`, `[whispers]`, `[shouting]`, `[sighs]`. Di Multilingual v2, cukup gunakan sebagai panduan saat memilih hasil.

---

## 1. Bocil berseragam
**Suara:** anak laki-laki 12 tahun, cempreng, cepat, semangat, sok jago. Logat Jakarta.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Bocil_Datang_1` | Bang, PS kosong? Main sejam ya, Bang! | [excited] buru-buru |
| `VO_Bocil_Datang_2` | Bang, nyalain dong, buruan… nanti ketahuan emak! | [whispers] panik |
| `VO_Bocil_Pesan_Mi_1` | Bang, mi goreng satu, telornya dua ya! | [excited] |
| `VO_Bocil_Rusak_1` | Bang! Stiknya nggak bisa nendang, Bang! | [shouting] kesal |
| `VO_Bocil_Rusak_2` | Yaaah, stiknya mati! Lagi seru-serunya! | merengek |
| `VO_Bocil_Bayar_1` | Nih, Bang… uang jajan saya. | [sighs] pasrah |
| `VO_Bocil_Marah_Pulang_1` | Ah lama, Bang. Pindah ke rental sebelah aja! | ngambek |
| `VO_Bocil_Ketahuan_1` | Ampun, Mak! Cuma sebentar, Mak! | [shouting] takut |

## 2. Anak SMA
**Suara:** remaja laki-laki 17 tahun, santai, gaul, agak cuek. Logat Jakarta.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_SMA_Datang_1` | Bang, satu jam dulu. Nanti nambah kalau menang. | santai |
| `VO_SMA_Datang_2` | Sore, Bang. TV yang kosong yang mana? | santai |
| `VO_SMA_Pesan_Mi_1` | Bang, mi rebus satu sama es teh, ya. | santai |
| `VO_SMA_Rusak_1` | Bang, ini stiknya jalan sendiri! | heran |
| `VO_SMA_Rage_1` | AAARGH! Penaltinya nggak masuk! | [shouting] frustrasi |
| `VO_SMA_Rage_2` | Curang! Itu offside, Bang! OFFSIDE! | [shouting] |
| `VO_SMA_Bayar_1` | Berapa, Bang? Oke, pas ya. | santai |
| `VO_SMA_Marah_Pulang_1` | Udah ah, kelamaan nunggu. | malas |

## 3. Mahasiswa
**Suara:** laki-laki 21 tahun, lelah, sopan tapi sedikit sarkas, bicara pelan.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Mahasiswa_Datang_1` | Mas, PC-nya ada yang kosong? Mau ngerjain tugas… katanya. | datar, lucu |
| `VO_Mahasiswa_Pesan_Mi_1` | Mas, mi instan satu ya. Belum makan dari pagi. | [sighs] lelah |
| `VO_Mahasiswa_Rusak_1` | Mas… PC-nya hang. Tugas saya belum ke-save. | putus asa |
| `VO_Mahasiswa_Lemot_1` | Mas, internetnya lemot banget. Loading-nya kayak skripsi saya. | sarkas |
| `VO_Mahasiswa_Bayar_1` | Ini, Mas. Makasih. | sopan |
| `VO_Mahasiswa_Marah_Pulang_1` | Ya udah, Mas. Saya ke warnet sebelah. | kecewa |

## 4. Mas-mas kantoran
**Suara:** pria 30 tahun, sopan, capek habis kerja, suara rendah.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Kantoran_Datang_1` | Mas, buat lepas penat. Satu jam, ya. | [sighs] lega |
| `VO_Kantoran_Pesan_Mi_1` | Mas, mi goreng sama kopi ada? | sopan |
| `VO_Kantoran_Rusak_1` | Waduh, kok mati, Mas? | kaget |
| `VO_Kantoran_Lemot_1` | Mas, koneksinya kayak Senin pagi. Macet. | sarkas |
| `VO_Kantoran_Ngutang_1` | Mas, gajian minggu depan. Catat dulu, ya. | malu-malu |
| `VO_Kantoran_Ngutang_2` | Aduh, dompet ketinggalan di motor, Mas. Besok saya bayar. | meyakinkan |
| `VO_Kantoran_Bayar_1` | Ini, Mas. Makasih, ya. | ramah |
| `VO_Kantoran_Marah_Pulang_1` | Lama banget. Saya ada meeting. | kesal tertahan |

## 5. Gamer online
**Suara:** pria 22 tahun, energik, cepat, penuh istilah gamer. Logat Jakarta.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Gamer_Datang_1` | Bang, PC paling kenceng! Gue mau push rank! | [excited] |
| `VO_Gamer_Pesan_Mi_1` | Bang, mi goreng anterin ke bilik ya, lagi war! | buru-buru |
| `VO_Gamer_Lemot_1` | BANG! PING-NYA MERAH! Gue di-kick dari game! | [shouting] panik |
| `VO_Gamer_Lemot_2` | Lag, Bang, lag! Restart router-nya dong! | [shouting] |
| `VO_Gamer_Rusak_1` | Bang, PC-nya blue screen! Pas lagi clutch! | frustrasi |
| `VO_Gamer_Joki_1` | Bang, gue joki rank. Diskon dikit lah, nanti satu tim gue bawa ke sini. | merayu |
| `VO_Gamer_Paket_1` | Bang, paket malam. Sampai pagi juga gue jabanin. | percaya diri |
| `VO_Gamer_Ngutang_1` | Bang, transfer belum masuk. Besok gue lunasin, sumpah. | memelas |
| `VO_Gamer_Bayar_1` | Mantap, Bang. Besok gue balik lagi. | puas |
| `VO_Gamer_Marah_Pulang_1` | Ah, gue cabut, Bang. | kesal |

## 6. Bapak-bapak
**Suara:** pria 50 tahun, santai, logat Jawa ringan, pencinta bola.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Bapak_Datang_1` | Mas, main bola sejam. Yang ada PlayStasiun-nya. | santai |
| `VO_Bapak_Pesan_Mi_1` | Mas, kopi item sama mi rebus, ya. | santai |
| `VO_Bapak_Rusak_1` | Lho, lho… stiknya ndak nyambung ini, Mas. | bingung |
| `VO_Bapak_Rage_1` | Wasitnya dibayar! Hadeh, stiknya sampai tak banting, Mas. | [shouting] lalu malu |
| `VO_Bapak_Ngutang_1` | Mas, ngutang dulu ya. Besok tak bayar, sumpah. | membujuk |
| `VO_Bapak_Bayar_1` | Iki, Mas. Matur nuwun. | ramah |
| `VO_Bapak_Marah_Pulang_1` | Wis, wis… mulih wae. | menggerutu |

## 7. Emak
**Suara:** ibu 45 tahun, galak tapi lucu, lantang. Logat Betawi.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_Emak_Datang_1` | Bang! Liat anak saya nggak?! Seragam putih biru! Harusnya masih di sekolah! | [shouting] marah |
| `VO_Emak_Datang_2` | Mana tuh bocah?! PR belum dikerjain, malah main PS! | [shouting] |
| `VO_Emak_Jewer_1` | Ayo pulang! Main PS terus, nilai merah semua! | [shouting] |
| `VO_Emak_Pergi_1` | Awas ya, kalau ketemu di sini, saya laporin Pak RT! | mengancam |

## 8. Pak RT
**Suara:** pria 55 tahun, berwibawa, sopan tapi tegas, bicara pelan.

| File | Kalimat | Arahan |
|---|---|---|
| `VO_PakRT_Datang_1` | Permisi, Mas. Sudah jam sepuluh lewat. Warga pada komplain suaranya sampai gang sebelah. | tenang, tegas |
| `VO_PakRT_Kopi_1` | Wah, kopi… Ya sudah, tapi suaranya dikecilin, ya, Mas. | luluh |
| `VO_PakRT_Kecil_1` | Nah, begitu dong. Terima kasih, Mas. | puas |
| `VO_PakRT_Abai_1` | Mas ini dibilangin… Nanti saya bawa ke rapat warga. | kecewa |

---

**Total: 55 baris** dari 8 karakter. Kalau ingin lebih banyak variasi, tambahkan `_2`, `_3`, dan seterusnya dengan nama yang sama. Game memilih salah satu secara acak, hingga maksimal 3 variasi per situasi.
