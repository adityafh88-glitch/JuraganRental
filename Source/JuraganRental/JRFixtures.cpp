#include "JRFixtures.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "JRBlockout.h"
#include "JRCustomer.h"
#include "JRGameMode.h"

using namespace JRBlockout;

/* ---------- Ruangan garasi (20 x 14 m) ---------- */

AJRRoom::AJRRoom()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AJRRoom::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor Wall = Hex(0xd8cdb4), Wall2 = Hex(0xc9bc9f);

	// Lantai keramik (atas lantai di Z = 1 cm) + nat
	AddPart(this, Root, EShape::Cube, FVector(1400, 2000, 10), FVector(0, 0, -4), Hex(0x8f8270));
	for (int32 i = -6; i <= 6; i += 2)
	{
		AddPart(this, Root, EShape::Cube, FVector(3, 2000, 1), FVector(i * 100.f, 0, 1.5f), Hex(0x7d715f), false);
	}
	for (int32 i = -9; i <= 9; i += 2)
	{
		AddPart(this, Root, EShape::Cube, FVector(1400, 3, 1), FVector(0, i * 100.f, 1.5f), Hex(0x7d715f), false);
	}

	// Dinding 3 m. Belakang = +X, depan (pintu) = -X, kiri = -Y, kanan = +Y.
	AddPart(this, Root, EShape::Cube, FVector(20, 2040, 300), FVector(710, 0, 150), Wall);
	AddPart(this, Root, EShape::Cube, FVector(1400, 20, 300), FVector(0, -1010, 150), Wall2);
	AddPart(this, Root, EShape::Cube, FVector(1400, 20, 300), FVector(0, 1010, 150), Wall2);
	AddPart(this, Root, EShape::Cube, FVector(20, 1330, 300), FVector(-710, -345, 150), Wall2); // depan, kiri pintu
	AddPart(this, Root, EShape::Cube, FVector(20, 490, 300), FVector(-710, 765, 150), Wall2);   // depan, kanan pintu
	AddPart(this, Root, EShape::Cube, FVector(20, 200, 80), FVector(-710, 420, 260), Wall2);    // ambang pintu

	// Plafon (menutup langit; garasi diterangi lampu neon)
	AddPart(this, Root, EShape::Cube, FVector(1440, 2040, 10), FVector(0, 0, 305), Hex(0xe8e4da));

	// Pintu rolling terbuka: kotak gulungan menempel di ambang pintu bagian dalam, dan keset
	AddProp(this, Root, TEXT("SM_Pintu_Rolling"), FVector(-683, 420, 0), 0.f, false);
	AddProp(this, Root, TEXT("SM_Keset"), FVector(-650, 420, 1), 0.f, false);

	// Papan nama di dinding belakang
	AddProp(this, Root, TEXT("SM_Papan_Nama"), FVector(702, -420, 205), 180.f, false);
	UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this);
	Sign->SetupAttachment(Root);
	Sign->SetRelativeLocation(FVector(696, -420, 240));
	Sign->SetRelativeRotation(FRotator(0, 180, 0));
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetWorldSize(22.f);
	Sign->SetTextRenderColor(FColor(255, 194, 75));
	Sign->SetText(FText::FromString(TEXT("RENTAL PS & WARNET JURAGAN")));
	Sign->RegisterComponent();
	AddInstanceComponent(Sign);

	// Bangku antre, rak kaset, poster
	AddProp(this, Root, TEXT("SM_Bangku_Panjang"), FVector(-650, 30, 0));
	AddProp(this, Root, TEXT("SM_Rak_Kaset"), FVector(690, 250, 100), 180.f, false);
	AddProp(this, Root, TEXT("SM_Poster"), FVector(-450, -998, 140), 90.f, false);
	AddProp(this, Root, TEXT("SM_Poster"), FVector(520, -998, 150), 90.f, false);

	// Lampu neon garasi
	for (const FVector& Pos : { FVector(450, -450, 296), FVector(450, 450, 296), FVector(50, -450, 296),
		FVector(50, 450, 296), FVector(-350, -450, 296), FVector(-350, 450, 296) })
	{
		AddProp(this, Root, TEXT("SM_Lampu_Neon"), Pos, 0.f, false);
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(Pos - FVector(0, 0, 10));
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(45.f);
		Light->SetAttenuationRadius(1100.f);
		Light->SetLightColor(FLinearColor(1.f, 0.97f, 0.9f));
		Light->RegisterComponent();
		AddInstanceComponent(Light);
	}
}

/* ---------- Meja kasir ---------- */

AJRKasir::AJRKasir()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AJRKasir::BeginPlay()
{
	Super::BeginPlay();
	// Papan depan meja menghadap pelanggan (-X, arah pintu); pemain berdiri di sisi +X.
	AddProp(this, Root, TEXT("SM_Meja_Kasir"), FVector::ZeroVector, 180.f);
	AddProp(this, Root, TEXT("SM_Laci_Kasir"), FVector(0, -50, 100), 0.f, false);
	AddProp(this, Root, TEXT("SM_Buku_Utang"), FVector(0, 40, 100), 0.f, false);
}

FJRInteraction AJRKasir::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (const AJRCustomer* Payer = GM ? GM->NextPayer() : nullptr)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Terima bayaran %s  (%s)"), *Payer->DisplayName, *JRFormatRp(Payer->Bill));
	}
	return Out;
}

void AJRKasir::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>())
	{
		GM->CollectNextPayer();
	}
}

/* ---------- MCB ---------- */

AJRMcb::AJRMcb()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AJRMcb::BeginPlay()
{
	Super::BeginPlay();
	// Sumbu lokal: +X menghadap ke dalam ruangan.
	UStaticMeshComponent* Box = AddProp(this, Root, TEXT("SM_Kotak_MCB"), FVector::ZeroVector);
	LightMat = MakeColorMat(this, Hex(0x3ccf6e));
	Box->SetMaterialByName(TEXT("M_LED_Green"), LightMat);
}

void AJRMcb::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const bool bTripped = GM && GM->bMcbTripped;
	if (LightMat && bTripped != bLastTripped)
	{
		LightMat->SetVectorParameterValue(TEXT("Color"), bTripped ? Hex(0xff4433) : Hex(0x3ccf6e));
		bLastTripped = bTripped;
	}
}

FJRInteraction AJRMcb::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (GM && GM->bMcbTripped)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = TEXT("Naikkan MCB");
	}
	else
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("MCB menyala");
	}
	return Out;
}

void AJRMcb::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>())
	{
		GM->ResetMcb();
	}
}

/* ---------- Pojok kantin ---------- */

AJRStove::AJRStove()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0, 30, 150));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(20.f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRStove::BeginPlay()
{
	Super::BeginPlay();
	// Sumbu lokal: +X menghadap ke dalam ruangan, +Y ke arah dinding depan (pintu).
	AddProp(this, Root, TEXT("SM_Meja_Kantin"), FVector::ZeroVector, -90.f);
	UStaticMeshComponent* Kompor = AddProp(this, Root, TEXT("SM_Kompor_Listrik"), FVector(0, 30, 90), 0.f, false);
	PlateMat = MakeColorMat(this, Hex(0x2a2c30));
	Kompor->SetMaterialByName(TEXT("M_Plat_Kompor"), PlateMat);
	AddProp(this, Root, TEXT("SM_Panci"), FVector(2, 30, 98), 0.f, false);
	AddProp(this, Root, TEXT("SM_Mangkuk_Mi"), FVector(6, -45, 90), 0.f, false);
	AddProp(this, Root, TEXT("SM_Gelas_EsTeh"), FVector(-19, -15, 90), 0.f, false);
	AddProp(this, Root, TEXT("SM_Kardus_Mi"), FVector(-9, 110, 0), 0.f);
}

void AJRStove::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM) return;
	FString Text;
	FColor Color(255, 194, 75);
	if (GM->StoveState == EJRStoveState::Cooking)
	{
		Text = FString::Printf(TEXT("MASAK %d%%"), FMath::RoundToInt(GM->CookProgress / AJRGameMode::CookSeconds * 100.f));
	}
	else if (GM->StoveState == EJRStoveState::Ready)
	{
		Text = TEXT("MATANG!");
		Color = FColor(92, 201, 138);
	}
	Label->SetText(FText::FromString(Text));
	Label->SetTextRenderColor(Color);
	if (PlateMat)
	{
		const bool bHot = GM->StoveState == EJRStoveState::Cooking && !GM->bMcbTripped;
		PlateMat->SetVectorParameterValue(TEXT("Color"), bHot ? Hex(0xff6a2a) : Hex(0x2a2c30));
	}
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}

FJRInteraction AJRStove::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM) return Out;
	if (GM->bCarryMie)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Antar mi ke pelanggan yang pesan");
	}
	else if (GM->StoveState == EJRStoveState::Idle)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Masak mi instan  (modal %s, kompor 600 VA)"), *JRFormatRp(AJRGameMode::MieCost));
	}
	else if (GM->StoveState == EJRStoveState::Cooking)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = GM->bMcbTripped ? TEXT("Listrik mati, kompor berhenti") : TEXT("Mi sedang dimasak...");
	}
	else
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = TEXT("Angkat mi ke mangkuk");
	}
	return Out;
}

void AJRStove::Interact()
{
	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM || GM->bCarryMie) return;
	if (GM->StoveState == EJRStoveState::Idle) GM->StartCook();
	else if (GM->StoveState == EJRStoveState::Ready) GM->TakeMie();
}
