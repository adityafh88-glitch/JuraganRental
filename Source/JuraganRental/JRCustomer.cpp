#include "JRCustomer.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "JRGameMode.h"
#include "JRStation.h"

namespace
{
	UAnimSequence* LoadAnim(const TCHAR* Name)
	{
		return LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Characters/Animations/%s.%s"), Name, Name));
	}
}

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

	const FString Path = FString::Printf(TEXT("/Game/Characters/%s.%s"), MeshName, MeshName);
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path))
	{
		Body->SetSkeletalMesh(Mesh);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JuraganRental: karakter %s tidak ditemukan"), *Path);
	}
	SetActorScale3D(FVector(InScale));
	UpdateAnimation();
}

FJRInteraction AJRCustomer::GetInteraction() const
{
	if (State == EJRCustomerState::WaitPay)
	{
		FJRInteraction Out;
		Out.Kind = EJRInteractKind::Press;
		Out.Prompt = FString::Printf(TEXT("Terima bayaran %s  (%s)"), *DisplayName, *JRFormatRp(Bill));
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
		if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->CollectPayer(this);
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
		PlayLoop(LoadAnim(Station && Station->Type == EJRUnitType::PC ? TEXT("A_Duduk_Kursi_Main") : TEXT("A_Duduk_Lesehan_Main")));
		break;
	default:
		PlayLoop(LoadAnim(TEXT("A_Idle")));
		break;
	}
}

void AJRCustomer::WalkTo(const FVector& Loc, EJRCustomerState Next)
{
	Target = FVector(Loc.X, Loc.Y, 0.f);
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
		return true;
	}
	const float Step = FMath::Min(Dist, Speed * DeltaSeconds);
	SetActorLocation(FVector(Loc.X, Loc.Y, 0.f) + Delta / Dist * Step);
	SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));
	return Dist - Step < 5.f;
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
		}
		break;
	case EJRCustomerState::WaitPay:
		Patience = 1.f;
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
			GM->LeaveAngry(this, DisplayName + TEXT(" bosan antre lalu pulang"));
		}
		break;
	case EJRCustomerState::WaitStart:
		WaitMinutes += DtH * 60.f;
		Patience -= DtH;
		if (Patience <= 0.f)
		{
			GM->LeaveAngry(this, FString::Printf(TEXT("%s pulang, unitnya tidak dinyalakan"), *DisplayName));
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
	if (State == EJRCustomerState::Playing && bOrder) Text = TEXT("Pesan mi!");
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
