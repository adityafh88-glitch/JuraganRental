#include "JRVisitor.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

AJRVisitor::AJRVisitor()
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
}

void AJRVisitor::Setup(const TCHAR* MeshName, const TCHAR* AnimName)
{
	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Characters/%s.%s"), MeshName, MeshName)))
	{
		Body->SetSkeletalMesh(Mesh);
	}
	if (UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Characters/Animations/%s.%s"), AnimName, AnimName)))
	{
		Body->PlayAnimation(Anim, true);
	}
}

void AJRVisitor::LeaveTo(const FVector& Target)
{
	Exit = FVector(Target.X, Target.Y, 0.f);
	bLeaving = true;
	if (UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Animations/A_Jalan.A_Jalan")))
	{
		Body->PlayAnimation(Walk, true);
	}
}

void AJRVisitor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLeaving) return;
	const FVector Loc = GetActorLocation();
	FVector Delta = Exit - Loc;
	Delta.Z = 0.f;
	const float Dist = Delta.Size();
	if (Dist < 10.f)
	{
		Destroy();
		return;
	}
	SetActorLocation(Loc + Delta / Dist * FMath::Min(Dist, 300.f * DeltaSeconds));
	SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));
}
