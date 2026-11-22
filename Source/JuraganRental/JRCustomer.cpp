#include "JRCustomer.h"
#include "JRLang.h"
#include "UObject/StrongObjectPtr.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "JRGameMode.h"
#include "JRStation.h"
#include "JRSound.h"

namespace JRLocal_Customer
{
	UAnimSequence* LoadAnim(const TCHAR* Name)
	{
		return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Characters/Animations/%s.%s"), Name, Name));
	}
}
using namespace JRLocal_Customer;

AJRCustomer::AJRCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // model Blender menghadap +Y
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	HitBox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(Root);
	HitBox->InitCapsuleSize(38.f, 90.f);
	HitBox->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Bubble = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Bubble"));
	Bubble->SetupAttachment(Root);
	Bubble->SetRelativeLocation(FVector(0, 0, 200));
	Bubble->SetHorizontalAlignment(EHTA_Center);
	Bubble->SetWorldSize(20.f);
	Bubble->SetTextRenderColor(FColor(255, 194, 75));
	Bubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRCustomer::Setup(int32 InTypeIndex, const FString& InName, const TCHAR* MeshName, float InScale, EJRUnitType InWant, bool bInFlex)
{
	TypeIndex = InTypeIndex;
	DisplayName = InName;
	Want = InWant;
	bFlex = bInFlex;

	const FString MeshPath = FString::Printf(TEXT("/Game/Characters/%s.%s"), MeshName, MeshName);
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath))
	{
		Body->SetSkeletalMesh(Mesh);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JuraganRental: karakter %s tidak ditemukan"), *MeshPath);
	}
	SetActorScale3D(FVector(InScale * FMath::FRandRange(0.94f, 1.06f)));	// tinggi badan sedikit beragam
	RandomizeLook();
	UpdateAnimation();
}

void AJRCustomer::RandomizeLook()
{
	// Warna baju, celana, rambut, dan kulit diacak agar pelanggan tidak terlihat kembar.
	// Bocil SD dan anak SMA tetap berseragam (hanya rambut dan kulit yang beragam).
	// Dikunci (strong pointer) agar tidak dibersihkan garbage collector saat semua pelanggan pergi.
	static TStrongObjectPtr<UMaterialInterface> BaseRef(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_JR_Color.M_JR_Color")));
	if (!BaseRef.IsValid()) BaseRef.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_JR_Color.M_JR_Color")));
	UMaterialInterface* Base = BaseRef.Get();
	if (!Base) return;
	auto Pick = [](std::initializer_list<FLinearColor> L) { const int32 i = FMath::RandRange(0, (int32)L.size() - 1); return *(L.begin() + i); };
	const bool bUniform = TypeIndex == 0 || TypeIndex == 1;
	FLinearColor Shirt, Pants;
	switch (TypeIndex)
	{
	case 2: // mahasiswa: kaos/kemeja santai
		Shirt = Pick({ FLinearColor(0.2f, 0.35f, 0.6f), FLinearColor(0.75f, 0.3f, 0.25f), FLinearColor(0.85f, 0.8f, 0.7f), FLinearColor(0.3f, 0.55f, 0.35f), FLinearColor(0.55f, 0.45f, 0.7f) });
		Pants = Pick({ FLinearColor(0.15f, 0.22f, 0.4f), FLinearColor(0.35f, 0.32f, 0.28f), FLinearColor(0.12f, 0.12f, 0.13f) });
		break;
	case 3: // kantoran: kemeja pastel, celana bahan
		Shirt = Pick({ FLinearColor(0.8f, 0.85f, 0.95f), FLinearColor(0.95f, 0.95f, 0.95f), FLinearColor(0.85f, 0.75f, 0.85f), FLinearColor(0.75f, 0.85f, 0.78f) });
		Pants = Pick({ FLinearColor(0.12f, 0.12f, 0.14f), FLinearColor(0.25f, 0.25f, 0.28f), FLinearColor(0.3f, 0.25f, 0.18f) });
		break;
	case 4: // gamer: kaos gelap / jersey tim
		Shirt = Pick({ FLinearColor(0.08f, 0.08f, 0.09f), FLinearColor(0.6f, 0.08f, 0.1f), FLinearColor(0.1f, 0.25f, 0.55f), FLinearColor(0.15f, 0.4f, 0.2f), FLinearColor(0.85f, 0.55f, 0.1f) });
		Pants = Pick({ FLinearColor(0.1f, 0.1f, 0.11f), FLinearColor(0.2f, 0.2f, 0.22f) });
		break;
	default: // bapak-bapak: batik/kemeja cerah
		Shirt = Pick({ FLinearColor(0.55f, 0.3f, 0.12f), FLinearColor(0.2f, 0.4f, 0.5f), FLinearColor(0.6f, 0.15f, 0.15f), FLinearColor(0.85f, 0.75f, 0.45f) });
		Pants = Pick({ FLinearColor(0.15f, 0.15f, 0.16f), FLinearColor(0.35f, 0.3f, 0.22f) });
		break;
	}
	const FLinearColor Hair = Pick({ FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.08f, 0.05f, 0.03f), FLinearColor(0.15f, 0.09f, 0.05f) });
	const FLinearColor Skin = Pick({ FLinearColor(0.85f, 0.62f, 0.45f), FLinearColor(0.72f, 0.5f, 0.34f), FLinearColor(0.58f, 0.38f, 0.25f) });

	const TArray<FName> Slots = Body->GetMaterialSlotNames();
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FString N = Slots[i].ToString();
		FLinearColor C;
		if (N.EndsWith(TEXT("_hair"))) C = Hair;
		else if (N == TEXT("M_Skin")) C = Skin;
		else if (!bUniform && (N.EndsWith(TEXT("_shirt")) || N.EndsWith(TEXT("_sleeve")))) C = Shirt;
		else if (!bUniform && N.EndsWith(TEXT("_pants"))) C = Pants;
		else continue;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
		M->SetVectorParameterValue(TEXT("Color"), C);
		Body->SetMaterial(i, M);
	}
}

FJRInteraction AJRCustomer::GetInteraction() const
{
	if (State == EJRCustomerState::WaitPay)
	{
		FJRInteraction Out;
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = JRPrintf(JRT(TEXT("Terima bayaran %s  (%s)")), *DisplayName, *JRFormatRp(Bill));
		return Out;
	}
	if (Station && (State == EJRCustomerState::WaitStart || State == EJRCustomerState::Playing))
	{
		return Station->GetInteraction();
	}
	return FJRInteraction();
}

void AJRCustomer::Interact()
{
	if (State == EJRCustomerState::WaitPay)
	{
		if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->CollectPayer(this, true);
	}
	else if (Station)
	{
		Station->Interact();
	}
}

void AJRCustomer::PlayLoop(UAnimSequence* Anim)
{
	if (Anim && Anim != CurrentAnim)
	{
		Body->PlayAnimation(Anim, true);
		CurrentAnim = Anim;
	}
}

void AJRCustomer::UpdateAnimation()
{
	static const TCHAR* Walk = TEXT("A_Jalan");
	switch (State)
	{
	case EJRCustomerState::Walk:
		PlayLoop(LoadAnim(Walk));
		break;
	case EJRCustomerState::WaitStart:
	case EJRCustomerState::Playing:
		if (bRaging) { PlayLoop(LoadAnim(TEXT("A_Rage_Quit"))); break; }
		PlayLoop(LoadAnim(Station && Station->Type != EJRUnitType::PS ? TEXT("A_Duduk_Kursi_Main") : TEXT("A_Duduk_Lesehan_Main")));
		break;
	case EJRCustomerState::Queue:
		PlayLoop(LoadAnim(bBenchSeat ? TEXT("A_Duduk_Bangku") : TEXT("A_Idle")));
		break;
	default:
		PlayLoop(LoadAnim(TEXT("A_Idle")));
		break;
	}
}

void AJRCustomer::WalkTo(const FVector& Loc, EJRCustomerState Next)
{
	Path.Reset();
	if (const AJRGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AJRGameMode>() : nullptr) GM->MakePath(GetActorLocation(), Loc, Path);
	if (Path.Num() == 0) Path.Add(Loc);
	SegStart = GetActorLocation();
	Target = Path[0];
	NextState = Next;
	State = EJRCustomerState::Walk;
}

bool AJRCustomer::MoveStep(float DeltaSeconds)
{
	const FVector Loc = GetActorLocation();
	FVector Delta = Target - Loc;
	Delta.Z = 0.f;
	const float Dist = Delta.Size();
	if (Dist < 5.f)
	{
		SetActorLocation(Target);
		if (Path.Num() > 1)
		{
			// Lanjut ke titik berikutnya (mis. dari kaki tangga ke puncak tangga)
			Path.RemoveAt(0);
			SegStart = Target;
			Target = Path[0];
			return false;
		}
		return true;
	}
	const float Step = FMath::Min(Dist, Speed * DeltaSeconds);
	FVector Next = Loc + Delta / Dist * Step;
	// Tinggi mengikuti segmen (naik/turun tangga), bukan selalu di lantai 0
	const float SegLen = FVector::Dist2D(SegStart, Target);
	const float Alpha = SegLen > 1.f ? 1.f - FMath::Clamp((Dist - Step) / SegLen, 0.f, 1.f) : 1.f;
	Next.Z = FMath::Lerp(SegStart.Z, Target.Z, Alpha);
	SetActorLocation(Next);
	SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));
	return Dist - Step < 5.f && Path.Num() <= 1;
}

void AJRCustomer::Arrive(AJRGameMode* GM)
{
	State = NextState;
	switch (State)
	{
	case EJRCustomerState::WaitStart:
		if (Station)
		{
			SetActorLocation(Station->GetSeatLocation()); // animasi duduk yang menurunkan pinggul
			SetActorRotation(Station->GetSeatRotation());
			if (FMath::FRand() < 0.6f)
			{
				static const TCHAR* Codes[] = { TEXT("Bocil"), TEXT("SMA"), TEXT("Mahasiswa"), TEXT("Kantoran"), TEXT("Gamer"), TEXT("Bapak") };
				JRSound::Voice(this, Codes[FMath::Clamp(TypeIndex, 0, 5)], TEXT("Datang"), GetActorLocation());
			}
		}
		break;
	case EJRCustomerState::Queue:
		if (bBenchSeat)
		{
			SetActorLocation(Target);
			SetActorRotation(FRotator(0.f, 0.f, 0.f)); // duduk menghadap ke dalam ruangan
		}
		break;
	case EJRCustomerState::WaitPay:
		Patience = 1.5f;	// sabar menunggu dilayani di kasir
		PayOrder = GetWorld()->GetTimeSeconds();
		SetActorLocation(Target);
		SetActorRotation(FRotator(0.f, 0.f, 0.f)); // menghadap kasir
		break;
	case EJRCustomerState::Gone:
		GM->CustomerGone(this);
		break;
	default:
		break;
	}
}

void AJRCustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (!GM || !GM->IsPlaying())
	{
		return;
	}
	const float DtH = DeltaSeconds * GM->HoursPerSecond();

	switch (State)
	{
	case EJRCustomerState::Walk:
		if (MoveStep(DeltaSeconds))
		{
			Arrive(GM);
		}
		break;
	case EJRCustomerState::Queue:
		WaitMinutes += DtH * 60.f;
		Patience -= DtH;
		if (Patience <= 0.f)
		{
			GM->CountAngry(true);
			GM->LeaveAngry(this, DisplayName + JRT(TEXT(" bosan antre lalu pulang")));
		}
		break;
	case EJRCustomerState::WaitStart:
		WaitMinutes += DtH * 60.f;
		Patience -= DtH;
		if (Patience <= 0.f)
		{
			GM->CountAngry(false);
			GM->LeaveAngry(this, JRPrintf(JRT(TEXT("%s pulang, unitnya tidak dinyalakan")), *DisplayName));
		}
		break;
	case EJRCustomerState::Playing:
		GM->TickPlaying(this, DtH);
		break;
	case EJRCustomerState::WaitPay:
		Patience -= DtH;
		if (Patience <= 0.f)
		{
			GM->RunAway(this);
		}
		break;
	default:
		break;
	}

	if (!IsValid(this) || State == EJRCustomerState::Gone)
	{
		return;
	}
	UpdateAnimation();

	FString Text;
	if (State == EJRCustomerState::Playing && bOrder) Text = JRT(TEXT("Pesan mi!"));
	else if (State == EJRCustomerState::Queue) Text = TEXT("...");
	else if (State == EJRCustomerState::WaitPay) Text = JRFormatRp(Bill);
	else if (State == EJRCustomerState::Walk && NextState == EJRCustomerState::Gone) Text = Mood;
	Bubble->SetText(FText::FromString(Text));

	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Bubble->GetComponentLocation();
		Bubble->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}
