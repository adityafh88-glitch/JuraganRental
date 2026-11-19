// Tipe data bersama Juragan Rental.

#pragma once

#include "CoreMinimal.h"
#include "JRTypes.generated.h"

UENUM(BlueprintType)
enum class EJRUnitType : uint8
{
	PS,
	PC
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
/** Karyawan yang bisa direkrut di ruko. */
enum class EJRStaff : uint8
{
	Kasir,
	Teknisi,
	Koki,
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
