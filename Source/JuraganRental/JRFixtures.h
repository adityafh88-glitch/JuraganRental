// Perabot tetap di toko: ruangan, meja kasir, dan kotak MCB.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRFixtures.generated.h"

class UMaterialInstanceDynamic;

/** Lantai, dinding, papan nama, bangku antre, dan lampu garasi. */
UCLASS()
class JURAGANRENTAL_API AJRRoom : public AActor
{
	GENERATED_BODY()

public:
	AJRRoom();
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
};

/** Meja kasir: tempat pelanggan membayar. */
UCLASS()
class JURAGANRENTAL_API AJRKasir : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRKasir();
	virtual void BeginPlay() override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
};

/** Kotak MCB di dinding. Turun saat daya melebihi kapasitas. */
UCLASS()
class JURAGANRENTAL_API AJRMcb : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRMcb();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LightMat;

	bool bLastTripped = false;
};
