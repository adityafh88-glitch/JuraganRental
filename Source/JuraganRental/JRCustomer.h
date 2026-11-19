// Pelanggan: datang, antre, main, bayar di kasir, lalu pulang (atau kabur).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRTypes.h"
#include "JRCustomer.generated.h"

class AJRStation;
class AJRGameMode;
class UAnimSequence;
class USkeletalMeshComponent;
class UTextRenderComponent;
class UCapsuleComponent;

UCLASS()
class JURAGANRENTAL_API AJRCustomer : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRCustomer();

	/** MeshName = nama aset di /Game/Characters, misalnya "SK_Bocil". */
	void Setup(int32 InTypeIndex, const FString& InName, const TCHAR* MeshName, float InScale, EJRUnitType InWant, bool bInFlex);

	virtual void Tick(float DeltaSeconds) override;

	/** Lihat pelanggan lalu tekan E: terima bayaran, atau diteruskan ke unit yang ia pakai. */
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

	/** Jalan ke Loc; setibanya di sana state menjadi Next. */
	void WalkTo(const FVector& Loc, EJRCustomerState Next);

	int32 TypeIndex = 0;
	FString DisplayName;
	EJRUnitType Want = EJRUnitType::PS;
	EJRUnitType Used = EJRUnitType::PS;
	bool bFlex = false;

	EJRCustomerState State = EJRCustomerState::Walk;
	EJRCustomerState NextState = EJRCustomerState::Walk;
	FVector Target = FVector::ZeroVector;
	float Speed = 260.f;

	UPROPERTY()
	TObjectPtr<AJRStation> Station;

	int32 QueueIdx = -1;
	bool bBenchSeat = false;	// antre sambil duduk di bangku
	float WaitMinutes = 0.f;	// total menit game menunggu
	float Patience = 0.f;		// jam game sebelum menyerah
	float HoursLeft = 0.f;
	float Bill = 0.f;
	int32 Breakdowns = 0;		// berapa kali unitnya rusak / mati listrik
	double PayOrder = 0.0;
	bool bOrder = false;		// sedang pesan mi
	float OrderHours = 0.f;		// sisa waktu sebelum pesanan batal
	bool bAte = false;
	int32 MissedOrders = 0;
	float Played = 0.f;			// jam game sudah dimainkan
	float EmakAt = -1.f;		// bocil: jam main saat emak datang (-1 = tidak)
	bool bRaging = false;		// sedang membanting stik
	bool bFined = false;		// sudah ditagih ganti rugi
	float SatAdj = 0.f;			// penyesuaian kepuasan dari kejadian
	bool bLagged = false;		// sudah kena internet lemot
	bool bPaket = false;		// paket malam warnet
	FString Mood;

private:
	bool MoveStep(float DeltaSeconds);
	void Arrive(AJRGameMode* GM);
	void UpdateAnimation();
	void PlayLoop(UAnimSequence* Anim);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;

	/** Hanya untuk trace interaksi (kanal Visibility); tidak menghalangi gerak. */
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> HitBox;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Bubble;

	UPROPERTY()
	TObjectPtr<UAnimSequence> CurrentAnim;
};
