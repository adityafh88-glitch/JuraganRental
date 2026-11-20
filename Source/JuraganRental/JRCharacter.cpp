#include "JRCharacter.h"
#include "JRSettings.h"

#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
#include "JRHUD.h"
#include "JRInteractable.h"
#include "Engine/OverlapResult.h"

AJRCharacter::AJRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(85.f);

	HeldBowl = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldBowl"));
	HeldBowl->SetupAttachment(Camera);
	HeldBowl->SetRelativeLocation(FVector(45.f, 14.f, -24.f));
	HeldBowl->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	HeldBowl->SetRelativeScale3D(FVector(1.3f));
	HeldBowl->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldBowl->SetCastShadow(false);
	HeldBowl->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Bowl(TEXT("/Game/Props/SM_Mangkuk_Mi.SM_Mangkuk_Mi"));
	if (Bowl.Succeeded()) HeldBowl->SetStaticMesh(Bowl.Object);

	HeldBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldBox"));
	HeldBox->SetupAttachment(Camera);
	HeldBox->SetRelativeLocation(FVector(70.f, 0.f, -45.f));
	HeldBox->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	HeldBox->SetRelativeScale3D(FVector(1.f, 1.f, 1.3f));
	HeldBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldBox->SetCastShadow(false);
	HeldBox->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Kardus(TEXT("/Game/Props/SM_Kardus_Mi.SM_Kardus_Mi"));
	if (Kardus.Succeeded()) HeldBox->SetStaticMesh(Kardus.Object);

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
	PhoneAction = MakeAction(EInputActionValueType::Boolean);
	PauseAction = MakeAction(EInputActionValueType::Boolean);
	PauseAction->bTriggerWhenPaused = true;	// Esc juga harus bisa menutup menu jeda

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

	// Joystick (gamepad dan joystick virtual layar sentuh)
	Mapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
	{
		UInputModifierScalar* Speed = NewObject<UInputModifierScalar>(Mapping);
		Speed->Scalar = FVector(2.5f, 2.f, 1.f);
		UInputModifierNegate* Ng = Negate(false, true);
		FEnhancedActionKeyMapping& M = Mapping->MapKey(LookAction, EKeys::Gamepad_Right2D);
		M.Modifiers.Add(Ng);
		M.Modifiers.Add(Speed);
	}
	Mapping->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Bottom);
	Mapping->MapKey(PhoneAction, EKeys::Gamepad_Special_Right);
	Mapping->MapKey(ConfirmAction, EKeys::Gamepad_Special_Left);
	Mapping->MapKey(InteractAction, EKeys::E);
	Mapping->MapKey(ConfirmAction, EKeys::Enter);
	Mapping->MapKey(PhoneAction, EKeys::Tab);
	Mapping->MapKey(PauseAction, EKeys::Escape);
	Mapping->MapKey(PauseAction, EKeys::P);
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
	EIC->BindAction(PhoneAction, ETriggerEvent::Started, this, &AJRCharacter::TogglePhone);
	EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AJRCharacter::TogglePause);
}

void AJRCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || IsPhoneOpen()) return;
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AJRCharacter::Look(const FInputActionValue& Value)
{
	if (IsPhoneOpen()) return;
	const FVector2D Axis = Value.Get<FVector2D>();
	const UJRSettingsSave* S = JRSettings::Get();
	AddControllerYawInput(Axis.X * S->LookSpeed);
	AddControllerPitchInput((S->bInvertLook ? -Axis.Y : Axis.Y) * S->LookSpeed);
}

void AJRCharacter::InteractStarted()
{
	{
		const APlayerController* PC = Cast<APlayerController>(Controller);
		if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr)
		{
			if (Hud->IsBillingOpen()) { Hud->SetBillingOpen(false); return; }
		}
	}
	if (IsPhoneOpen()) return;
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

void AJRCharacter::TogglePhone()
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr) Hud->TogglePhone();
}

void AJRCharacter::TogglePause()
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr) Hud->TogglePause();
}

bool AJRCharacter::IsPhoneOpen() const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	const AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr;
	return Hud && Hud->IsUIOpen();
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
		// Bantuan bidik: benda kecil (sampah, kardus) cukup dekat arah pandang, tidak harus tepat di titik tengah.
		// Layar sentuh lebih longgar karena membidik dengan jari kurang presisi.
		const APlayerController* PC = Cast<APlayerController>(Controller);
		const AJRHUD* Hud = PC ? Cast<AJRHUD>(PC->GetHUD()) : nullptr;
		const bool bTouch = Hud && Hud->IsTouchMode();
		if (!NewTarget) NewTarget = FindAssistTarget(Start, Camera->GetForwardVector(), 260.f, bTouch ? 13.f : 6.f);
		const float Now = GetWorld()->GetTimeSeconds();
		if (NewTarget) TargetSeenAt = Now;
		else if (TargetActor.IsValid() && Now - TargetSeenAt < (bTouch ? 0.35f : 0.15f)
			&& FVector::Dist(TargetActor->GetActorLocation(), Start) < 320.f)
		{
			NewTarget = TargetActor.Get();	// bidikan goyang sesaat: sasaran dipertahankan sebentar
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

AActor* AJRCharacter::FindAssistTarget(const FVector& Start, const FVector& Dir, float Range, float ConeDeg) const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JRAssist), false, this);
	GetWorld()->OverlapMultiByChannel(Overlaps, Start, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Range), Params);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(ConeDeg));
	AActor* Best = nullptr;
	float BestScore = -1.f;
	TSet<AActor*> Seen;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (!A || Seen.Contains(A) || !A->Implements<UJRInteractable>()) continue;
		Seen.Add(A);
		const IJRInteractable* I = Cast<IJRInteractable>(A);
		const EJRInteractKind Kind = I->GetInteraction().Kind;
		if (Kind == EJRInteractKind::None) continue;
		const FVector Point = A->GetComponentsBoundingBox(true).GetCenter();
		const FVector To = Point - Start;
		const float Dist = To.Size();
		if (Dist > Range || Dist < 1.f) continue;
		const float Dot = FVector::DotProduct(To / Dist, Dir);
		if (Dot < MinDot) continue;
		// Utamakan yang paling segaris dengan pandangan; yang bisa ditekan/ditahan diutamakan dari sekadar info.
		const float Score = Dot + (Kind == EJRInteractKind::Info ? 0.f : 0.05f);
		if (Score <= BestScore) continue;
		// Harus terlihat (tidak terhalang dinding/benda lain)
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Point, ECC_Visibility, Params) && Hit.GetActor() != A) continue;
		Best = A;
		BestScore = Score;
	}
	return Best;
}

void AJRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRotationInitialized && Controller)
	{
		Controller->SetControlRotation(FRotator::ZeroRotator); // menghadap deretan TV
		bRotationInitialized = true;
	}

	if (const AJRGameMode* GM = GetWorld()->GetAuthGameMode<AJRGameMode>())
	{
		if (HeldBowl->IsVisible() != GM->bCarryMie) HeldBowl->SetVisibility(GM->bCarryMie);
		const bool bBox = !GM->CarryBox.IsNone();
		if (HeldBox->IsVisible() != bBox) HeldBox->SetVisibility(bBox);
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
