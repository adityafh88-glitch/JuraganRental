// Kurir: kardus pesanan yang datang pagi hari, dan penanda hijau tempat memasang unit baru.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRDelivery.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;

/** Kardus pesanan di depan pintu. Tekan E untuk mengangkat. */
UCLASS()
class JURAGANRENTAL_API AJRDeliveryBox : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRDeliveryBox();
	void Setup(FName InItemId, const FString& InName);
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

	FName ItemId;
	FString ItemName;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;
};

/** Penanda lantai hijau di slot kosong berikutnya; muncul saat membawa kardus TV atau PC. */
UCLASS()
class JURAGANRENTAL_API AJRSlotMarker : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRSlotMarker();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Plate;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;

	bool bShown = true;
};
