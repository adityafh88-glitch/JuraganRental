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

	/** Bangun ruangan: setengah panjang (X) dan lebar (Y) dalam cm, posisi pintu di dinding depan. */
	void Build(float HalfX, float HalfY, float DoorY, bool bRuko);

	/** Material teks tanpa cahaya untuk papan nama (direferensikan di sini agar ikut dikemas). */
	UPROPERTY()
	TObjectPtr<class UMaterialInterface> SignMaterial;

	/** Nyalakan/matikan semua lampu neon (untuk efek berkedip di cerita misteri). */
	void SetLightsOn(bool bOn);

	UPROPERTY()
	TArray<TObjectPtr<class UPointLightComponent>> Lights;

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
	/** Hadiah tamat cerita misteri: kaset emas dipajang di meja. */
	void ShowGoldCassette(bool bShow);

	UPROPERTY()
	TObjectPtr<class UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> GoldCassette;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
};

/** Dekorasi yang dibeli lewat HP: poster, kipas, tanaman, neon RGB, AC. */
UCLASS()
class JURAGANRENTAL_API AJRDecor : public AActor
{
	GENERATED_BODY()

public:
	AJRDecor();
	/** Bangun dekorasi Id di posisi yang sesuai ukuran ruangan. */
	void Setup(FName Id, float HalfX, float HalfY, float DoorY);

private:
	UStaticMeshComponent* Shape(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, float Glow = 0.f);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cylinder;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cone;
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

/** Pojok kantin: meja, kompor listrik, panci. Masak mi di sini lalu antar ke pelanggan. */
UCLASS()
class JURAGANRENTAL_API AJRStove : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRStove();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<class UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PlateMat;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> Sizzle;
};

/** Router internet di dinding belakang ruko. Saat internet lemot: tahan E untuk restart. */
UCLASS()
class JURAGANRENTAL_API AJRRouter : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRRouter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<class UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LightMat;
};
