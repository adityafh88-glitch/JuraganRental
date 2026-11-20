---
name: jr-teks
description: Menambah atau mengubah teks yang tampil di game Juragan Rental beserta terjemahan Inggrisnya (JRT, strings_en.tsv, tabel bahasa). Pakai setiap kali menulis teks UI, notifikasi, dialog, atau komentar warganet baru.
---

# Teks baru dan terjemahan

Bahasa utama game adalah Indonesia, dengan terjemahan Inggris yang diambil dari `Tools/strings_en.tsv`.

## Langkah

1. Di kode, bungkus teks: `JRT(TEXT("Teks Indonesia"))`. Untuk teks berformat pakai `JRPrintf(JRT(TEXT("Hari ke-%d")), Hari)`.
   - Jangan memanggil `JRT` di inisialisasi statis/global.
   - Tanpa emoji.
2. Jalankan `python Tools/wrap_strings.py` untuk membungkus otomatis teks yang terlewat.
   - Kata yang tidak boleh diterjemahkan (nama rental, Surface, Landscape, dan sejenisnya) ditaruh di `SKIP_EXACT` dalam skrip itu.
3. Tambahkan baris ke `Tools/strings_en.tsv`: `teks_indonesia<TAB>english`.
   - Teks Indonesia harus **sama persis** dengan yang di kode, termasuk `%d`/`%s` dan `\"`.
   - Tulis lewat skrip Python dalam mode byte (`open(p,'rb')` / `'wb'`), dengan akhir baris `\r\n`, dan lewati baris yang sudah ada. Jangan pakai heredoc bash, karena escape-nya sering rusak.
4. Jalankan `python Tools/make_lang_table.py` untuk membuat ulang tabel bahasa.
5. Compile (skill `jr-build`), lalu cek dengan `-jrenglish` (skill `jr-uji`) bahwa teksnya tampil dalam bahasa Inggris.

## Gaya bahasa

Bahasa santai khas rental PS ("Bang, nambah sejam!", "wkwk" boleh di komentar warganet). Jangan memakai nama merek sungguhan: konsolnya bernama "PlayStasiun".
