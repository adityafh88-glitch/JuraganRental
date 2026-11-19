#include "JRStation.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "JRBlockout.h"
#include "JRCustomer.h"
#include "JRSound.h"
#include "Components/AudioComponent.h"
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
		UStaticMeshComponent* Monitor = AddProp(this, Root, TEXT("SM_Monitor_CRT"), FVector(-8, 0, 75));
		Monitor->SetMaterialByName(TEXT("M_TV_Screen"), ScreenMat);
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

void AJRStation::PlayActivitySound()
{
	const FVector Screen = GetActorLocation() + FVector(0, 0, 110);
	const FVector Hands = GetSeatLocation() + FVector(0, 0, 70);
	const float R = FMath::FRand();
	if (Type == EJRUnitType::PS)
	{
		if (R < 0.55f)
		{
			static const TCHAR* Klik[] = { TEXT("SFX_Stik_Klik_1"), TEXT("SFX_Stik_Klik_2"), TEXT("SFX_Stik_Klik_3") };
			JRSound::PlayNear(this, Klik[FMath::RandRange(0, 2)], Hands, 0.85f);
		}
		else if (R < 0.72f) JRSound::PlayNear(this, TEXT("SFX_TV_Tendang"), Screen, 0.75f);
		else if (R < 0.90f) JRSound::PlayNear(this, FMath::RandBool() ? TEXT("SFX_TV_Bip_1") : TEXT("SFX_TV_Bip_2"), Screen, 0.6f);
		else JRSound::PlayNear(this, TEXT("SFX_TV_Gol"), Screen, 0.7f);
	}
	else
	{
		if (R < 0.75f)
		{
			static const TCHAR* Ketik[] = { TEXT("SFX_Keyboard_1"), TEXT("SFX_Keyboard_2"), TEXT("SFX_Keyboard_3") };
			JRSound::PlayNear(this, Ketik[FMath::RandRange(0, 2)], GetActorLocation() + GetActorForwardVector() * 20.f + FVector(0, 0, 80), 0.85f);
		}
		else JRSound::PlayNear(this, TEXT("SFX_Mouse_Klik"), GetActorLocation() + FVector(0, 30, 80), 0.75f);
	}
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
	if (bBroken && bVirus)
	{
		Out.Kind = EJRInteractKind::Hold;
		Out.HoldTime = FixSeconds * 2.5f;
		Out.Prompt = FString::Printf(TEXT("Tahan E: install ulang %s (virus)"), *DisplayName);
	}
	else if (bBroken)
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
	else if (GM->CanOfferMie(this))
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("%s mi ke %s  (+%s)"), Customer->bOrder ? TEXT("Antar") : TEXT("Tawarkan"),
			*Customer->DisplayName, *JRFormatRp(AJRGameMode::MiePrice));
	}
	else if (Customer && Customer->State == EJRCustomerState::Playing && Customer->bOrder)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Pelanggan pesan mi. Masak di kompor pojok kantin");
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
		bVirus = false;
		GM->AddToast(FString::Printf(TEXT("%s beres!"), *DisplayName), FLinearColor(0.36f, 0.79f, 0.54f));
	}
	else if (Customer && Customer->State == EJRCustomerState::WaitStart)
	{
		GM->StartSession(this);
	}
	else if (GM->CanOfferMie(this))
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
		ScreenColor = bVirus ? Hex(0x6a2a8a) : Hex(0x8a2a24);
		Text = DisplayName + (bVirus ? TEXT(" VIRUS") : TEXT(" RUSAK"));
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

	// Bunyi aktivitas pendek dan acak selama unit dipakai (bukan musik terus-menerus).
	if (bPlaying && !GM->bMcbTripped && !bBroken && GM->IsPlaying())
	{
		ActivityTimer -= DeltaSeconds;
		if (ActivityTimer <= 0.f)
		{
			PlayActivitySound();
			ActivityTimer = Type == EJRUnitType::PS ? FMath::FRandRange(2.5f, 6.f) : FMath::FRandRange(2.f, 5.f);
		}
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
