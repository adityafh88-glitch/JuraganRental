// HUD: status toko, petunjuk interaksi, notifikasi, laporan harian, dan HP jadul (Tab).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JRHUD.generated.h"

class UJRPhoneWidget;
class UJRChoiceWidget;
class UJRTouchWidget;

UCLASS()
class JURAGANRENTAL_API AJRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	void TogglePhone();
	void SetPhoneOpen(bool bOpen);
	bool IsPhoneOpen() const { return bPhoneOpen; }
	/** HP atau dialog kejadian sedang terbuka: pemain tidak bisa jalan/menoleh. */
	bool IsUIOpen() const { return bPhoneOpen || bChoiceShown; }
	bool IsTouchMode() const { return bTouchMode; }

private:
	void DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer);

	void SyncChoice();
	void ApplyInputMode();

	UPROPERTY()
	TObjectPtr<UJRPhoneWidget> Phone;

	UPROPERTY()
	TObjectPtr<UJRChoiceWidget> Choice;

	bool bChoiceShown = false;
	bool bTouchMode = false;

	UPROPERTY()
	TObjectPtr<UJRTouchWidget> Touch;

	bool bPhoneOpen = false;
};
