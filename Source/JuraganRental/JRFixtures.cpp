#include "JRFixtures.h"
#include "JRLang.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
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
#include "JRHUD.h"
#include "JRStation.h"
#include "JRCustomer.h"
#include "GameFramework/PlayerController.h"
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

/* ---------- Sampah ---------- */

AJRTrash::AJRTrash()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AJRTrash::Setup(int32 InKind)
{
	Kind = InKind;
	if (Kind == 0) AddProp(this, Root, TEXT("SM_Mangkuk_Mi"), FVector::ZeroVector, FMath::FRandRange(0.f, 360.f));
	else if (Kind == 1)
	{
		if (UStaticMeshComponent* G = AddProp(this, Root, TEXT("SM_Gelas_EsTeh"), FVector::ZeroVector, 0.f))
		{
			G->SetRelativeRotation(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 90.f));	// gelas tergeletak
			G->SetRelativeLocation(FVector(0, 0, 4));
		}
	}
	else
	{
		AddPart(this, Root, EShape::Sphere, FVector(14, 12, 8), FVector(0, 0, 4), Hex(FMath::RandBool() ? 0xe8e2c8 : 0xd94a3a));	// bungkus kusut
	}
}

FJRInteraction AJRTrash::GetInteraction() const
{
	FJRInteraction Out;
	Out.Kind = EJRInteractKind::Press;
	Out.Prompt = JRT(TEXT("Pungut sampah"));
	return Out;
}

void AJRTrash::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->PickTrash(this);
}

/* ---------- Komputer billing ---------- */

AJRBillingPC::AJRBillingPC()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AJRBillingPC::BeginPlay()
{
	Super::BeginPlay();
	// Meja kecil + monitor menghadap pemain (sisi +X, sama seperti laci kasir)
	// Sama seperti bilik PC warnet: layar menghadap +X (sisi pemain di belakang kasir)
	AddProp(this, Root, TEXT("SM_Meja_Warnet"), FVector(0, 0, 0));
	UStaticMeshComponent* Mon = AddProp(this, Root, TEXT("SM_Monitor_CRT"), FVector(-8, 0, 75));
	if (Mon) Mon->SetMaterialByName(TEXT("M_TV_Screen"), MakeColorMat(this, Hex(0x1f6fd6)));
	AddProp(this, Root, TEXT("SM_Keyboard"), FVector(18, 0, 75), 0.f, false);
	AddProp(this, Root, TEXT("SM_CPU"), FVector(-5, 30, 0), 0.f, false);
}

FJRInteraction AJRBillingPC::GetInteraction() const
{
	FJRInteraction Out;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	int32 Waiting = 0;
	if (GM) for (const AJRStation* S : GM->GetStations()) if (S && S->Customer && S->Customer->State == EJRCustomerState::WaitStart) ++Waiting;
	Out.Kind = EJRInteractKind::Press;
	Out.Prompt = Waiting > 0 ? JRPrintf(JRT(TEXT("Buka billing  (%d unit menunggu)")), Waiting) : FString(JRT(TEXT("Buka komputer billing")));
	return Out;
}

void AJRBillingPC::Interact()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr) Hud->SetBillingOpen(true);
}

/* ---------- Panah tutorial ---------- */

AJRTutorialArrow::AJRTutorialArrow()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	Cone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cone"));
	Cone->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Cone->SetStaticMesh(ConeF.Object);
	Cone->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));	// ujung kerucut menunjuk ke bawah
	Cone->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.5f));
	Cone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cone->CastShadow = false;
}

void AJRTutorialArrow::Point(bool bShow, const FVector& Target)
{
	if (!Cone->GetMaterial(0) || !Cone->GetMaterial(0)->IsA<UMaterialInstanceDynamic>())
	{
		Cone->SetMaterial(0, MakeColorMat(this, FLinearColor(0.2f, 1.f, 0.35f)));
	}
	SetActorHiddenInGame(!bShow);
	Base = Target + FVector(0.f, 0.f, 220.f);
	SetActorLocation(Base);
}

void AJRTutorialArrow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsHidden()) return;
	Time += DeltaSeconds;
	SetActorLocation(Base + FVector(0.f, 0.f, 18.f * FMath::Sin(Time * 4.f)));
	Cone->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextF(TEXT("/Engine/EngineMaterials/UnlitText.UnlitText"));
	Cube = CubeF.Object;
	Cylinder = CylF.Object;
	Cone = ConeF.Object;
	Sphere = SphereF.Object;
	TextMat = TextF.Object;
}

UStaticMeshComponent* AJRDecor::Shape(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, float Glow, const FRotator& Rot)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetupAttachment(Root);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocation(Loc);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetMaterial(0, SharedColorMat(Color)); // warna jenuh (dikali Glow malah jadi putih); material bersama per warna
	C->RegisterComponent();
	return C;
}

/* ---------- Pemandangan luar: jalan kampung ---------- */

void AJRDecor::Motor(const FVector& L, const FLinearColor& Body)
{
	// Motor bebek menghadap jalan (sumbu -X)
	const FLinearColor Tire(0.06f, 0.06f, 0.07f), Metal(0.55f, 0.57f, 0.6f);
	for (float Dx : { -48.f, 48.f })
	{
		Shape(Cylinder, L + FVector(Dx, 0, 28), FVector(0.56f, 0.56f, 0.1f), Tire, 0.f, FRotator(0, 0, 90));
	}
	Shape(Cube, L + FVector(0, 0, 52), FVector(0.95f, 0.26f, 0.22f), Body);			// bodi
	Shape(Cube, L + FVector(12, 0, 72), FVector(0.48f, 0.26f, 0.1f), FLinearColor(0.08f, 0.08f, 0.09f)); // jok
	Shape(Cube, L + FVector(-48, 0, 85), FVector(0.06f, 0.06f, 0.55f), Metal);		// setang tegak
	Shape(Cube, L + FVector(-48, 0, 112), FVector(0.05f, 0.6f, 0.05f), Metal);		// setang
	Shape(Sphere, L + FVector(-58, 0, 95), FVector(0.14f, 0.14f, 0.14f), FLinearColor(1.f, 0.95f, 0.75f)); // lampu depan
}

void AJRDecor::House(const FVector& F, float Width, float Height, const FLinearColor& Wall, int32 Seed, const TCHAR* Sign)
{
	// F = tengah sisi depan (menghadap +X, ke arah jalan), di lantai
	const float Depth = 500.f;
	// Badan rumah padat: pemain tidak bisa menembus rumah tetangga
	Shape(Cube, F + FVector(-Depth / 2, 0, Height / 2), FVector(Depth / 100.f, Width / 100.f, Height / 100.f), Wall)->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// Atap genteng: lempeng miring di depan + penutup atas
	const FLinearColor Roof = Seed % 2 ? FLinearColor(0.55f, 0.2f, 0.12f) : FLinearColor(0.35f, 0.18f, 0.12f);
	Shape(Cube, F + FVector(25, 0, Height + 18), FVector(1.4f, (Width + 60.f) / 100.f, 0.12f), Roof, 0.f, FRotator(-18.f, 0, 0));
	Shape(Cube, F + FVector(-Depth / 2, 0, Height + 8), FVector((Depth + 20.f) / 100.f, (Width + 40.f) / 100.f, 0.16f), Roof * 0.85f);
	// Pintu dan jendela
	Shape(Cube, F + FVector(2, -Width * 0.22f, 105), FVector(0.04f, 0.9f, 2.1f), FLinearColor(0.32f, 0.2f, 0.12f));
	Shape(Cube, F + FVector(2, Width * 0.18f, 150), FVector(0.04f, 1.2f, 0.9f), FLinearColor(0.55f, 0.75f, 0.85f));
	if (Sign)
	{
		Shape(Cube, F + FVector(8, 0, Height - 40), FVector(0.06f, Width / 120.f, 0.5f), FLinearColor(0.95f, 0.85f, 0.2f));
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
		T->SetupAttachment(Root);
		T->SetRelativeLocation(F + FVector(13, 0, Height - 40));
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldSize(30.f);
		T->SetTextRenderColor(FColor(150, 30, 20));
		if (TextMat) T->SetTextMaterial(TextMat);
		T->SetText(FText::FromString(Sign));
		T->RegisterComponent();
	}
}

void AJRDecor::BuildStreet(float HX, float HY, float DoorY, bool bRuko)
{
	const float X0 = -HX;					// dinding depan (pintu)
	const float Len = 2.f * HY + 4000.f;	// panjang jalan
	const FLinearColor Concrete(0.62f, 0.6f, 0.56f), Asphalt(0.17f, 0.17f, 0.19f), Grass(0.3f, 0.48f, 0.22f);
	// Trotoar, jalan aspal + marka, trotoar seberang, rumput
	Shape(Cube, FVector(X0 - 130.f, 0, 1.f), FVector(2.6f, Len / 100.f, 0.1f), Concrete);	// trotoar (sedikit lebih tinggi)
	Shape(Cube, FVector(X0 - 660.f, 0, -3.f), FVector(8.f, Len / 100.f, 0.1f), Asphalt);	// di atas lantai bawaan level
	for (float Y = -Len / 2 + 200.f; Y < Len / 2; Y += 450.f)
	{
		Shape(Cube, FVector(X0 - 660.f, Y, 2.6f), FVector(0.12f, 2.2f, 0.02f), FLinearColor(0.9f, 0.9f, 0.85f));
	}
	Shape(Cube, FVector(X0 - 1170.f, 0, 1.f), FVector(2.2f, Len / 100.f, 0.1f), Concrete);
	Shape(Cube, FVector(X0 - 2400.f, 0, -3.f), FVector(24.f, Len / 100.f, 0.1f), Grass);

	// Deretan rumah dan warung di seberang jalan
	static const FLinearColor Walls[] = {
		FLinearColor(0.93f, 0.88f, 0.72f), FLinearColor(0.72f, 0.86f, 0.74f), FLinearColor(0.7f, 0.8f, 0.92f),
		FLinearColor(0.95f, 0.76f, 0.62f), FLinearColor(0.88f, 0.88f, 0.88f), FLinearColor(0.93f, 0.8f, 0.85f) };
	int32 K = 0;
	for (float Y = -HY - 900.f; Y < HY + 900.f; Y += 720.f, ++K)
	{
		const TCHAR* Sign = K == 2 ? JRT(TEXT("WARUNG BU SRI")) : K == 5 ? JRT(TEXT("SERVIS HP")) : nullptr;
		House(FVector(X0 - 1300.f, Y, 0), 600.f, 300.f + (K * 53 % 3) * 60.f, Walls[K % 6], K, Sign);
	}

	// Motor parkir dekat pintu (tidak menghalangi jalur masuk pelanggan)
	Motor(FVector(X0 - 140.f, DoorY + 210.f, 0), FLinearColor(0.8f, 0.12f, 0.12f));
	Motor(FVector(X0 - 140.f, DoorY + 300.f, 0), FLinearColor(0.15f, 0.3f, 0.75f));
	if (bRuko) Motor(FVector(X0 - 140.f, DoorY + 390.f, 0), FLinearColor(0.1f, 0.1f, 0.1f));

	// Tiang lampu jalan
	const FVector Pole(X0 - 250.f, DoorY - 380.f, 0);
	Shape(Cylinder, Pole + FVector(0, 0, 230), FVector(0.12f, 0.12f, 4.6f), FLinearColor(0.3f, 0.32f, 0.35f));
	Shape(Cube, Pole + FVector(-70, 0, 455), FVector(1.4f, 0.08f, 0.08f), FLinearColor(0.3f, 0.32f, 0.35f));
	Shape(Cube, Pole + FVector(-135, 0, 445), FVector(0.4f, 0.22f, 0.1f), FLinearColor(1.f, 0.92f, 0.7f));
	UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
	Lamp->SetupAttachment(Root);
	Lamp->SetRelativeLocation(Pole + FVector(-135, 0, 420));
	Lamp->SetIntensityUnits(ELightUnits::Candelas);
	Lamp->SetIntensity(60.f);
	Lamp->SetAttenuationRadius(900.f);
	Lamp->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
	Lamp->SetCastShadows(false);	// lampu ruangan tanpa bayangan: jauh lebih ringan di HP
	Lamp->RegisterComponent();

	// Pohon di trotoar seberang
	for (float Y : { -HY + 200.f, 300.f, HY + 300.f })
	{
		const FVector T(X0 - 1170.f, Y, 0);
		Shape(Cylinder, T + FVector(0, 0, 110), FVector(0.22f, 0.22f, 2.2f), FLinearColor(0.38f, 0.25f, 0.15f));
		Shape(Sphere, T + FVector(0, 0, 290), FVector(2.f, 2.f, 1.7f), FLinearColor(0.2f, 0.45f, 0.18f));
	}
}

void AJRDecor::Tree(const FVector& At, float Size, int32 Kind)
{
	const FLinearColor Trunk(0.38f, 0.25f, 0.15f);
	if (Kind == 1)
	{
		// Pohon kelapa: batang tinggi ramping, daun berupa beberapa lempeng miring
		Shape(Cylinder, At + FVector(0, 0, 330 * Size), FVector(0.16f, 0.16f, 6.6f * Size), FLinearColor(0.48f, 0.38f, 0.26f));
		for (int32 i = 0; i < 5; ++i)
		{
			Shape(Cube, At + FVector(0, 0, 655 * Size), FVector(2.4f * Size, 0.35f, 0.05f), FLinearColor(0.22f, 0.5f, 0.2f), 0.f, FRotator(-18.f, i * 72.f, 0.f));
		}
		return;
	}
	// Pohon rindang (mangga/ketapang): batang + dua gumpal daun
	const FLinearColor Leaf = Kind == 2 ? FLinearColor(0.17f, 0.4f, 0.16f) : FLinearColor(0.24f, 0.5f, 0.2f);
	Shape(Cylinder, At + FVector(0, 0, 120 * Size), FVector(0.28f * Size, 0.28f * Size, 2.4f * Size), Trunk);
	Shape(Sphere, At + FVector(0, 0, 310 * Size), FVector(2.6f * Size, 2.6f * Size, 2.1f * Size), Leaf);
	Shape(Sphere, At + FVector(60 * Size, -50 * Size, 390 * Size), FVector(1.6f * Size, 1.6f * Size, 1.4f * Size), Leaf * 1.12f);
}

void AJRDecor::BuildKampung(float HX, float HY)
{
	static const FLinearColor Walls[] = {
		FLinearColor(0.93f, 0.88f, 0.72f), FLinearColor(0.72f, 0.86f, 0.74f), FLinearColor(0.7f, 0.8f, 0.92f),
		FLinearColor(0.95f, 0.76f, 0.62f), FLinearColor(0.88f, 0.88f, 0.88f), FLinearColor(0.93f, 0.8f, 0.85f) };
	const FLinearColor Grass(0.3f, 0.48f, 0.22f), Grass2(0.34f, 0.52f, 0.25f), Dirt(0.55f, 0.46f, 0.34f);

	// Tanah berumput luas sampai cakrawala (di bawah semua lantai/jalan). Satu-satunya pijakan di luar gedung
	// (level tidak punya landscape lagi), jadi wajib collision agar pemain yang keluar pintu tidak jatuh.
	if (UStaticMeshComponent* Ground = Shape(Cube, FVector(0, 0, -14.f), FVector(900.f, 900.f, 0.1f), Grass))
		Ground->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// Pagar tak terlihat: pemain boleh ke jalan depan dan gang samping, tetapi tidak berjalan jauh ke kampung.
	{
		const float X0 = -HX - 1150.f, X1 = HX + 900.f, Y1 = HY + 1100.f, H = 600.f;
		const FVector Fence[][2] = {
			{ FVector(X0, 0, H / 2), FVector(20.f, 2.f * Y1, H) }, { FVector(X1, 0, H / 2), FVector(20.f, 2.f * Y1, H) },
			{ FVector((X0 + X1) / 2, -Y1, H / 2), FVector(X1 - X0, 20.f, H) }, { FVector((X0 + X1) / 2, Y1, H / 2), FVector(X1 - X0, 20.f, H) } };
		for (const auto& W : Fence)
		{
			UStaticMeshComponent* C = Shape(Cube, W[0], W[1] / 100.f, Grass);
			C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			C->SetHiddenInGame(true);
			C->SetCastShadow(false);
		}
	}
	// Halaman belakang dan samping (sedikit beda warna + jalan setapak tanah)
	Shape(Cube, FVector(HX + 700.f, 0, -8.f), FVector(14.f, (2 * HY + 2400.f) / 100.f, 0.1f), Grass2);
	Shape(Cube, FVector(HX + 320.f, 0, -6.f), FVector(1.2f, (2 * HY + 2000.f) / 100.f, 0.1f), Dirt);

	// Rumah-rumah di belakang gedung (punggungnya menghadap kita)
	int32 K = 7;
	for (float Y = -HY - 400.f; Y < HY + 500.f; Y += 760.f, ++K)
	{
		House(FVector(HX + 1150.f, Y, 0), 620.f, 300.f + (K * 37 % 3) * 70.f, Walls[K % 6], K);
	}
	// Rumah di kiri dan kanan gedung (dipisah gang kecil)
	for (float Side : { -1.f, 1.f })
	{
		for (float X = -HX + 350.f; X < HX + 400.f; X += 760.f, ++K)
		{
			House(FVector(X + 250.f, Side * (HY + 600.f), 0), 560.f, 290.f + (K * 41 % 3) * 60.f, Walls[K % 6], K);
		}
	}

	// Lapisan jauh: atap-atap rumah dan masjid berkubah sebagai penanda kampung
	const float R = FMath::Max(HX, HY) + 3200.f;
	for (int32 i = 0; i < 14; ++i, ++K)
	{
		const float A = (i / 14.f) * 2.f * PI + 0.2f;
		const FVector P(FMath::Cos(A) * R, FMath::Sin(A) * R * 1.15f, 0.f);
		if (P.X < -HX - 2600.f && FMath::Abs(P.Y) < HY + 1800.f) continue;	// jangan menutupi jalan depan
		House(P, 700.f, 340.f + (K * 29 % 4) * 60.f, Walls[K % 6], K);
	}
	{
		const FVector M(HX + 3600.f, -HY - 1500.f, 0.f);	// masjid di belakang kiri
		Shape(Cube, M + FVector(0, 0, 230), FVector(10.f, 10.f, 4.6f), FLinearColor(0.95f, 0.95f, 0.9f));
		Shape(Sphere, M + FVector(0, 0, 520), FVector(6.f, 6.f, 5.f), FLinearColor(0.15f, 0.55f, 0.35f));
		Shape(Cylinder, M + FVector(0, 0, 790), FVector(0.1f, 0.1f, 0.9f), FLinearColor(0.85f, 0.75f, 0.3f));
		Shape(Cylinder, M + FVector(-650, 600, 600), FVector(1.1f, 1.1f, 12.f), FLinearColor(0.95f, 0.95f, 0.9f));	// menara
		Shape(Cone, M + FVector(-650, 600, 1250), FVector(1.4f, 1.4f, 1.4f), FLinearColor(0.15f, 0.55f, 0.35f));
	}

	// Pepohonan di sela rumah
	const FVector Trees[] = {
		FVector(HX + 500.f, -HY - 300.f, 0), FVector(HX + 450.f, HY + 250.f, 0), FVector(HX + 1700.f, 200.f, 0),
		FVector(-HX + 100.f, -HY - 1500.f, 0), FVector(HX + 200.f, HY + 1500.f, 0), FVector(HX + 2300.f, -HY + 400.f, 0),
		FVector(HX + 2100.f, HY + 900.f, 0), FVector(-HX - 300.f, HY + 1700.f, 0), FVector(HX + 900.f, -HY - 1700.f, 0),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Trees); ++i) Tree(Trees[i], 1.f + (i % 3) * 0.25f, i % 3);

	// Tiang listrik berkabel menyusuri gang samping kanan
	const FLinearColor PoleC(0.45f, 0.45f, 0.47f), Wire(0.08f, 0.08f, 0.09f);
	const float PY = HY + 230.f;
	TArray<FVector> Tops;
	for (float X = -HX - 300.f; X <= HX + 900.f; X += 800.f)
	{
		Shape(Cylinder, FVector(X, PY, 380), FVector(0.14f, 0.14f, 7.6f), PoleC);
		Shape(Cube, FVector(X, PY, 720), FVector(0.08f, 1.2f, 0.08f), PoleC);	// palang
		Tops.Add(FVector(X, PY, 715));
	}
	for (int32 i = 0; i + 1 < Tops.Num(); ++i)
	{
		for (float Dy : { -45.f, 45.f })
		{
			const FVector A = Tops[i] + FVector(0, Dy, 0), B = Tops[i + 1] + FVector(0, Dy, 0);
			Shape(Cube, (A + B) * 0.5f - FVector(0, 0, 18.f), FVector((B - A).Size() / 100.f, 0.02f, 0.02f), Wire);	// kabel agak melengkung ke bawah
		}
	}
}

void AJRDecor::Setup(FName Id, float HX, float HY, float DoorY)
{
	if (Id == TEXT("luar")) { BuildStreet(HX, HY, DoorY, HY > 1100.f); BuildKampung(HX, HY); return; }
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
		L->SetCastShadows(false);	// lampu ruangan tanpa bayangan: jauh lebih ringan di HP
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

UTextRenderComponent* AJRRoom::OutsideText(const FString& Text, const FVector& Loc, float Size, const FColor& Color)
{
	UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
	T->SetupAttachment(Root);
	T->SetRelativeLocation(Loc);
	T->SetRelativeRotation(FRotator(0, 180, 0));	// terbaca dari jalan (menghadap -X)
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetWorldSize(Size);
	T->SetTextRenderColor(Color);
	if (SignMaterial) T->SetTextMaterial(SignMaterial);
	T->SetText(FText::FromString(Text));
	T->RegisterComponent();
	AddInstanceComponent(T);
	return T;
}

void AJRRoom::BuildFacade(const FJRLayout& L, int32 Era, float WallH)
{
	const float HX = L.HalfX, HY = L.HalfY, DoorY = L.DoorY;
	const float Face = -(HX + 20.f);	// sisi luar dinding depan
	const FColor White(255, 255, 255), Dark(30, 25, 20), Yellow(255, 214, 80);

	// Pelipit gelap di kaki dinding depan
	AddPart(this, Root, EShape::Cube, FVector(4, 2 * HY + 40, 40), FVector(Face - 2.f, 0, 20), Hex(0x4a4f57), false);

	// Papan nama di atas pintu (rangka gelap + panel warna era) dan lis lampu
	const float SignW = FMath::Min(700.f, 2 * HY * 0.45f);
	const float SignZ = WallH > 400.f ? 290.f : 262.f;
	const FLinearColor Panel = Era >= 3 ? Hex(0x1d3d8f) : Era == 2 ? Hex(0xb5282b) : Hex(0xe0a526);
	AddPart(this, Root, EShape::Cube, FVector(10, SignW + 20, 82), FVector(Face - 5.f, DoorY, SignZ), Hex(0x22252b), false);
	AddPart(this, Root, EShape::Cube, FVector(4, SignW, 70), FVector(Face - 11.f, DoorY, SignZ), Panel, false);
	const AJRGameMode* GMN = GetWorld()->GetAuthGameMode<AJRGameMode>();
	const FString RN = (GMN ? GMN->RentalName : FString(TEXT("Juragan"))).ToUpper();
	const FString Name = Era >= 2 ? JRPrintf(JRT(TEXT("GAME CENTER %s")), *RN) : JRPrintf(JRT(TEXT("RENTAL PS & WARNET %s")), *RN);
	OutsideText(Name, FVector(Face - 14.f, DoorY, SignZ + 6.f), Era == 1 ? 30.f : 36.f, Era == 1 ? Dark : White);
	OutsideText(JRT(TEXT("PS  -  WARNET  -  MI INSTAN")), FVector(Face - 14.f, DoorY, SignZ - 24.f), 15.f, Era == 1 ? Dark : Yellow);
	AddPart(this, Root, EShape::Cube, FVector(4, SignW + 20, 4), FVector(Face - 12.f, DoorY, SignZ - 44.f), Hex(0x6ff0ff), false);

	// Kanopi bergaris merah-putih di atas pintu
	const int32 Stripes = 7;
	const float CanW = 260.f, StripeW = CanW / Stripes;
	for (int32 i = 0; i < Stripes; ++i)
	{
		const float Y = DoorY - CanW / 2 + (i + 0.5f) * StripeW;
		if (UStaticMeshComponent* S = AddPart(this, Root, EShape::Cube, FVector(110, StripeW, 4), FVector(Face - 50.f, Y, 228.f), i % 2 ? Hex(0xf2f2ee) : Hex(0xc9302c), false))
		{
			S->SetRelativeRotation(FRotator(-14.f, 0.f, 0.f));	// miring ke arah jalan
		}
	}

	// Jendela kaca kiri dan kanan pintu, dengan tulisan tempel
	for (int32 Side : { -1, 1 })
	{
		const float Y = DoorY + Side * 260.f;
		if (FMath::Abs(Y) > HY - 80.f) continue;
		AddPart(this, Root, EShape::Cube, FVector(4, 170, 110), FVector(Face - 2.f, Y, 135.f), Hex(0x2b3138), false);	// kusen
		AddPart(this, Root, EShape::Cube, FVector(4, 156, 96), FVector(Face - 4.f, Y, 135.f), Hex(0x24496b), false);	// kaca gelap agar tulisan terbaca
		OutsideText(Side < 0 ? JRT(TEXT("BUKA 08.00 - 24.00")) : JRT(TEXT("PS2  -  PC GAMING")), FVector(Face - 7.f, Y, 150.f), 15.f, White);
		OutsideText(Side < 0 ? JRT(TEXT("SELAMAT DATANG")) : JRT(TEXT("MI + ES TEH")), FVector(Face - 7.f, Y, 120.f), 13.f, Yellow);
	}

	// Spanduk turnamen (ruko dan game center)
	if (Era >= 2)
	{
		const float Y = DoorY - 520.f > -HY + 120.f ? DoorY - 560.f : DoorY + 560.f;
		AddPart(this, Root, EShape::Cube, FVector(3, 300, 70), FVector(Face - 3.f, Y, 230.f), Hex(0xf4f1e6), false);
		OutsideText(JRT(TEXT("TURNAMEN PS TIAP SABTU!")), FVector(Face - 6.f, Y, 238.f), 17.f, FColor(190, 30, 25));
		OutsideText(JRT(TEXT("Hadiah jutaan - daftar di kasir")), FVector(Face - 6.f, Y, 214.f), 11.f, Dark);
	}

	// Game Center: papan besar di lantai 2 dan jendela lantai 2
	if (Era >= 3)
	{
		AddPart(this, Root, EShape::Cube, FVector(10, 820, 120), FVector(Face - 5.f, 0.f, L.FloorH + 210.f), Hex(0x111318), false);
		OutsideText(JRPrintf(JRT(TEXT("GAME CENTER %s  -  2 LANTAI")), *RN), FVector(Face - 12.f, 0.f, L.FloorH + 222.f), 46.f, FColor(110, 240, 255));
		OutsideText(JRT(TEXT("VIP LOUNGE  -  PLAYSTASIUN 3  -  WARNET FIBER")), FVector(Face - 12.f, 0.f, L.FloorH + 180.f), 17.f, Yellow);
		for (float Y : { -0.7f * HY, -0.35f * HY, 0.35f * HY, 0.7f * HY })
		{
			AddPart(this, Root, EShape::Cube, FVector(4, 200, 100), FVector(Face - 2.f, Y, L.FloorH + 90.f), Hex(0x2b3138), false);
			AddPart(this, Root, EShape::Cube, FVector(4, 186, 86), FVector(Face - 4.f, Y, L.FloorH + 90.f), Hex(0x6fa3c9), false);
		}
	}

	// Lampu sorot papan nama (menerangi tampak depan saat malam)
	UPointLightComponent* Spot = NewObject<UPointLightComponent>(this);
	Spot->SetupAttachment(Root);
	Spot->SetRelativeLocation(FVector(Face - 90.f, DoorY, SignZ + 20.f));
	Spot->SetIntensityUnits(ELightUnits::Candelas);
	Spot->SetIntensity(25.f);
	Spot->SetAttenuationRadius(500.f);
	Spot->SetLightColor(FLinearColor(1.f, 0.9f, 0.75f));
	Spot->SetCastShadows(false);	// lampu ruangan tanpa bayangan: jauh lebih ringan di HP
	Spot->RegisterComponent();
	AddInstanceComponent(Spot);
}

void AJRRoom::SetLightsOn(bool bOn)
{
	for (UPointLightComponent* L : Lights) if (L) L->SetVisibility(bOn);
}

void AJRRoom::Build(const FJRLayout& L, int32 Era)
{
	const float HX = L.HalfX, HY = L.HalfY, DoorY = L.DoorY;
	const bool bRuko = Era >= 2;
	const bool bUp = L.HasUpstairs();
	const float FH = L.FloorH;
	const float WallH = bUp ? FH + 350.f : 300.f;	// tinggi dinding luar
	// Warna khas tiap era (dinding atas, cat bawah/lambri, lis, lantai, nat):
	// Garasi = cat krem + tosca khas rumah kampung, lantai semen; Ruko = kuning muda + biru, keramik krem;
	// Game Center = ungu tua ala gamer, lantai abu gelap.
	// Dinding bangunan (juga tampak dari luar) tetap netral; warna dalam berupa lapisan cat tipis di sisi dalam.
	const FLinearColor Wall = bRuko ? Hex(0xcfd8d4) : Hex(0xd8cdb4);
	const FLinearColor Wall2 = bRuko ? Hex(0xbac6c2) : Hex(0xc9bc9f);
	const FLinearColor Paint  = bUp ? Hex(0x3d3a63) : bRuko ? Hex(0xf2e2a6) : Hex(0xeee0bd);
	const FLinearColor Paint2 = bUp ? Hex(0x35325a) : bRuko ? Hex(0xe8d595) : Hex(0xe6d6af);
	const FLinearColor Lower = bUp ? Hex(0x23213d) : bRuko ? Hex(0x4f7fb5) : Hex(0x4f9e93);
	const FLinearColor Trim  = bUp ? Hex(0x29d3ff) : bRuko ? Hex(0x2f5a8c) : Hex(0x2f6f66);
	const FLinearColor Floor = bUp ? Hex(0x55545e) : bRuko ? Hex(0xe2d6c0) : Hex(0x7f7a72);
	const FLinearColor Grout = bUp ? Hex(0x3e3d46) : bRuko ? Hex(0xc4b79f) : Hex(0x6c675f);

	// Lantai: garasi semen dengan nat besar tiap 2 m, ruko/game center keramik tiap 1 m
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 2 * HY, 10), FVector(0, 0, -4), Floor);
	const float Tile = bRuko ? 100.f : 200.f;
	for (float X = -HX + Tile; X < HX; X += Tile)
	{
		AddPart(this, Root, EShape::Cube, FVector(3, 2 * HY, 1), FVector(X, 0, 1.5f), Grout, false);
	}
	for (float Y = -HY + Tile; Y < HY; Y += Tile)
	{
		AddPart(this, Root, EShape::Cube, FVector(2 * HX, 3, 1), FVector(0, Y, 1.5f), Grout, false);
	}

	// Dinding 3 m. Belakang = +X, depan (pintu) = -X, kiri = -Y, kanan = +Y.
	AddPart(this, Root, EShape::Cube, FVector(20, 2 * HY + 40, WallH), FVector(HX + 10, 0, WallH / 2), Wall);
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 20, WallH), FVector(0, -(HY + 10), WallH / 2), Wall2);
	AddPart(this, Root, EShape::Cube, FVector(2 * HX, 20, WallH), FVector(0, HY + 10, WallH / 2), Wall2);
	const float L0 = -(HY + 10), L1 = DoorY - 100.f, R0 = DoorY + 100.f, R1 = HY + 10;
	AddPart(this, Root, EShape::Cube, FVector(20, L1 - L0, WallH), FVector(-(HX + 10), (L0 + L1) / 2, WallH / 2), Wall2); // depan, kiri pintu
	AddPart(this, Root, EShape::Cube, FVector(20, R1 - R0, WallH), FVector(-(HX + 10), (R0 + R1) / 2, WallH / 2), Wall2); // depan, kanan pintu
	AddPart(this, Root, EShape::Cube, FVector(20, 200, WallH - 220.f), FVector(-(HX + 10), DoorY, (220.f + WallH) / 2), Wall2); // di atas pintu

	// Cat dinding dalam (setebal 1 cm, setinggi dinding) lalu cat bawah (lambri) 110 cm + lis di lantai 1
	{
		auto Cat = [&](float SX, float SY, float CX, float CY, const FLinearColor& C)
		{
			AddPart(this, Root, EShape::Cube, FVector(SX, SY, WallH), FVector(CX, CY, WallH / 2), C, false);
		};
		Cat(1, 2 * HY, HX - 0.5f, 0, Paint);
		Cat(2 * HX, 1, 0, -(HY - 0.5f), Paint2);
		Cat(2 * HX, 1, 0, HY - 0.5f, Paint2);
		Cat(1, (DoorY - 100.f) + HY, -(HX - 0.5f), (-HY + DoorY - 100.f) / 2, Paint2);
		Cat(1, HY - (DoorY + 100.f), -(HX - 0.5f), (DoorY + 100.f + HY) / 2, Paint2);
		AddPart(this, Root, EShape::Cube, FVector(1, 200, WallH - 220.f), FVector(-(HX - 0.5f), DoorY, (220.f + WallH) / 2), Paint2, false);	// di atas pintu
	}
	{
		const float LH = 110.f;
		auto Lambri = [&](float SX, float SY, float CX, float CY)
		{
			AddPart(this, Root, EShape::Cube, FVector(SX, SY, LH), FVector(CX, CY, LH / 2), Lower, false);
			AddPart(this, Root, EShape::Cube, FVector(SX + 1.f, SY + 1.f, 6), FVector(CX, CY, LH + 3.f), Trim, false);
		};
		Lambri(2, 2 * HY, HX - 1.f, 0);															// belakang
		Lambri(2 * HX, 2, 0, -(HY - 1.f));															// kiri
		Lambri(2 * HX, 2, 0, HY - 1.f);															// kanan
		Lambri(2, (DoorY - 100.f) + HY, -(HX - 1.f), (-HY + DoorY - 100.f) / 2);					// depan, kiri pintu
		Lambri(2, HY - (DoorY + 100.f), -(HX - 1.f), (DoorY + 100.f + HY) / 2);					// depan, kanan pintu
	}

	if (!bUp)
	{
		// Plafon
		AddPart(this, Root, EShape::Cube, FVector(2 * HX + 40, 2 * HY + 40, 10), FVector(0, 0, 305), bRuko ? Hex(0xf3efe4) : Hex(0xd9d3c4));
	}
	else
	{
		// ---- Lantai 2: pelat lantai (berlubang di atas tangga), tangga, pagar, atap ----
		const float SY0 = -HY, SY1 = -HY + L.StairW, SX0 = L.StairX0, SX1 = L.StairX1;
		const FLinearColor Slab = Hex(0x5b2b33), Under = Hex(0xe8e4da);
		auto SlabPart = [&](float X0, float X1, float Y0, float Y1)
		{
			AddPart(this, Root, EShape::Cube, FVector(X1 - X0, Y1 - Y0, 12), FVector((X0 + X1) / 2, (Y0 + Y1) / 2, FH - 6.f), Slab);		// karpet lantai 2
			AddPart(this, Root, EShape::Cube, FVector(X1 - X0, Y1 - Y0, 2), FVector((X0 + X1) / 2, (Y0 + Y1) / 2, FH - 13.f), Under, false);	// plafon lantai 1
		};
		SlabPart(-HX, HX, SY1, HY);	// sebagian besar lantai
		SlabPart(-HX, SX0, SY0, SY1);	// di depan kaki tangga
		SlabPart(SX1, HX, SY0, SY1);	// bordes di puncak tangga
		// Anak tangga: 14 x 25 cm, padat sampai lantai (pemain dan pelanggan bisa naik)
		const int32 Steps = 14;
		const float Run = (SX1 - SX0) / Steps, Rise = FH / Steps;
		for (int32 i = 0; i < Steps; ++i)
		{
			const float Top = (i + 1) * Rise;
			AddPart(this, Root, EShape::Cube, FVector(Run, L.StairW, Top), FVector(SX0 + (i + 0.5f) * Run, (SY0 + SY1) / 2, Top / 2), i % 2 ? Hex(0x8a6a4a) : Hex(0x7d5f42));
		}
		// Pagar di tepi lubang tangga (lantai 2) dan pegangan tangga
		const FLinearColor Metal = Hex(0x3a3d42);
		// Lantai 2: penghalang tak terlihat + palang atas dan tiang (pagar besi)
		if (UStaticMeshComponent* Block = AddPart(this, Root, EShape::Cube, FVector(SX1 - SX0, 6, 100), FVector((SX0 + SX1) / 2, SY1 + 3.f, FH + 50.f), Metal))
		{
			Block->SetVisibility(false);
		}
		AddPart(this, Root, EShape::Cube, FVector(SX1 - SX0, 6, 6), FVector((SX0 + SX1) / 2, SY1 + 3.f, FH + 100.f), Metal, false);
		for (float X = SX0; X <= SX1 + 1.f; X += 75.f)
		{
			AddPart(this, Root, EShape::Cube, FVector(5, 5, 100), FVector(X, SY1 + 3.f, FH + 50.f), Metal, false);
		}
		// Pegangan tangga miring mengikuti anak tangga, ditopang tiang tiap dua anak tangga
		const float RailLen = FMath::Sqrt(FMath::Square(SX1 - SX0) + FMath::Square(FH));
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(FH, SX1 - SX0));
		if (UStaticMeshComponent* Rail = AddPart(this, Root, EShape::Cube, FVector(RailLen, 6, 6), FVector((SX0 + SX1) / 2, SY1 - 6.f, FH / 2 + 90.f), Metal, false))
		{
			Rail->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
		}
		for (int32 i = 1; i < Steps; i += 2)
		{
			const float X = SX0 + (i + 0.5f) * Run;
			const float StepTop = (i + 1) * Rise;
			const float RailZ = (X - SX0) / (SX1 - SX0) * FH + 90.f;
			AddPart(this, Root, EShape::Cube, FVector(5, 5, RailZ - StepTop), FVector(X, SY1 - 6.f, (RailZ + StepTop) / 2), Metal, false);
		}
		// Atap
		AddPart(this, Root, EShape::Cube, FVector(2 * HX + 40, 2 * HY + 40, 10), FVector(0, 0, WallH + 5.f), Under);
		// Papan "LANTAI 2 - VIP" di dinding belakang lantai 2
		UTextRenderComponent* Vip = NewObject<UTextRenderComponent>(this);
		Vip->SetupAttachment(Root);
		Vip->SetRelativeLocation(FVector(HX - 6, 100.f, FH + 260.f));
		Vip->SetRelativeRotation(FRotator(0, 180, 0));
		Vip->SetHorizontalAlignment(EHTA_Center);
		Vip->SetVerticalAlignment(EVRTA_TextCenter);
		Vip->SetWorldSize(42.f);
		Vip->SetTextRenderColor(FColor(255, 196, 70));
		if (SignMaterial) Vip->SetTextMaterial(SignMaterial);
		Vip->SetText(FText::FromString(JRT(TEXT("LANTAI 2  -  VIP LOUNGE"))));
		Vip->RegisterComponent();
		AddInstanceComponent(Vip);
		// Lampu lantai 2
		for (float X : { -0.5f * HX, 0.1f * HX, 0.65f * HX })
		{
			for (float Y : { -0.35f * HY, 0.45f * HY })
			{
				const FVector Pos(X, Y, WallH - 6.f);
				AddProp(this, Root, TEXT("SM_Lampu_Neon"), Pos, 0.f, false);
				UPointLightComponent* Up = NewObject<UPointLightComponent>(this);
				Up->SetupAttachment(Root);
				Up->SetRelativeLocation(Pos - FVector(0, 0, 10));
				Up->SetMobility(EComponentMobility::Movable);
				Up->SetIntensityUnits(ELightUnits::Candelas);
				Up->SetIntensity(34.f);
				Up->SetAttenuationRadius(1200.f);
				Up->SetLightColor(FLinearColor(1.f, 0.9f, 0.8f));
				Up->SetCastShadows(false);	// lampu ruangan tanpa bayangan: jauh lebih ringan di HP
				Up->RegisterComponent();
				Lights.Add(Up);
				AddInstanceComponent(Up);
			}
		}
	}

	BuildFacade(L, Era, WallH);

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
	Sign->SetWorldSize(Era >= 3 ? 15.f : 19.f);
	Sign->SetTextRenderColor(FColor(45, 8, 4)); // hampir hitam di atas papan terang: kontras tinggi
	if (SignMaterial)
	{
		Sign->SetTextMaterial(SignMaterial); // tanpa cahaya: warna tetap sama di ruangan terang maupun gelap
	}
	{
		const AJRGameMode* GMS = GetWorld()->GetAuthGameMode<AJRGameMode>();
		const FString SN = (GMS ? GMS->RentalName : FString(TEXT("Juragan"))).ToUpper();
		Sign->SetText(FText::FromString(Era >= 3 ? JRPrintf(JRT(TEXT("GAME CENTER %s  -  2 LANTAI")), *SN) : bRuko ? JRPrintf(JRT(TEXT("GAME CENTER %s")), *SN) : JRPrintf(JRT(TEXT("RENTAL PS & WARNET %s")), *SN)));
	}
	Sign->RegisterComponent();
	AddInstanceComponent(Sign);

	// Bangku antre, rak kaset, poster
	AddProp(this, Root, TEXT("SM_Bangku_Panjang"), FVector(-(HX - 50), 30, 0));
	AddProp(this, Root, TEXT("SM_Rak_Kaset"), FVector(HX - 10, 0.25f * HY, 100), 180.f, false);
	AddProp(this, Root, TEXT("SM_Poster"), FVector(-0.64f * HX, -(HY - 2), 140), 90.f, false);
	if (!bUp) AddProp(this, Root, TEXT("SM_Poster"), FVector(0.74f * HX, -(HY - 2), 150), 90.f, false);	// di Era 3 dinding ini tertutup tangga
	if (bRuko)
	{
		AddProp(this, Root, TEXT("SM_Kulkas_Minuman"), FVector(bUp ? -320.f : 40.f, -(HY - 35), 0), 90.f); // kulkas minuman dekat kantin
		AddProp(this, Root, TEXT("SM_Poster"), FVector(0.2f * HX, HY - 2, 150), -90.f, false);
	}

	BuildClutter(L, Era);

	// Lampu neon
	const float Xs[] = { -0.5f * HX, 0.07f * HX, 0.64f * HX };
	for (float X : Xs)
	{
		for (float Y : { -0.45f * HY, 0.45f * HY })
		{
			const FVector Pos(X, Y, bUp ? FH - 20.f : 296.f);
			AddProp(this, Root, TEXT("SM_Lampu_Neon"), Pos, 0.f, false);
			UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
			Light->SetupAttachment(Root);
			Light->SetRelativeLocation(Pos - FVector(0, 0, 10));
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->SetIntensity(bRuko ? 40.f : 34.f);	// lebih redup: dinding berwarna tidak pucat, langit-langit tidak silau
			Light->SetAttenuationRadius(bRuko ? 1300.f : 1100.f);
			Light->SetLightColor(FLinearColor(1.f, 0.97f, 0.9f));
			Light->SetCastShadows(false);	// lampu ruangan tanpa bayangan: jauh lebih ringan di HP
			Light->RegisterComponent();
			Lights.Add(Light);
			AddInstanceComponent(Light);
		}
	}
}

/* ---------- Barang pengisi ruangan (model Polyfork, tanpa tabrakan agar jalur pelanggan aman) ---------- */

void AJRRoom::BuildClutter(const FJRLayout& L, int32 Era)
{
	const float HX = L.HalfX, HY = L.HalfY, DoorY = L.DoorY;
	const bool bUp = L.HasUpstairs();
	// FaceYaw: arah hadap model (0 = +X / ke dinding belakang, 180 = ke pintu, 90 = ke kanan, -90 = ke kiri)
	auto Put = [&](const TCHAR* Name, const FVector& At, float FaceYaw, float Scale = 1.f)
	{
		if (UStaticMeshComponent* C = AddProp(this, Root, Name, At, FaceYaw + PfYaw, false))
		{
			C->SetRelativeScale3D(FVector(Scale));
			C->SetCastShadow(false);
		}
	};
	const float CeilZ = bUp ? L.FloorH - 14.f : 300.f;
	// Catatan: model dinding Polyfork sudah membawa tinggi gantungnya sendiri (Z di sini = geseran dari tinggi itu):
	// jam 222-258 cm, papan menu 140-252, bingkai 120-190, lampu hias 270-287, pot gantung 265-320.

	// Jam dinding (Game Center: dinding kiri tertutup tangga, pindah ke kanan) dan papan menu kantin dekat kompor
	if (!bUp) Put(TEXT("SM_PF_Wall_Clock"), FVector(0.14f * HX, -(HY - 6.f), 0.f), 90.f);
	else Put(TEXT("SM_PF_Wall_Clock"), FVector(0.15f * HX, HY - 6.f, 0.f), -90.f);
	Put(TEXT("SM_PF_Chalkboard_Menu_Board"), FVector(L.Stove().X - 170.f, -(HY - 2.f), -40.f), 90.f);
	// Dinding kanan (+Y): bingkai gambar di antara PC 1 dan PC 2
	if (L.PcX.Num() >= 2) Put(TEXT("SM_PF_Framed_Wall_Art"), FVector((L.PcX[0] + L.PcX[1]) * 0.5f, HY - 2.f, 15.f), -90.f);
	// Pojok belakang: kabel ruwet di belakang TV pertama
	if (L.PsY.Num() > 0) Put(TEXT("SM_PF_Cable_Spaghetti_Bundle"), FVector(HX - 25.f, L.PsY[0] - 95.f, 0.f), 180.f, 0.8f);
	// Pojok depan kanan: rak kawat berisi kardus, krat minuman
	{
		const FVector Rack(-(HX - 32.f), HY - 120.f, 0.f);
		Put(TEXT("SM_PF_Wire_Shelving_Rack"), Rack, 0.f);
		Put(TEXT("SM_PF_Cardboard_Delivery_Box"), Rack + FVector(0.f, -20.f, 104.f), 0.f, 0.85f);
		Put(TEXT("SM_PF_Cardboard_Delivery_Box"), Rack + FVector(0.f, 25.f, 141.f), 10.f, 0.7f);
		Put(TEXT("SM_PF_Beverage_Crate"), FVector(-(HX - 30.f), HY - 220.f, 0.f), 0.f);
		Put(TEXT("SM_PF_Beverage_Crate"), FVector(-(HX - 30.f), HY - 220.f, 41.f), 15.f);
	}
	// Depan: tanaman di antara bangku antre dan kasir, APAR dekat pintu, tempat sampah kasir
	Put(TEXT("SM_PF_Floor_Houseplant"), FVector(-(HX - 45.f), -0.28f * HY, 0.f), 0.f);
	Put(TEXT("SM_PF_Fire_Extinguisher"), FVector(-(HX - 14.f), DoorY - 125.f, 0.f), 0.f);
	Put(TEXT("SM_PF_Wastepaper_Bin"), FVector(L.Kasir().X - 80.f, L.Kasir().Y - 110.f, 0.f), 0.f);
	// Pot gantung di dekat pintu
	Put(TEXT("SM_PF_Hanging_Plant_Basket"), FVector(-(HX - 60.f), DoorY + 170.f, CeilZ - 320.f), 0.f);

	// Luar gedung: kotak meteran PLN, tong air, pot tanaman, tempat sampah, AC luar dan motor (ruko ke atas)
	const float Out = -(HX + 20.f);
	Put(TEXT("SM_PF_Meter_Box"), FVector(Out - 14.f, DoorY - 210.f, 110.f), 180.f);
	Put(TEXT("SM_PF_Plastic_Drum"), FVector(Out - 45.f, DoorY + 230.f, 0.f), 0.f);
	Put(TEXT("SM_PF_Potted_Shrub"), FVector(Out - 50.f, DoorY - 140.f, 0.f), 0.f, 0.8f);
	Put(TEXT("SM_PF_Potted_Shrub"), FVector(Out - 50.f, DoorY + 140.f, 0.f), 40.f, 0.8f);
	Put(TEXT("SM_PF_Street_Bin"), FVector(Out - 60.f, DoorY - 330.f, 0.f), 180.f);
	if (Era >= 2)
	{
		Put(TEXT("SM_PF_Air_Con_Unit"), FVector(Out - 24.f, -0.55f * HY, 40.f), 180.f);
		Put(TEXT("SM_PF_Delivery_Moped"), FVector(Out - 330.f, DoorY + 420.f, 0.f), 90.f);
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
		Out.Prompt = JRPrintf(JRT(TEXT("Terima bayaran %s  (%s)")), *Payer->DisplayName, *JRFormatRp(Payer->Bill));
	}
	return Out;
}

void AJRKasir::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>())
	{
		GM->CollectNextPayer(true);
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
		Out.Prompt = JRT(TEXT("Naikkan MCB"));
	}
	else
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = JRT(TEXT("MCB menyala"));
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
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* P = AddPart(this, Root, EShape::Sphere, FVector(10.f), FVector(2, 30, 112), FLinearColor(0.92f, 0.92f, 0.95f), false);
		P->SetCastShadow(false);
		P->SetVisibility(false);
		Puffs.Add(P);
	}
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
		Text = JRPrintf(JRT(TEXT("MASAK %d%%")), FMath::RoundToInt(GM->CookProgress / AJRGameMode::CookSeconds * 100.f));
	}
	else if (GM->StoveState == EJRStoveState::Ready)
	{
		Text = TEXT("MATANG!");
		Color = FColor(92, 201, 138);
	}
	Label->SetText(FText::FromString(Text));
	Label->SetTextRenderColor(Color);
	{
		// Uap: tiap gumpal naik 45 cm sambil membesar lalu mengecil, bergiliran (saat matang lebih pelan)
		const bool bSteam = (GM->StoveState == EJRStoveState::Cooking && !GM->bMcbTripped) || GM->StoveState == EJRStoveState::Ready;
		SteamTime += DeltaSeconds * (GM->StoveState == EJRStoveState::Ready ? 0.45f : 0.8f);
		for (int32 i = 0; i < Puffs.Num(); ++i)
		{
			UStaticMeshComponent* P = Puffs[i];
			if (!P) continue;
			if (P->IsVisible() != bSteam) P->SetVisibility(bSteam);
			if (!bSteam) continue;
			const float T = FMath::Frac(SteamTime + i / (float)Puffs.Num());
			const float Wob = FMath::Sin((SteamTime + i * 1.7f) * 4.f) * (4.f + T * 10.f);	// makin tinggi makin melebar
			P->SetRelativeLocation(FVector(2.f + Wob, 30.f + Wob * 0.5f, 108.f + T * 45.f));
			P->SetRelativeScale3D(FVector(FMath::Sin(T * PI) * 0.085f + 0.01f));
		}
	}
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
		Out.Prompt = JRT(TEXT("Taruh mi lagi di kompor  (antar/tawarkan ke pelanggan yang main)"));
	}
	else if (GM->IsCarrying())
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = JRT(TEXT("Tangan penuh. Pasang dulu kardusnya"));
	}
	else if (GM->StoveState == EJRStoveState::Idle)
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = JRPrintf(JRT(TEXT("Masak mi instan  (modal %s, kompor 600 VA)")), *JRFormatRp(AJRGameMode::MieCost));
	}
	else if (GM->StoveState == EJRStoveState::Cooking)
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = GM->bMcbTripped ? JRT(TEXT("Listrik mati, kompor berhenti")) : JRT(TEXT("Mi sedang dimasak..."));
	}
	else
	{
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = JRT(TEXT("Angkat mi ke mangkuk"));
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
	Label->SetText(FText::FromString(bSlow ? JRT(TEXT("INTERNET LEMOT")) : TEXT("")));
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
		Out.Prompt = JRT(TEXT("Tahan E: restart router"));
	}
	else
	{
		Out.Kind = EJRInteractKind::Info;
		Out.Prompt = JRT(TEXT("Internet lancar"));
	}
	return Out;
}

void AJRRouter::Interact()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->RestartRouter();
}
