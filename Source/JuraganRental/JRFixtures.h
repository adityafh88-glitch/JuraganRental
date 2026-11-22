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
	void Build(const struct FJRLayout& L, int32 Era);

private:
	/** Tampak depan dari jalan: papan nama, kanopi, jendela, spanduk, lampu. */
	void BuildFacade(const struct FJRLayout& L, int32 Era, float WallH);
	class UTextRenderComponent* OutsideText(const FString& Text, const FVector& Loc, float Size, const FColor& Color);
	/** Barang pengisi ruangan dan halaman depan (model Polyfork). */
	void BuildClutter(const struct FJRLayout& L, int32 Era);
	/** Koreksi arah hadap model Polyfork (glTF) terhadap konvensi AddProp. */
	static constexpr float PfYaw = 0.f;

public:

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

/** Sampah yang ditinggalkan pelanggan: mangkuk mi, gelas, atau bungkus. Dipungut pemain atau OB. */
UCLASS()
class JURAGANRENTAL_API AJRTrash : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRTrash();
	void Setup(int32 InKind);
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;
	int32 Kind = 0;	// 0 mangkuk, 1 gelas, 2 bungkus

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
};

/** Komputer billing di samping meja kasir: menyalakan semua unit dari satu tempat. */
UCLASS()
class JURAGANRENTAL_API AJRBillingPC : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRBillingPC();
	virtual void BeginPlay() override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
};

/** Panah hijau melayang di atas sasaran tutorial. */
UCLASS()
class JURAGANRENTAL_API AJRTutorialArrow : public AActor
{
	GENERATED_BODY()

public:
	AJRTutorialArrow();
	virtual void Tick(float DeltaSeconds) override;
	/** Tampilkan di atas Target (cm, titik lantai/benda); bShow=false menyembunyikan. */
	void Point(bool bShow, const FVector& Target);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Cone;

	FVector Base = FVector::ZeroVector;
	float Time = 0.f;
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
	UStaticMeshComponent* Shape(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, float Glow = 0.f, const FRotator& Rot = FRotator::ZeroRotator);
	void BuildStreet(float HX, float HY, float DoorY, bool bRuko);
	/** Latar kampung di samping, belakang, dan cakrawala (pengganti bukit gurun bawaan level). */
	void BuildKampung(float HX, float HY);
	void Tree(const FVector& At, float Size, int32 Kind);
	void House(const FVector& FrontCenter, float Width, float Height, const FLinearColor& Wall, int32 Seed, const TCHAR* Sign = nullptr);
	void Motor(const FVector& Loc, const FLinearColor& Body);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cylinder;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Cone;

	UPROPERTY()
	TObjectPtr<UStaticMesh> Sphere;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> TextMat;
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
