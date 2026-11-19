// Tamu yang datang karena suatu kejadian (emak, Pak RT): berdiri di pintu, lalu pergi.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRVisitor.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

UCLASS()
class JURAGANRENTAL_API AJRVisitor : public AActor
{
	GENERATED_BODY()

public:
	AJRVisitor();

	/** MeshName di /Game/Characters, AnimName di /Game/Characters/Animations. */
	void Setup(const TCHAR* MeshName, const TCHAR* AnimName);

	/** Jalan ke Target lalu menghilang. */
	void LeaveTo(const FVector& Target);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;

	FVector Exit = FVector::ZeroVector;
	bool bLeaving = false;
};
