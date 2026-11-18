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
enum class EJRGamePhase : uint8
{
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
