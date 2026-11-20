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

	/** Jalan mengikuti titik-titik (boleh beda lantai), lalu diam di titik terakhir menghadap FinalYaw. */
	void WalkPath(const TArray<FVector>& Points, float FinalYaw);
	bool IsWalking() const { return Path.Num() > 0; }
	/** Kecepatan jalan WalkPath (cm/detik). Karyawan sedikit lebih gesit dari tamu. */
	float WalkSpeed = 260.f;

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;

	FVector Exit = FVector::ZeroVector;
	bool bLeaving = false;
	TArray<FVector> Path;
	FVector SegStart = FVector::ZeroVector;
	float EndYaw = 0.f;
	void PlayAnimNamed(const TCHAR* Name);
};
