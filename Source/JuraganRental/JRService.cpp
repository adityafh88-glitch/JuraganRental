#include "JRService.h"
#include "JRLang.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "JRBlockout.h"
#include "JRGameMode.h"

using namespace JRBlockout;

AJRService::AJRService()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(17.f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRService::Setup(EJRService InKind)
{
	Kind = InKind;
	switch (Kind)
	{
	case EJRService::Kulkas:
	{
		// Kulkas pajang pintu kaca biru (beda dari kulkas kantin), berisi botol warna-warni.
		AddPart(this, Root, EShape::Cube, FVector(60, 72, 180), FVector(0, 0, 90), Hex(0x1f6fd1));
		AddPart(this, Root, EShape::Cube, FVector(2, 62, 118), FVector(31, 0, 96), Hex(0xbfe6ff), false);
		AddPart(this, Root, EShape::Cube, FVector(3, 70, 26), FVector(31, 0, 166), Hex(0xf4f1e8), false);
		static const uint32 Bottle[] = { 0xf39c12, 0x27ae60, 0x8e5a2b, 0xe74c3c, 0x2e86de };
		for (int32 Row = 0; Row < 3; ++Row)
		{
			AddPart(this, Root, EShape::Cube, FVector(40, 60, 2), FVector(10, 0, 50 + Row * 36), Hex(0xd8dde3), false);	// rak
			for (int32 i = 0; i < 5; ++i)
			{
				AddPart(this, Root, EShape::Cylinder, FVector(8, 8, 22), FVector(22, -24 + i * 12, 62 + Row * 36), Hex(Bottle[(i + Row) % 5]), false);
			}
		}
		Label->SetRelativeLocation(FVector(10, 0, 205));
		break;
	}
	case EJRService::Konter:
	{
		// Etalase konter pulsa kuning, papan biru di belakang, beberapa HP di atas kaca.
		AddPart(this, Root, EShape::Cube, FVector(60, 120, 95), FVector(0, 0, 47.5f), Hex(0xf2c230));
		AddPart(this, Root, EShape::Cube, FVector(64, 124, 4), FVector(0, 0, 97), Hex(0x2b2f36), false);
		AddPart(this, Root, EShape::Cube, FVector(2, 110, 26), FVector(31, 0, 66), Hex(0xd63031), false);
		AddPart(this, Root, EShape::Cube, FVector(4, 120, 70), FVector(-34, 0, 150), Hex(0x1f5fbf), false);
		for (int32 i = 0; i < 4; ++i)
		{
			AddPart(this, Root, EShape::Cube, FVector(10, 6, 14), FVector(5, -40 + i * 26, 106), Hex(i % 2 ? 0x1b1b1b : 0x5d6d7e), false);
		}
		Label->SetRelativeLocation(FVector(0, 0, 205));
		break;
	}
	default:
	{
		// Meja printer: mesin cetak abu-abu dan tumpukan kertas.
		AddPart(this, Root, EShape::Cube, FVector(60, 110, 75), FVector(0, 0, 37.5f), Hex(0x8a6a45));
		AddPart(this, Root, EShape::Cube, FVector(42, 52, 26), FVector(-2, -18, 88), Hex(0xd9d9d9), false);
		AddPart(this, Root, EShape::Cube, FVector(18, 34, 2), FVector(24, -18, 80), Hex(0xbfbfbf), false);
		AddPart(this, Root, EShape::Cube, FVector(26, 22, 10), FVector(0, 34, 80), Hex(0xffffff), false);
		AddPart(this, Root, EShape::Cube, FVector(6, 30, 3), FVector(18, -18, 100), Hex(0x2ecc71), false);	// panel tombol
		Label->SetRelativeLocation(FVector(0, 0, 175));
		break;
	}
	}
}

FJRInteraction AJRService::GetInteraction() const
{
	const AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
	return GM ? GM->ServiceInteraction(Kind) : FJRInteraction();
}

void AJRService::Interact()
{
	if (AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr) GM->ServiceInteract(Kind);
}

void AJRService::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM) return;
	bool bAlert = false;
	Label->SetText(FText::FromString(GM->ServiceLabel(Kind, bAlert)));
	Label->SetTextRenderColor(bAlert ? FColor(255, 160, 60) : FColor(255, 214, 120));
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}
