# Naskah Suara — Juragan Rental

Naskah dialog untuk ElevenLabs. Kolom **Teks** sudah siap disalin langsung ke kotak Text to Speech. Tag emosi dalam kurung siku sudah ada di depan kalimat, jadi tidak perlu mengetik apa-apa lagi.

## Cara pakai (singkat)

1. **Voice Design:** buat 1 suara per karakter. Salin **Prompt Voice Design** karakter itu, lalu Generate dan **Save** dengan nama karakternya. Atur Label Language ke **Indonesian**.
2. **Text to Speech:** pilih suara karakter, model **Eleven v3**, lalu **salin isi kolom Teks**, Generate, dan pilih versi terbaik.
3. **Unduh**, ganti nama file sesuai kolom **File** (contoh `VO_Bocil_Datang_1.mp3`), lalu simpan ke `D:\Dev\JuraganRental\ArtSource\Audio\Voice\`. MP3 boleh.
4. Kalau terdengar kaku, turunkan **Stability** ke sekitar 35%.

Baris yang belum dibuat tidak masalah, karena game melewatinya. Jangan meng-clone suara orang sungguhan tanpa izin. ElevenLabs juga tidak mengizinkan suara anak di bawah 18 tahun, jadi karakter Bocil disuarakan pengisi suara dewasa bergaya kartun.

---

## 1. Bocil berseragam
**Prompt Voice Design:** `Young adult male voice actor in his early 20s with a high, bright, cartoonish voice. Cheeky, very energetic and fast-talking, playful animated-show style. Indonesian accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Bocil_Datang_1` | `[excited] Bang, PS kosong? Main sejam ya, Bang!` |
| `VO_Bocil_Datang_2` | `[whispers] Bang, nyalain dong, buruan… nanti ketahuan emak!` |
| `VO_Bocil_Pesan_Mi_1` | `[excited] Bang, mi goreng satu, telornya dua ya!` |
| `VO_Bocil_Rusak_1` | `[shouting] Bang! Stiknya nggak bisa nendang, Bang!` |
| `VO_Bocil_Rusak_2` | `[frustrated] Yaaah, stiknya mati! Lagi seru-serunya!` |
| `VO_Bocil_Bayar_1` | `[sighs] Nih, Bang… uang jajan saya.` |
| `VO_Bocil_Marah_Pulang_1` | `[annoyed] Ah lama, Bang. Pindah ke rental sebelah aja!` |
| `VO_Bocil_Ketahuan_1` | `[scared] Ampun, Mak! Cuma sebentar, Mak!` |

## 2. Anak SMA
**Prompt Voice Design:** `Male in his early 20s, relaxed and casual, slightly lazy delivery, friendly slang-heavy tone. Indonesian Jakarta accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_SMA_Datang_1` | `[casual] Bang, satu jam dulu. Nanti nambah kalau menang.` |
| `VO_SMA_Datang_2` | `[casual] Sore, Bang. TV yang kosong yang mana?` |
| `VO_SMA_Pesan_Mi_1` | `[casual] Bang, mi rebus satu sama es teh, ya.` |
| `VO_SMA_Rusak_1` | `[confused] Bang, ini stiknya jalan sendiri!` |
| `VO_SMA_Rage_1` | `[shouting] AAARGH! Penaltinya nggak masuk!` |
| `VO_SMA_Rage_2` | `[shouting] Curang! Itu offside, Bang! OFFSIDE!` |
| `VO_SMA_Bayar_1` | `[casual] Berapa, Bang? Oke, pas ya.` |
| `VO_SMA_Marah_Pulang_1` | `[annoyed] Udah ah, kelamaan nunggu.` |

## 3. Mahasiswa
**Prompt Voice Design:** `Male, 22 years old, tired and soft-spoken, polite but a little sarcastic, slow delivery. Indonesian accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Mahasiswa_Datang_1` | `[deadpan] Mas, PC-nya ada yang kosong? Mau ngerjain tugas… katanya.` |
| `VO_Mahasiswa_Pesan_Mi_1` | `[sighs] Mas, mi instan satu ya. Belum makan dari pagi.` |
| `VO_Mahasiswa_Rusak_1` | `[desperate] Mas… PC-nya hang. Tugas saya belum ke-save.` |
| `VO_Mahasiswa_Lemot_1` | `[sarcastic] Mas, internetnya lemot banget. Loading-nya kayak skripsi saya.` |
| `VO_Mahasiswa_Bayar_1` | `[polite] Ini, Mas. Makasih.` |
| `VO_Mahasiswa_Marah_Pulang_1` | `[disappointed] Ya udah, Mas. Saya ke warnet sebelah.` |

## 4. Mas-mas kantoran
**Prompt Voice Design:** `Male office worker, about 30, calm low voice, polite, sounds exhausted after work. Indonesian accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Kantoran_Datang_1` | `[sighs] Mas, buat lepas penat. Satu jam, ya.` |
| `VO_Kantoran_Pesan_Mi_1` | `[polite] Mas, mi goreng sama kopi ada?` |
| `VO_Kantoran_Rusak_1` | `[surprised] Waduh, kok mati, Mas?` |
| `VO_Kantoran_Lemot_1` | `[sarcastic] Mas, koneksinya kayak Senin pagi. Macet.` |
| `VO_Kantoran_Ngutang_1` | `[embarrassed] Mas, gajian minggu depan. Catat dulu, ya.` |
| `VO_Kantoran_Ngutang_2` | `[nervous] Aduh, dompet ketinggalan di motor, Mas. Besok saya bayar.` |
| `VO_Kantoran_Bayar_1` | `[friendly] Ini, Mas. Makasih, ya.` |
| `VO_Kantoran_Marah_Pulang_1` | `[annoyed] Lama banget. Saya ada meeting.` |

## 5. Gamer online
**Prompt Voice Design:** `Male gamer, mid 20s, hyped and loud, fast and excitable, uses gaming slang. Indonesian Jakarta accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Gamer_Datang_1` | `[excited] Bang, PC paling kenceng! Gue mau push rank!` |
| `VO_Gamer_Pesan_Mi_1` | `[hurried] Bang, mi goreng anterin ke bilik ya, lagi war!` |
| `VO_Gamer_Lemot_1` | `[shouting] BANG! PING-NYA MERAH! Gue di-kick dari game!` |
| `VO_Gamer_Lemot_2` | `[shouting] Lag, Bang, lag! Restart router-nya dong!` |
| `VO_Gamer_Rusak_1` | `[frustrated] Bang, PC-nya blue screen! Pas lagi clutch!` |
| `VO_Gamer_Joki_1` | `[persuasive] Bang, gue joki rank. Diskon dikit lah, nanti satu tim gue bawa ke sini.` |
| `VO_Gamer_Paket_1` | `[confident] Bang, paket malam. Sampai pagi juga gue jabanin.` |
| `VO_Gamer_Ngutang_1` | `[pleading] Bang, transfer belum masuk. Besok gue lunasin, sumpah.` |
| `VO_Gamer_Bayar_1` | `[satisfied] Mantap, Bang. Besok gue balik lagi.` |
| `VO_Gamer_Marah_Pulang_1` | `[annoyed] Ah, gue cabut, Bang.` |

## 6. Bapak-bapak
**Prompt Voice Design:** `Middle-aged Indonesian man, about 50, warm and easygoing, slightly raspy voice, light Javanese accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Bapak_Datang_1` | `[relaxed] Mas, main bola sejam. Yang ada PlayStasiun-nya.` |
| `VO_Bapak_Pesan_Mi_1` | `[relaxed] Mas, kopi item sama mi rebus, ya.` |
| `VO_Bapak_Rusak_1` | `[confused] Lho, lho… stiknya ndak nyambung ini, Mas.` |
| `VO_Bapak_Rage_1` | `[shouting] Wasitnya dibayar! [embarrassed] Hadeh, stiknya sampai tak banting, Mas.` |
| `VO_Bapak_Ngutang_1` | `[pleading] Mas, ngutang dulu ya. Besok tak bayar, sumpah.` |
| `VO_Bapak_Bayar_1` | `[friendly] Iki, Mas. Matur nuwun.` |
| `VO_Bapak_Marah_Pulang_1` | `[grumbling] Wis, wis… mulih wae.` |

## 7. Emak
**Prompt Voice Design:** `Middle-aged Indonesian woman, about 45, loud, bossy and comically fierce, strong Betawi Jakarta accent.`

| File | Teks (salin langsung) |
|---|---|
| `VO_Emak_Datang_1` | `[shouting] Bang! Liat anak saya nggak?! Seragam putih biru! Harusnya masih di sekolah!` |
| `VO_Emak_Datang_2` | `[angry] Mana tuh bocah?! PR belum dikerjain, malah main PS!` |
| `VO_Emak_Jewer_1` | `[shouting] Ayo pulang! Main PS terus, nilai merah semua!` |
| `VO_Emak_Pergi_1` | `[threatening] Awas ya, kalau ketemu di sini, saya laporin Pak RT!` |

## 8. Pak RT
**Prompt Voice Design:** `Indonesian man in his late 50s, calm, authoritative neighborhood chief, polite but firm, slow and measured.`

| File | Teks (salin langsung) |
|---|---|
| `VO_PakRT_Datang_1` | `[calm] Permisi, Mas. Sudah jam sepuluh lewat. Warga pada komplain, suaranya sampai gang sebelah.` |
| `VO_PakRT_Kopi_1` | `[softening] Wah, kopi… Ya sudah, tapi suaranya dikecilin, ya, Mas.` |
| `VO_PakRT_Kecil_1` | `[pleased] Nah, begitu dong. Terima kasih, Mas.` |
| `VO_PakRT_Abai_1` | `[disappointed] Mas ini dibilangin… Nanti saya bawa ke rapat warga.` |

---

**Total: 55 baris dari 8 karakter.** Kalau ingin variasi, buat file `_2`, `_3` dengan nama situasi yang sama. Game memilih salah satunya secara acak.
