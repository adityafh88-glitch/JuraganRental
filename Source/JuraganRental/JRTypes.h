// Tipe data bersama Juragan Rental.

#pragma once

#include "CoreMinimal.h"
#include "JRTypes.generated.h"

UENUM(BlueprintType)
enum class EJRUnitType : uint8
{
	PS,
	PC,
	VIP		// booth VIP lantai 2 (Era 3): sofa + TV besar + PlayStasiun 3
};

UENUM(BlueprintType)
enum class EJRInteractKind : uint8
{
	None,
	Info,	// hanya teks petunjuk
	Press,	// tekan E sekali
	Hold	// tahan E selama HoldTime
};

UENUM(BlueprintType)
enum class EJRCustomerState : uint8
{
	Walk,
	Queue,
	WaitStart,
	Playing,
	WaitPay,
	Gone
};

UENUM(BlueprintType)
enum class EJRStoveState : uint8
{
	Idle,
	Cooking,
	Ready
};

UENUM(BlueprintType)
/** Misi harian/mingguan. */
enum class EJRMission : uint8
{
	Serve,		// layani N pelanggan sampai bayar
	Mie,		// jual N mangkuk mi
	Income,		// pemasukan minimal Rp N
	Start,		// nyalakan N sesi
	Repair,		// perbaiki N unit
	NoRun,		// tidak ada yang kabur (dinilai saat tutup)
	Happy,		// kepuasan rata-rata minimal N (dinilai saat tutup)
	Upsell,		// "Bang, nambah sejam!" dilayani N kali
	Count
};

struct FJRMission
{
	EJRMission Kind = EJRMission::Serve;
	int32 Target = 0;
	int32 Progress = 0;
	int32 Reward = 0;
	bool bDone = false;
	bool bWeekly = false;
};

/** Karyawan yang bisa direkrut di ruko. */
enum class EJRStaff : uint8
{
	Kasir,
	Teknisi,
	Koki,
	Cleaner,	// OB: memungut sampah
	Count
};

/** Kejadian penting hari ini: bahan Kartu Momen dan komentar warganet (Viralin). */
enum class EJRMoment : uint8
{
	Emak, RageQuit, Fight, RunAway, Trip, PakRt, Upsell, Streamer, Lurah, Rival, Tour, LevelUp, Count
};

/** Satu postingan warganet di aplikasi Viralin (HP dalam game). */
struct FJRPost
{
	FString Author;
	FString Text;
	int32 Likes = 0;
	int32 Day = 0;
};

/** Bisnis sampingan Game Center (Era 3). */
enum class EJRService : uint8
{
	Kulkas,		// kulkas minuman: dibeli pelanggan saat bayar
	Konter,		// konter pulsa & top-up game: pembeli datang khusus
	Printer,	// printer & fotokopi: anak sekolah/mahasiswa mencetak tugas
	Count
};

enum class EJRGamePhase : uint8
{
	Menu,		// menu utama (dunia sudah dibangun sebagai latar)
	Intro,
	Playing,
	Report
};

USTRUCT(BlueprintType)
struct FJRInteraction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EJRInteractKind Kind = EJRInteractKind::None;

	UPROPERTY(BlueprintReadOnly)
	FString Prompt;

	UPROPERTY(BlueprintReadOnly)
	float HoldTime = 0.f;
};

/** Satu barang di aplikasi Toko Grosir (HP jadul). */
struct FJRShopItem
{
	FName Id;
	FString Name;
	FString Desc;
	int32 Cost = 0;
	FString Lock; // kosong = bisa dibeli; selain itu alasan terkunci
};

/** Kejadian yang meminta pemain memilih (emak datang, rage quit, ngutang, ...). Permainan berhenti sampai dipilih. */
struct FJRChoice
{
	FString Title;
	FString Body;
	TArray<FString> Labels;
	TArray<TFunction<void()>> Actions;
	int32 Serial = 0;	// nomor unik, agar HUD tahu dialog sudah berganti
};

/** Format rupiah: 1500000 -> "Rp 1.500.000". */
inline FString JRFormatRp(double Value)
{
	const int64 N = FMath::RoundToInt64(FMath::Abs(Value));
	const FString Digits = FString::Printf(TEXT("%lld"), N);
	FString Out;
	int32 Count = 0;
	for (int32 i = Digits.Len() - 1; i >= 0; --i)
	{
		Out.InsertAt(0, Digits[i]);
		if (++Count % 3 == 0 && i > 0)
		{
			Out.InsertAt(0, TEXT('.'));
		}
	}
	return FString(Value < 0 ? TEXT("-Rp ") : TEXT("Rp ")) + Out;
}
