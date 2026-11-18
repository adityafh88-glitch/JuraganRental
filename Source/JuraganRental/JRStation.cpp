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
	ScreenMat = MakeColorMat(this, Hex(0x0d0f12));
	if (Type == EJRUnitType::PS)
	{
		AddProp(this, Root, TEXT("SM_Meja_TV"), FVector(0, 0, 0));
		UStaticMeshComponent* Tv = AddProp(this, Root, TEXT("SM_TV_Tabung"), FVector(-4, 0, 50));
		Tv->SetMaterialByName(TEXT("M_TV_Screen"), ScreenMat);
		AddProp(this, Root, TEXT("SM_Konsol_PlayStasiun"), FVector(0, 16.5f, 28.5f));
		AddProp(this, Root, TEXT("SM_Karpet"), FVector(SeatDistance(), 0, 0), 0.f, false);
		AddProp(this, Root, TEXT("SM_Stik"), FVector(SeatDistance() - 45, 45, 2), 160.f, false);
		Label->SetRelativeLocation(FVector(0, 0, 140));
	}
	else
	{
		AddProp(this, Root, TEXT("SM_Meja_Warnet"), FVector(0, 0, 0));
		UStaticMeshComponent* Crt = AddProp(this, Root, TEXT("SM_Monitor_CRT"), FVector(-8, 0, 75));
		Crt->SetMaterialByName(TEXT("M_TV_Screen"), ScreenMat);
		AddProp(this, Root, TEXT("SM_CPU"), FVector(-5, 30, 0));
		AddProp(this, Root, TEXT("SM_Keyboard"), FVector(18, 0, 75), 0.f, false);
		AddProp(this, Root, TEXT("SM_Mouse"), FVector(16, 30, 75), 0.f, false);
		AddProp(this, Root, TEXT("SM_Kursi_Plastik"), FVector(SeatDistance(), 0, 0), 180.f, false); // menghadap meja
		AddProp(this, Root, TEXT("SM_Sekat_Bilik"), FVector(30, 112, 0));
		Label->SetRelativeLocation(FVector(0, 0, 165));
	}
}

AJRGameMode* AJRStation::GetGM() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr;
}

FVector AJRStation::GetSeatLocation() const
{
	FVector Seat = GetActorLocation() + GetActorForwardVector() * SeatDistance();
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
	else if (Customer && Customer->State == EJRCustomerState::Playing && Customer->bOrder)
	{
		if (GM->bCarryMie)
		{
			Out.Kind = EJRInteractKind::Press;
			Out.Prompt = FString::Printf(TEXT("Antar mi ke %s  (+%s)"), *Customer->DisplayName, *JRFormatRp(AJRGameMode::MiePrice));
		}
		else
		{
			Out.Kind = EJRInteractKind::Info;
			Out.Prompt = TEXT("Pelanggan pesan mi. Masak di kompor pojok kantin");
		}
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
	else if (Customer && Customer->bOrder && GM->bCarryMie)
	{
		GM->DeliverMie(this);
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
		Text = FString::Printf(TEXT("%d:%02d%s"), Minutes / 60, Minutes % 60, Customer->bOrder ? TEXT("  + MI") : TEXT(""));
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
