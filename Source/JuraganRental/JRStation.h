// Satu unit rental: TV + PlayStasiun atau PC warnet.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRTypes.h"
#include "JRStation.generated.h"

class AJRCustomer;
class AJRGameMode;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

UCLASS()
class JURAGANRENTAL_API AJRStation : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRStation();

	/** Bangun geometri unit. Dipanggil sekali setelah spawn. */
	void Setup(EJRUnitType InType, int32 InIndex);

	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

	FVector GetSeatLocation() const;
	FRotator GetSeatRotation() const;
	bool IsFree() const { return Customer == nullptr && !bBroken && !bPowerOff; }
	/** Dimatikan pemain lewat billing (hemat daya): tidak memakan listrik, jam main pelanggan berhenti. */
	bool bPowerOff = false;

	/** Jarak tempat duduk pelanggan dari unit (karpet lesehan / kursi warnet), cm. */
	float SeatDistance() const { return Type == EJRUnitType::PS ? 170.f : Type == EJRUnitType::VIP ? 190.f : 80.f; }

	EJRUnitType Type = EJRUnitType::PS;
	int32 Index = 0;
	FString DisplayName;
	bool bBroken = false;
	bool bVirus = false;	// PC kena virus: perbaikan lebih lama
	bool bHaunted = false;	// cerita misteri: layar menyala sendiri
	FString HauntText;		// teks di layar saat bHaunted (kosong = "LANJUTKAN?")
	bool bUsed = false;		// barang bekas dari loak: lebih sering rusak
	void MarkUsed() { bUsed = true; DisplayName += TEXT(" (bekas)"); }

	/** Tingkat tempat duduk: 0 standar, 1 empuk, 2 gaming. */
	void ApplySeatTier(int32 Tier);

	/** Level upgrade unit (1-5): layar datar, lampu LED berwarna, tarif dan kepuasan naik. */
	int32 UpLevel = 1;
	void ApplyUpLevel(int32 Level);

	UPROPERTY()
	TObjectPtr<AJRCustomer> Customer;

	static constexpr float FixSeconds = 1.6f;

private:
	AJRGameMode* GetGM() const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ScreenMat;

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> Seat;

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> SeatExtra;	// bantal (PS) atau sandaran tinggi (PC)

	int32 SeatTier = -1;

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> OldScreen;	// TV tabung (PS) atau monitor CRT (PC)

	UPROPERTY()
	TArray<TObjectPtr<class UStaticMeshComponent>> FlatParts;	// TV/monitor layar datar hasil upgrade

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> Led;	// lis lampu LED penanda level

	float ActivityTimer = 2.f;	// detik sampai bunyi aktivitas berikutnya
	void PlayActivitySound();

	FLinearColor LastScreenColor = FLinearColor::Black;
};
