#include "JRDelivery.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "JRBlockout.h"
#include "JRGameMode.h"

using namespace JRBlockout;

namespace
{
	void FaceCamera(UTextRenderComponent* Text)
	{
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(Text, 0))
		{
			const FVector ToCam = Cam->GetCameraLocation() - Text->GetComponentLocation();
			Text->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
		}
	}

	UTextRenderComponent* MakeLabel(UObject* Outer, const TCHAR* Name)
	{
		UTextRenderComponent* T = Cast<AActor>(Outer)->CreateDefaultSubobject<UTextRenderComponent>(Name);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetWorldSize(18.f);
		T->SetTextRenderColor(FColor(255, 194, 75));
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return T;
	}
}

/* ---------- Kardus pesanan ---------- */

AJRDeliveryBox::AJRDeliveryBox()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	Label = MakeLabel(this, TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0, 0, 70));
}

void AJRDeliveryBox::Setup(FName InItemId, const FString& InName)
{
	ItemId = InItemId;
	ItemName = InName;
	UStaticMeshComponent* Box = AddProp(this, Root, TEXT("SM_Kardus_Mi"), FVector::ZeroVector, 0.f);
	Box->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.6f));
	Label->SetText(FText::FromString(InName));
}

void AJRDeliveryBox::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FaceCamera(Label);
}

FJRInteraction AJRDeliveryBox::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM) return Out;
	if (GM->IsCarrying())
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Tangan penuh. Selesaikan dulu yang sedang dibawa");
	}
	else
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Angkat kardus: %s"), *ItemName);
	}
	return Out;
}

void AJRDeliveryBox::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->PickBox(this);
}

/* ---------- Penanda tempat pasang ---------- */

AJRSlotMarker::AJRSlotMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	Label = MakeLabel(this, TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(60, 0, 120));
	Label->SetWorldSize(24.f);
	Label->SetTextRenderColor(FColor(92, 230, 140));
	Label->SetText(FText::FromString(TEXT("PASANG DI SINI")));
}

void AJRSlotMarker::BeginPlay()
{
	Super::BeginPlay();
	// Pelat hijau di lantai, cukup tinggi agar mudah dibidik.
	Plate = AddPart(this, Root, EShape::Cube, FVector(120, 190, 6), FVector(40, 0, 3), Hex(0x3ccf6e), false);
	Plate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Plate->SetCollisionResponseToAllChannels(ECR_Ignore);
	Plate->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	bShown = false;
}

void AJRSlotMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	FVector Loc = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;
	const bool bShow = GM && GM->GetInstallSlot(Loc, Rot);
	if (bShow)
	{
		SetActorLocationAndRotation(Loc, Rot);
		FaceCamera(Label);
	}
	if (bShow != bShown)
	{
		SetActorHiddenInGame(!bShow);
		SetActorEnableCollision(bShow);
		bShown = bShow;
	}
}

FJRInteraction AJRSlotMarker::GetInteraction() const
{
	FJRInteraction Out;
	if (bShown)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = TEXT("Pasang unit di sini");
	}
	return Out;
}

void AJRSlotMarker::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->InstallBox();
}
