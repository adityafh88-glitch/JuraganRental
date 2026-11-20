---
name: jr-build
description: Compile kode C++ Juragan Rental (editor Win64) dan membuat paket .exe/ZIP Windows. Pakai setelah mengubah file di Source/, saat diminta "build", "compile", "buat exe", "paket PC", atau sebelum uji otomatis / build Android.
---

# Compile dan paket PC

## Compile kode (setelah mengubah Source/)

1. Unreal Editor **harus tertutup**. Skrip akan menolak bila editor masih berjalan. Jangan matikan editor milik pengguna; minta pengguna menutupnya sendiri.
2. ```powershell
   & 'D:\Dev\JuraganRental\Tools\build_editor.ps1'
   ```
3. Bila error: baca pesan `error C....` pertama. Error yang pernah muncul di proyek ini:
   - variabel bernama sama dengan variabel luar (shadowing, misalnya `M`, `MS`);
   - beberapa `constexpr` dalam satu deklarasi di kelas yang diekspor DLL;
   - `FKey` ambigu;
   - modul `ApplicationCore` belum ditautkan.

## Aturan kode proyek

- `bUseUnity = false`, jadi setiap .cpp harus meng-include apa yang dipakainya sendiri.
- **Jangan memanggil `JRT(...)` di inisialisasi statis/global**, karena build rilis akan crash.
- Aset yang disimpan di variabel `static` wajib `TStrongObjectPtr`, karena raw pointer bisa dihapus GC.
- Teks yang tampil ke pemain wajib dibungkus `JRT(TEXT("..."))` (skill `jr-teks`).
- Gaya UI lewat `JRStyle` (`Apply`, `Panel`, warna khas game). Jangan meniru warna game lain.
- Tanpa emoji di teks game, karena font tidak mendukung.

## Paket PC (.exe + ZIP)

```powershell
& 'D:\Dev\JuraganRental\Tools\build_pc.ps1'
```

Jalankan di background (15–30 menit). Hasilnya `Build\Windows\` dan `Build\JuraganRental_PC_v<versi>.zip`.
