#include "JRFixtures.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	// Lantai (atas lantai di Z = 1 cm) + garis keramik
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
	AddPart(this, Root, EShape::Cube, FVector(90, 180, 2), FVector(-650, 420, 2), Hex(0x7a3b2e), false); // keset

	// Papan nama di dinding belakang
	AddPart(this, Root, EShape::Cube, FVector(4, 520, 100), FVector(698, -420, 240), Hex(0x1d3f8a), false);
	UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this);
	Sign->SetupAttachment(Root);
	Sign->SetRelativeLocation(FVector(694, -420, 240));
	Sign->SetRelativeRotation(FRotator(0, 180, 0));
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetWorldSize(42.f);
	Sign->SetTextRenderColor(FColor(255, 194, 75));
	Sign->SetText(FText::FromString(TEXT("RENTAL PS & WARNET JURAGAN")));
	Sign->RegisterComponent();
	AddInstanceComponent(Sign);

	// Bangku antre dekat pintu
	AddPart(this, Root, EShape::Cube, FVector(55, 440, 45), FVector(-650, 30, 22.5f), Hex(0x6b4a2f));

	// Lampu neon garasi
	for (const FVector& Pos : { FVector(300, -400, 285), FVector(300, 400, 285), FVector(-300, -400, 285), FVector(-300, 400, 285) })
	{
		AddPart(this, Root, EShape::Cube, FVector(20, 120, 4), Pos + FVector(0, 0, 8), Hex(0xf5f5f0), false);
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(Pos);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(40.f);
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
	AddPart(this, Root, EShape::Cube, FVector(70, 260, 100), FVector(0, 0, 50), Hex(0x6b4a2f));
	AddPart(this, Root, EShape::Cube, FVector(42, 55, 32), FVector(0, -80, 116), Hex(0x3d3d3d));   // mesin kasir
	AddPart(this, Root, EShape::Cube, FVector(32, 45, 3), FVector(0, 40, 101.5f), Hex(0x2f8a4f)); // buku utang
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
	AddPart(this, Root, EShape::Cube, FVector(12, 50, 70), FVector::ZeroVector, Hex(0x9aa0a6));
	UStaticMeshComponent* Light = AddPart(this, Root, EShape::Cube, FVector(5, 12, 12), FVector(8, 0, 25), Hex(0x3ccf6e), false);
	LightMat = Cast<UMaterialInstanceDynamic>(Light->GetMaterial(0));
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
