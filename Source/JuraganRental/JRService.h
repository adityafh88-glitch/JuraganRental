// Bisnis sampingan Game Center (Era 3): kulkas minuman, konter pulsa & top-up, printer & fotokopi.
// Aturannya ada di AJRGameMode (ServiceInteraction / ServiceInteract); aktor ini hanya geometri dan label.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRTypes.h"
#include "JRService.generated.h"

UCLASS()
class JURAGANRENTAL_API AJRService : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRService();

	/** Bangun geometri. Sumbu lokal +X = bagian depan (menghadap pembeli). */
	void Setup(EJRService InKind);

	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

	EJRService Kind = EJRService::Kulkas;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<class UTextRenderComponent> Label;
};
