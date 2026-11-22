#include "JRBlockout.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace JRBlockout
{
	static UStaticMesh* LoadShape(EShape Shape)
	{
		const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
		if (Shape == EShape::Cylinder) Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		if (Shape == EShape::Sphere) Path = TEXT("/Engine/BasicShapes/Sphere.Sphere");
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	FLinearColor Hex(uint32 Rgb)
	{
		return FLinearColor(FColor((Rgb >> 16) & 0xFF, (Rgb >> 8) & 0xFF, Rgb & 0xFF));
	}

	UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, EShape Shape,
		const FVector& Size, const FVector& Center, const FLinearColor& Color, bool bCollide)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->SetStaticMesh(LoadShape(Shape));
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Center);
		Part->SetRelativeScale3D(Size / 100.f); // bentuk dasar engine berukuran 100 cm
		Part->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Part->RegisterComponent();
		Owner->AddInstanceComponent(Part);
		Part->SetMaterial(0, SharedColorMat(Color));	// material bersama per warna: engine bisa menggambar balok sewarna sekaligus
		return Part;
	}

	UMaterialInterface* SharedColorMat(const FLinearColor& Color)
	{
		// Aset di variabel static wajib TStrongObjectPtr (aturan proyek: raw pointer bisa dihapus GC).
		static const bool bNoShare = FParse::Param(FCommandLine::Get(), TEXT("jrnoshare"));	// uji performa: cara lama, satu material per balok
		if (bNoShare) return MakeColorMat(GetTransientPackage(), Color);
		static TMap<uint32, TStrongObjectPtr<UMaterialInstanceDynamic>> Cache;
		const uint32 Key = Color.ToFColor(false).DWColor();
		if (TStrongObjectPtr<UMaterialInstanceDynamic>* Hit = Cache.Find(Key)) return Hit->Get();
		UMaterialInstanceDynamic* Mat = MakeColorMat(GetTransientPackage(), Color);
		if (Mat) Cache.Add(Key, TStrongObjectPtr<UMaterialInstanceDynamic>(Mat));
		return Mat;
	}

	UMaterialInstanceDynamic* MakeColorMat(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Outer);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		return Mat;
	}

	UMaterialInstanceDynamic* Paint(UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (!Part) return nullptr;
		UMaterialInstanceDynamic* Mat = MakeColorMat(Part, Color);
		if (Mat) Part->SetMaterial(0, Mat);
		return Mat;
	}

	UStaticMeshComponent* AddProp(AActor* Owner, USceneComponent* Parent, const TCHAR* Name,
		const FVector& Loc, float FaceYaw, bool bCollide)
	{
		// SM_PF_* = model Polyfork (polyfork.dev, gratis untuk komersial) di /Game/Polyfork
		const FString Path = FString(Name).StartsWith(TEXT("SM_PF_")) ? FString::Printf(TEXT("/Game/Polyfork/%s.%s"), Name, Name) : FString::Printf(TEXT("/Game/Props/%s.%s"), Name, Name);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("JuraganRental: model %s tidak ditemukan"), *Path);
		}
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->SetStaticMesh(Mesh);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Loc);
		// Model dari Blender menghadap +Y; putar -90 agar menghadap +X actor.
		Part->SetRelativeRotation(FRotator(0.f, FaceYaw - 90.f, 0.f));
		Part->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Part->RegisterComponent();
		Owner->AddInstanceComponent(Part);
		return Part;
	}
}
