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
	bool IsFree() const { return Customer == nullptr && !bBroken; }

	EJRUnitType Type = EJRUnitType::PS;
	int32 Index = 0;
	FString DisplayName;
	bool bBroken = false;

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

	FLinearColor LastScreenColor = FLinearColor::Black;
};
