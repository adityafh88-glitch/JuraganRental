// Pelanggan: datang, antre, main, bayar di kasir, lalu pulang (atau kabur).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRTypes.h"
#include "JRCustomer.generated.h"

class AJRStation;
class AJRGameMode;
class UTextRenderComponent;

UCLASS()
class JURAGANRENTAL_API AJRCustomer : public AActor
{
	GENERATED_BODY()

public:
	AJRCustomer();

	void Setup(int32 InTypeIndex, const FString& InName, const FLinearColor& Top, const FLinearColor& Bottom,
		float InScale, EJRUnitType InWant, bool bInFlex);

	virtual void Tick(float DeltaSeconds) override;

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
	float WaitMinutes = 0.f;	// total menit game menunggu
	float Patience = 0.f;		// jam game sebelum menyerah
	float HoursLeft = 0.f;
	float Bill = 0.f;
	int32 Breakdowns = 0;		// berapa kali unitnya rusak / mati listrik
	double PayOrder = 0.0;
	FString Mood;

private:
	bool MoveStep(float DeltaSeconds);
	void Arrive(AJRGameMode* GM);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Bubble;
};
