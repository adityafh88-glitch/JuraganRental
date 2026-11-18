#include "JRBlockout.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

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
		Paint(Part, Color);
		return Part;
	}

	UMaterialInstanceDynamic* Paint(UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base || !Part)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Part);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		Part->SetMaterial(0, Mat);
		return Mat;
	}
}
