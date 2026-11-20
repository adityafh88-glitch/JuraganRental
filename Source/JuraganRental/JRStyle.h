// Gaya widget bersama: sudut bulat, garis tepi, tab berbentuk pil, efek tekan, teks tebal berbayang.
// Warna tetap khas Juragan Rental (panel gelap, biru, kuning-emas; HP jadul hijau LCD).

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

class UButton;
class UTextBlock;

namespace JRStyle
{
	enum class ETheme : uint8 { Default, Lcd };	// Lcd = HP jadul layar hijau

	enum class EBtn : uint8
	{
		Primary,	// biru (aksi utama) / hijau tua di LCD
		Secondary,	// gelap, teks terang
		Tab,		// pil (tab tidak aktif)
		TabActive,	// pil kuning-emas (tab aktif) / hitam di LCD
		Danger,		// merah (tutup/berbahaya)
		Disabled	// pudar (tidak bisa ditekan)
	};

	// Warna (fungsi, bukan variabel global, agar aman saat inisialisasi)
	JURAGANRENTAL_API FLinearColor Cream();		// latar panel (gelap)
	JURAGANRENTAL_API FLinearColor CreamDeep();	// latar sekunder
	JURAGANRENTAL_API FLinearColor Brown();		// garis tepi panel
	JURAGANRENTAL_API FLinearColor Ink();		// teks terang
	JURAGANRENTAL_API FLinearColor Muted();		// teks keterangan
	JURAGANRENTAL_API FLinearColor Accent();	// kuning-emas judul/aksen
	JURAGANRENTAL_API FLinearColor Good();		// hijau
	JURAGANRENTAL_API FLinearColor FromHex(uint32 Rgb, float Alpha = 1.f);

	JURAGANRENTAL_API FButtonStyle Button(EBtn Kind, float Radius = 14.f, ETheme Theme = ETheme::Default);
	/** Pasang gaya ke tombol; teks di dalamnya (jika TextBlock) ikut diwarnai. */
	JURAGANRENTAL_API void Apply(UButton* B, EBtn Kind, float Radius = 14.f, ETheme Theme = ETheme::Default);
	JURAGANRENTAL_API FLinearColor TextOn(EBtn Kind, ETheme Theme = ETheme::Default);
	JURAGANRENTAL_API void StyleText(UTextBlock* T, EBtn Kind, ETheme Theme = ETheme::Default);

	/** Panel gelap bersudut bulat dengan garis tepi halus. */
	JURAGANRENTAL_API FSlateBrush Panel(float Radius = 20.f, float Outline = 3.f);
}
