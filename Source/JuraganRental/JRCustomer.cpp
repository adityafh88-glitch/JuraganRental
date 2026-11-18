#include "JRCustomer.h"

#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "JRBlockout.h"
#include "JRGameMode.h"
#include "JRStation.h"

using namespace JRBlockout;

AJRCustomer::AJRCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Bubble = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Bubble"));
	Bubble->SetupAttachment(Root);
	Bubble->SetRelativeLocation(FVector(0, 0, 185));
	Bubble->SetHorizontalAlignment(EHTA_Center);
	Bubble->SetWorldSize(20.f);
	Bubble->SetTextRenderColor(FColor(255, 194, 75));
	Bubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJRCustomer::Setup(int32 InTypeIndex, const FString& InName, const FLinearColor& Top, const FLinearColor& Bottom,
	float InScale, EJRUnitType InWant, bool bInFlex)
{
	TypeIndex = InTypeIndex;
	DisplayName = InName;
	Want = InWant;
	bFlex = bInFlex;

	// Badan sederhana; +X adalah arah depan (pita mata).
	AddPart(this, Root, EShape::Cube, FVector(26, 42, 50), FVector(0, 0, 25), Bottom, false);
	AddPart(this, Root, EShape::Cylinder, FVector(52, 52, 62), FVector(0, 0, 81), Top, false);
	AddPart(this, Root, EShape::Sphere, FVector(42, 42, 42), FVector(0, 0, 133), Hex(0xd9a77c), false);
	AddPart(this, Root, EShape::Cube, FVector(6, 22, 5), FVector(19, 0, 137), Hex(0x222222), false);
	SetActorScale3D(FVector(InScale));
	SetActorEnableCollision(false);
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
	FVector Next = Loc + Delta / Dist * Step;
	Next.Z = FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 8.f)) * 4.f; // langkah kecil
	SetActorLocation(Next);
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
			const bool bLesehan = Station->Type == EJRUnitType::PS;
			SetActorLocation(Station->GetSeatLocation() - FVector(0, 0, bLesehan ? 28.f * GetActorScale3D().Z : 0.f));
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

	FString Text;
	if (State == EJRCustomerState::Queue) Text = TEXT("...");
	else if (State == EJRCustomerState::WaitPay) Text = JRFormatRp(Bill);
	else if (State == EJRCustomerState::Walk && NextState == EJRCustomerState::Gone) Text = Mood;
	Bubble->SetText(FText::FromString(Text));

	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Bubble->GetComponentLocation();
		Bubble->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
}
