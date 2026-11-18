#include "JRStation.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "JRBlockout.h"
#include "JRCustomer.h"
#include "JRGameMode.h"

using namespace JRBlockout;

AJRStation::AJRStation()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(22.f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRStation::Setup(EJRUnitType InType, int32 InIndex)
{
	Type = InType;
	Index = InIndex;
	DisplayName = FString::Printf(TEXT("%s %d"), Type == EJRUnitType::PS ? TEXT("TV") : TEXT("PC"), Index + 1);

	// Sumbu lokal: +X menghadap ke dalam ruangan (ke arah pelanggan).
	if (Type == EJRUnitType::PS)
	{
		AddPart(this, Root, EShape::Cube, FVector(90, 190, 70), FVector(0, 0, 35), Hex(0x6b4a2f));          // meja
		AddPart(this, Root, EShape::Cube, FVector(75, 130, 95), FVector(-5, 0, 118), Hex(0x2a2a2a));         // TV tabung
		UStaticMeshComponent* Screen = AddPart(this, Root, EShape::Cube, FVector(2, 105, 72), FVector(34, 0, 120), Hex(0x0d0f12), false);
		ScreenMat = Cast<UMaterialInstanceDynamic>(Screen->GetMaterial(0));
		AddPart(this, Root, EShape::Cube, FVector(34, 50, 10), FVector(18, 62, 75), Hex(0x3a4150));          // konsol
		AddPart(this, Root, EShape::Cube, FVector(130, 170, 4), FVector(190, 0, 2), Hex(0x2f5f86), false);   // karpet lesehan
		Label->SetRelativeLocation(FVector(0, 0, 200));
	}
	else
	{
		AddPart(this, Root, EShape::Cube, FVector(90, 180, 75), FVector(0, 0, 37.5f), Hex(0x59606a));        // meja
		AddPart(this, Root, EShape::Cube, FVector(8, 95, 62), FVector(32, 0, 108), Hex(0x1f1f1f));           // monitor
		UStaticMeshComponent* Screen = AddPart(this, Root, EShape::Cube, FVector(2, 82, 50), FVector(37, 0, 108), Hex(0x0d0f12), false);
		ScreenMat = Cast<UMaterialInstanceDynamic>(Screen->GetMaterial(0));
		AddPart(this, Root, EShape::Cube, FVector(42, 24, 60), FVector(-10, 66, 105), Hex(0x2e3238));        // CPU
		AddPart(this, Root, EShape::Cube, FVector(62, 62, 48), FVector(135, 0, 24), Hex(0x7a2e2e), false);   // kursi
		AddPart(this, Root, EShape::Cube, FVector(110, 6, 130), FVector(20, 125, 65), Hex(0x8b9098));        // sekat bilik
		Label->SetRelativeLocation(FVector(40, 0, 185));
	}
}

AJRGameMode* AJRStation::GetGM() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
}

FVector AJRStation::GetSeatLocation() const
{
	FVector Seat = GetActorLocation() + GetActorForwardVector() * (Type == EJRUnitType::PS ? 190.f : 135.f);
	Seat.Z = 0.f;
	return Seat;
}

FRotator AJRStation::GetSeatRotation() const
{
	return FRotator(0.f, GetActorRotation().Yaw + 180.f, 0.f);
}

FJRInteraction AJRStation::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetGM();
	if (!GM)
	{
		return Out;
	}
	if (bBroken)
	{
		Out.Kind = EJRInteractKind::Hold;
		Out.HoldTime = FixSeconds;
		Out.Prompt = Type == EJRUnitType::PS
			? FString::Printf(TEXT("Tahan E: perbaiki stik %s"), *DisplayName)
			: FString::Printf(TEXT("Tahan E: restart %s"), *DisplayName);
	}
	else if (Customer && Customer->State == EJRCustomerState::Playing && GM->bMcbTripped)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Listrik mati. Naikkan MCB di dinding kiri");
	}
	else if (Customer && Customer->State == EJRCustomerState::WaitStart)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Nyalakan %s  (%s/jam)"), *DisplayName, *JRFormatRp(GM->PriceFor(Type)));
	}
	return Out;
}

void AJRStation::Interact()
{
	AJRGameMode* GM = GetGM();
	if (!GM)
	{
		return;
	}
	if (bBroken)
	{
		bBroken = false;
		GM->AddToast(FString::Printf(TEXT("%s beres!"), *DisplayName), FLinearColor(0.36f, 0.79f, 0.54f));
	}
	else if (Customer && Customer->State == EJRCustomerState::WaitStart)
	{
		GM->StartSession(this);
	}
}

void AJRStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetGM();
	if (!GM)
	{
		return;
	}

	const bool bPlaying = Customer && Customer->State == EJRCustomerState::Playing;
	FLinearColor ScreenColor = Hex(0x0d0f12);
	FString Text = DisplayName;
	FColor TextColor(160, 165, 175);

	if (bBroken)
	{
		ScreenColor = Hex(0x8a2a24);
		Text = DisplayName + TEXT(" RUSAK");
		TextColor = FColor(239, 107, 91);
	}
	else if (bPlaying && GM->bMcbTripped)
	{
		Text = TEXT("MATI LISTRIK");
		TextColor = FColor(239, 107, 91);
	}
	else if (Customer && Customer->State == EJRCustomerState::WaitStart)
	{
		ScreenColor = Hex(0x1d2440);
		Text = TEXT("NYALAKAN");
		TextColor = FColor(95, 179, 255);
	}
	else if (bPlaying)
	{
		const bool bBlink = FMath::FloorToInt(GetWorld()->GetTimeSeconds() * 2.5f) % 2 == 0;
		ScreenColor = Type == EJRUnitType::PS ? (bBlink ? Hex(0x3a7bd5) : Hex(0x2f9e6a)) : Hex(0x6fb3ff);
		const int32 Minutes = FMath::Max(0, FMath::CeilToInt(Customer->HoursLeft * 60.f));
		Text = FString::Printf(TEXT("%d:%02d"), Minutes / 60, Minutes % 60);
		TextColor = FColor(255, 194, 75);
	}

	if (ScreenMat && !ScreenColor.Equals(LastScreenColor))
	{
		ScreenMat->SetVectorParameterValue(TEXT("Color"), ScreenColor);
		LastScreenColor = ScreenColor;
	}
	Label->SetText(FText::FromString(Text));
	Label->SetTextRenderColor(TextColor);

	// Label selalu menghadap kamera pemain.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}
