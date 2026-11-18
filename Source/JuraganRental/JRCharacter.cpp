#include "JRCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "JRGameMode.h"
#include "JRInteractable.h"

AJRCharacter::AJRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(85.f);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
}

void AJRCharacter::BuildInput()
{
	if (Mapping) return;

	auto MakeAction = [this](EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this);
		A->ValueType = Type;
		return A;
	};
	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	InteractAction = MakeAction(EInputActionValueType::Boolean);
	ConfirmAction = MakeAction(EInputActionValueType::Boolean);
	for (int32 i = 0; i < 6; ++i) BuyActions.Add(MakeAction(EInputActionValueType::Boolean));

	Mapping = NewObject<UInputMappingContext>(this);
	auto Negate = [this](bool bX, bool bY)
	{
		UInputModifierNegate* N = NewObject<UInputModifierNegate>(Mapping);
		N->bX = bX; N->bY = bY; N->bZ = false;
		return N;
	};
	auto Swizzle = [this]() { return NewObject<UInputModifierSwizzleAxis>(Mapping); }; // X -> Y

	// Gerak: D/kanan = +X, A/kiri = -X, W/atas = +Y, S/bawah = -Y
	for (const FKey& K : { EKeys::D, EKeys::Right }) Mapping->MapKey(MoveAction, K);
	for (const FKey& K : { EKeys::A, EKeys::Left }) Mapping->MapKey(MoveAction, K).Modifiers.Add(Negate(true, true));
	for (const FKey& K : { EKeys::W, EKeys::Up }) Mapping->MapKey(MoveAction, K).Modifiers.Add(Swizzle());
	for (const FKey& K : { EKeys::S, EKeys::Down })
	{
		UInputModifierSwizzleAxis* Sw = Swizzle();
		UInputModifierNegate* Ng = Negate(true, true);
		FEnhancedActionKeyMapping& M = Mapping->MapKey(MoveAction, K);
		M.Modifiers.Add(Sw);
		M.Modifiers.Add(Ng);
	}
	Mapping->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	Mapping->MapKey(InteractAction, EKeys::E);
	Mapping->MapKey(ConfirmAction, EKeys::Enter);
	const FKey Numbers[6] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
	for (int32 i = 0; i < 6; ++i) Mapping->MapKey(BuyActions[i], Numbers[i]);
}

void AJRCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(Mapping, 0);
		}
	}
}

void AJRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AJRCharacter::Move);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AJRCharacter::Look);
	EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AJRCharacter::InteractStarted);
	EIC->BindAction(InteractAction, ETriggerEvent::Completed, this, &AJRCharacter::InteractCompleted);
	EIC->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AJRCharacter::Confirm);
	for (int32 i = 0; i < BuyActions.Num(); ++i)
	{
		EIC->BindActionValueLambda(BuyActions[i], ETriggerEvent::Started, [this, i](const FInputActionValue&) { Buy(i + 1); });
	}
}

void AJRCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller) return;
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AJRCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AJRCharacter::InteractStarted()
{
	bInteractHeld = true;
	if (CurrentInteraction.Kind == EJRInteractKind::Press)
	{
		if (IJRInteractable* I = Cast<IJRInteractable>(TargetActor.Get())) I->Interact();
	}
}

void AJRCharacter::InteractCompleted()
{
	bInteractHeld = false;
	HoldProgress = 0.f;
}

void AJRCharacter::Confirm()
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->Confirm();
}

void AJRCharacter::Buy(int32 Slot)
{
	if (AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>()) GM->TryBuy(Slot);
}

void AJRCharacter::UpdateTarget()
{
	AActor* NewTarget = nullptr;
	const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>();
	if (GM && GM->IsPlaying())
	{
		const FVector Start = Camera->GetComponentLocation();
		const FVector End = Start + Camera->GetForwardVector() * 260.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(JRInteract), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor() && Hit.GetActor()->Implements<UJRInteractable>())
		{
			NewTarget = Hit.GetActor();
		}
	}
	if (NewTarget != TargetActor.Get())
	{
		TargetActor = NewTarget;
		HoldProgress = 0.f;
	}
	const IJRInteractable* I = Cast<IJRInteractable>(NewTarget);
	CurrentInteraction = I ? I->GetInteraction() : FJRInteraction();
}

void AJRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRotationInitialized && Controller)
	{
		Controller->SetControlRotation(FRotator::ZeroRotator); // menghadap deretan TV
		bRotationInitialized = true;
	}

	UpdateTarget();
	if (bInteractHeld && CurrentInteraction.Kind == EJRInteractKind::Hold)
	{
		HoldProgress += DeltaSeconds;
		if (HoldProgress >= CurrentInteraction.HoldTime)
		{
			if (IJRInteractable* I = Cast<IJRInteractable>(TargetActor.Get())) I->Interact();
			HoldProgress = 0.f;
			bInteractHeld = false;
		}
	}
	else
	{
		HoldProgress = 0.f;
	}
}
