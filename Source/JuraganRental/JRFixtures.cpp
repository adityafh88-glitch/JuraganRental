#include "JRFixtures.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "JRBlockout.h"
#include "JRCustomer.h"
#include "JRGameMode.h"
#include "JRSound.h"
#include "Components/AudioComponent.h"

using namespace JRBlockout;

/* ---------- Ruangan (garasi 20 x 14 m, ruko 24 x 16 m) ---------- */

AJRRoom::AJRRoom()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Unlit(TEXT("/Engine/EngineMaterials/UnlitText.UnlitText"));
	SignMaterial = Unlit.Object;
}

/* ---------- Dekorasi ---------- */

AJRDecor::AJRDecor()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Cube = CubeF.Object;
	Cylinder = CylF.Object;
	Cone = ConeF.Object;
}

UStaticMeshComponent* AJRDecor::Shape(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, float Glow)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetupAttachment(Root);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocation(Loc);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetMaterial(0, MakeColorMat(this, Color)); // warna jenuh (dikali Glow malah jadi putih)
	C->RegisterComponent();
	return C;
}

void AJRDecor::Setup(FName Id, float HX, float HY, float DoorY)
{
	// Ukuran bentuk dasar mesin: kubus/silinder 100 cm (poros di tengah), kerucut 100 cm.
	if (Id == TEXT("poster"))
	{
		AddProp(this, Root, TEXT("SM_Poster"), FVector(HX - 2.f, -0.18f * HY, 175.f), 180.f, false);
		AddProp(this, Root, TEXT("SM_Poster"), FVector(HX - 2.f, 0.08f * HY, 175.f), 180.f, false);
	}
	else if (Id == TEXT("kipas"))
	{
		AddProp(this, Root, TEXT("SM_Kipas_Berdiri"), FVector(-(HX - 140.f), -0.3f * HY, 0.f), 0.f, false);
		AddProp(this, Root, TEXT("SM_Kipas_Berdiri"), FVector(0.55f * HX, 0.f, 0.f), 180.f, false);
	}
	else if (Id == TEXT("tanaman"))
	{
		for (const FVector& P : { FVector(-(HX - 45.f), DoorY - 110.f, 0.f), FVector(-(HX - 45.f), DoorY + 110.f, 0.f), FVector(HX - 45.f, HY - 45.f, 0.f) })
		{
			Shape(Cylinder, P + FVector(0, 0, 22), FVector(0.36f, 0.36f, 0.44f), FLinearColor(0.45f, 0.22f, 0.12f));	// pot tanah liat
			Shape(Cone, P + FVector(0, 0, 85), FVector(0.55f, 0.55f, 0.95f), FLinearColor(0.12f, 0.45f, 0.18f));	// daun
		}
	}
	else if (Id == TEXT("neon"))
	{
		// Lis lampu RGB di dinding belakang, di atas deretan TV
		Shape(Cube, FVector(HX - 3.f, -0.5f * HY, 250.f), FVector(0.04f, 0.9f * HY / 100.f, 0.06f), FLinearColor(0.75f, 0.2f, 1.f), 6.f);
		Shape(Cube, FVector(HX - 3.f, -0.5f * HY, 238.f), FVector(0.04f, 0.9f * HY / 100.f, 0.03f), FLinearColor(0.1f, 0.85f, 1.f), 6.f);
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(Root);
		L->SetRelativeLocation(FVector(HX - 60.f, -0.5f * HY, 230.f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(18.f);
		L->SetAttenuationRadius(700.f);
		L->SetLightColor(FLinearColor(0.7f, 0.35f, 1.f));
		L->RegisterComponent();
	}
	else if (Id == TEXT("ac"))
	{
		// AC split di dinding kiri, tinggi
		Shape(Cube, FVector(0.1f * HX, -(HY - 14.f), 245.f), FVector(0.9f, 0.24f, 0.3f), FLinearColor(0.92f, 0.93f, 0.95f));
		Shape(Cube, FVector(0.1f * HX, -(HY - 27.f), 232.f), FVector(0.8f, 0.02f, 0.04f), FLinearColor(0.35f, 0.38f, 0.42f));
		Shape(Cube, FVector(0.1f * HX + 35.f, -(HY - 27.f), 252.f), FVector(0.03f, 0.01f, 0.015f), FLinearColor(0.2f, 1.f, 0.4f), 8.f); // lampu indikator
	}
}

void AJRRoom::SetLightsOn(bool bOn)
{
	for (UPointLightComponent* L : Lights) if (L) L->SetVisibility(bOn);
}

void AJRRoom::Build(float HX, float HY, float DoorY, bool bRuko)
{
	const FLinearColor Wall = bRuko ? Hex(0xcfd8d4) : Hex(0xd8cdb4);
	const FLinearColor Wall2 = bRuko ? Hex(0xbac6c2) : Hex(0xc9bc9f);
	const FLinearColor Floor = bRuko ? Hex(0xb3ada0) : Hex(0x8f8270);
	const FLinearColor Grout = bRuko ? Hex(0x9f998c) : Hex(0x7d715f);

	// Lantai keramik (atas lantai di Z = 1 cm) + nat tiap 2 m
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 2 * HY, 10), FVector(0, 0, -4), Floor);
	for (float X = -HX + 100.f; X < HX; X += 200.f)
	{
		AddPart(this, Root, EShape::Cube, FVector(3, 2 * HY, 1), FVector(X, 0, 1.5f), Grout, false);
	}
	for (float Y = -HY + 100.f; Y < HY; Y += 200.f)
	{
		AddPart(this, Root, EShape::Cube, FVector(2 * HX, 3, 1), FVector(0, Y, 1.5f), Grout, false);
	}

	// Dinding 3 m. Belakang = +X, depan (pintu) = -X, kiri = -Y, kanan = +Y.
	AddPart(this, Root, EShape::Cube, FVector(20, 2 * HY + 40, 300), FVector(HX + 10, 0, 150), Wall);
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 20, 300), FVector(0, -(HY + 10), 150), Wall2);
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 20, 300), FVector(0, HY + 10, 150), Wall2);
	const float L0 = -(HY + 10), L1 = DoorY - 100.f, R0 = DoorY + 100.f, R1 = HY + 10;
	AddPart(this, Root, EShape::Cube, FVector(20, L1 - L0, 300), FVector(-(HX + 10), (L0 + L1) / 2, 150), Wall2); // depan, kiri pintu
	AddPart(this, Root, EShape::Cube, FVector(20, R1 - R0, 300), FVector(-(HX + 10), (R0 + R1) / 2, 150), Wall2); // depan, kanan pintu
	AddPart(this, Root, EShape::Cube, FVector(20, 200, 80), FVector(-(HX + 10), DoorY, 260), Wall2);             // ambang pintu

	// Plafon
	AddPart(this, Root, EShape::Cube, FVector(2 * HX + 40, 2 * HY + 40, 10), FVector(0, 0, 305), Hex(0xe8e4da));

	// Pintu rolling terbuka dan keset
	AddProp(this, Root, TEXT("SM_Pintu_Rolling"), FVector(-(HX - 17), DoorY, 0), 0.f, false);
	AddProp(this, Root, TEXT("SM_Keset"), FVector(-(HX - 50), DoorY, 1), 0.f, false);

	// Papan nama di dinding belakang
	const float SignY = -0.42f * HY;
	AddProp(this, Root, TEXT("SM_Papan_Nama"), FVector(HX + 2, SignY, 205), 180.f, false);
	UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this);
	Sign->SetupAttachment(Root);
	Sign->SetRelativeLocation(FVector(HX - 16, SignY, 240)); // cukup jauh dari papan agar tidak tenggelam (z-fighting di HP)
	Sign->SetRelativeRotation(FRotator(0, 180, 0));
	Sign->SetHorizontalAlignment(EHTA_Center);
	Sign->SetVerticalAlignment(EVRTA_TextCenter);
	Sign->SetWorldSize(19.f);
	Sign->SetTextRenderColor(FColor(45, 8, 4)); // hampir hitam di atas papan terang: kontras tinggi
	if (SignMaterial)
	{
		Sign->SetTextMaterial(SignMaterial); // tanpa cahaya: warna tetap sama di ruangan terang maupun gelap
	}
	Sign->SetText(FText::FromString(bRuko ? TEXT("GAME CENTER JURAGAN") : TEXT("RENTAL PS & WARNET JURAGAN")));
	Sign->RegisterComponent();
	AddInstanceComponent(Sign);

	// Bangku antre, rak kaset, poster
	AddProp(this, Root, TEXT("SM_Bangku_Panjang"), FVector(-(HX - 50), 30, 0));
	AddProp(this, Root, TEXT("SM_Rak_Kaset"), FVector(HX - 10, 0.25f * HY, 100), 180.f, false);
	AddProp(this, Root, TEXT("SM_Poster"), FVector(-0.64f * HX, -(HY - 2), 140), 90.f, false);
	AddProp(this, Root, TEXT("SM_Poster"), FVector(0.74f * HX, -(HY - 2), 150), 90.f, false);
	if (bRuko)
	{
		AddProp(this, Root, TEXT("SM_Kulkas_Minuman"), FVector(40, -(HY - 35), 0), 90.f); // kulkas minuman dekat kantin
		AddProp(this, Root, TEXT("SM_Poster"), FVector(0.2f * HX, HY - 2, 150), -90.f, false);
	}

	// Lampu neon
	const float Xs[] = { -0.5f * HX, 0.07f * HX, 0.64f * HX };
	for (float X : Xs)
	{
		for (float Y : { -0.45f * HY, 0.45f * HY })
		{
			const FVector Pos(X, Y, 296);
			AddProp(this, Root, TEXT("SM_Lampu_Neon"), Pos, 0.f, false);
			UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
			Light->SetupAttachment(Root);
			Light->SetRelativeLocation(Pos - FVector(0, 0, 10));
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->SetIntensity(bRuko ? 55.f : 45.f);
			Light->SetAttenuationRadius(bRuko ? 1300.f : 1100.f);
			Light->SetLightColor(FLinearColor(1.f, 0.97f, 0.9f));
			Light->RegisterComponent();
			Lights.Add(Light);
			AddInstanceComponent(Light);
		}
	}
}

/* ---------- Meja kasir ---------- */

AJRKasir::AJRKasir()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void AJRKasir::BeginPlay()
{
	Super::BeginPlay();
	// Papan depan meja menghadap pelanggan (-X, arah pintu); pemain berdiri di sisi +X.
	AddProp(this, Root, TEXT("SM_Meja_Kasir"), FVector::ZeroVector, 180.f);
	AddProp(this, Root, TEXT("SM_Laci_Kasir"), FVector(0, -50, 100), 0.f, false);
	AddProp(this, Root, TEXT("SM_Buku_Utang"), FVector(0, 40, 100), 0.f, false);
	if (const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) ShowGoldCassette(GM->bGoldCassette);
}

void AJRKasir::ShowGoldCassette(bool bShow)
{
	if (bShow && !GoldCassette && CubeMesh)
	{
		GoldCassette = NewObject<UStaticMeshComponent>(this);
		GoldCassette->SetupAttachment(Root);
		GoldCassette->SetStaticMesh(CubeMesh);
		GoldCassette->SetRelativeLocation(FVector(10, 62, 104));
		GoldCassette->SetRelativeRotation(FRotator(0, 20, 0));
		GoldCassette->SetRelativeScale3D(FVector(0.10f, 0.16f, 0.03f));	// kubus 100 cm -> kaset 10x16x3 cm
		GoldCassette->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GoldCassette->SetMaterial(0, MakeColorMat(this, FLinearColor(1.f, 0.72f, 0.12f)));
		GoldCassette->RegisterComponent();
	}
	if (GoldCassette) GoldCassette->SetVisibility(bShow);
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
	Sizzle = JRSound::MakeLoop(Root, TEXT("LOOP_Masak"), 0.6f);
	if (Sizzle) Sizzle->SetRelativeLocation(FVector(0, 30, 100));
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
	if (Sizzle)
	{
		const bool bCook = GM->StoveState == EJRStoveState::Cooking && !GM->bMcbTripped;
		if (bCook && !Sizzle->IsPlaying()) Sizzle->Play();
		else if (!bCook && Sizzle->IsPlaying()) Sizzle->Stop();
	}
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
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = TEXT("Taruh mi lagi di kompor  (antar/tawarkan ke pelanggan yang main)");
	}
	else if (GM->IsCarrying())
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Tangan penuh. Pasang dulu kardusnya");
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
	if (!GM) return;
	if (GM->bCarryMie) { GM->PutBackMie(); return; }
	if (GM->IsCarrying()) return;
	if (GM->StoveState == EJRStoveState::Idle) GM->StartCook();
	else if (GM->StoveState == EJRStoveState::Ready) GM->TakeMie();
}

/* ---------- Router (Era 2) ---------- */

AJRRouter::AJRRouter()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(10, 0, 55));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(18.f);
	Label->SetTextRenderColor(FColor(239, 107, 91));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRRouter::BeginPlay()
{
	Super::BeginPlay();
	// Rak kecil di dinding + router. Sumbu lokal +X menghadap ke dalam ruangan.
	AddPart(this, Root, EShape::Cube, FVector(40, 80, 5), FVector(10, 0, -8), Hex(0x6b4a2f));
	UStaticMeshComponent* R = AddProp(this, Root, TEXT("SM_Router"), FVector(10, 0, -5), 0.f);
	LightMat = MakeColorMat(this, Hex(0x3ccf6e));
	R->SetMaterialByName(TEXT("M_LED_Green"), LightMat);
}

void AJRRouter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const bool bSlow = GM && GM->bNetSlow;
	const bool bBlink = FMath::FloorToInt(GetWorld()->GetTimeSeconds() * 2.5f) % 2 == 0;
	if (LightMat) LightMat->SetVectorParameterValue(TEXT("Color"), bSlow ? (bBlink ? Hex(0xff4433) : Hex(0x331111)) : (bBlink ? Hex(0x3ccf6e) : Hex(0x1d6a38)));
	Label->SetText(FText::FromString(bSlow ? TEXT("INTERNET LEMOT") : TEXT("")));
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}

FJRInteraction AJRRouter::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (GM && GM->bNetSlow)
	{
		Out.Kind = EJRInteractKind::Hold;
		Out.HoldTime = 2.f;
		Out.Prompt = TEXT("Tahan E: restart router");
	}
	else
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = TEXT("Internet lancar");
	}
	return Out;
}

void AJRRouter::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->RestartRouter();
}
