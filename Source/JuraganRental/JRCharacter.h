// Pemain orang pertama: jalan dengan WASD, lihat dengan mouse, interaksi dengan E.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JRTypes.h"
#include "JRCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class JURAGANRENTAL_API AJRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AJRCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	/** Interaksi yang sedang dilihat pemain (dibaca HUD). */
	FJRInteraction CurrentInteraction;
	float HoldProgress = 0.f;

	/* Aksi pemain (dipanggil keyboard maupun tombol layar sentuh) */
	void InteractStarted();
	void InteractCompleted();
	void Confirm();
	void TogglePhone();
	void TogglePause();

private:
	void BuildInput();
	void UpdateTarget();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	bool IsPhoneOpen() const;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	/** Mangkuk mi yang dibawa (terlihat di depan kamera). */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HeldBowl;

	/** Kardus kiriman kurir yang sedang dibawa. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HeldBox;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ConfirmAction;

	UPROPERTY()
	TObjectPtr<UInputAction> PhoneAction;

	UPROPERTY()
	TObjectPtr<UInputAction> PauseAction;

	TWeakObjectPtr<AActor> TargetActor;
	bool bInteractHeld = false;
	bool bRotationInitialized = false;
};
