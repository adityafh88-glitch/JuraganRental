// HUD: status toko, petunjuk interaksi, notifikasi, laporan harian, dan HP jadul (Tab).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JRHUD.generated.h"

class UJRPhoneWidget;

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

private:
	void DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer);

	UPROPERTY()
	TObjectPtr<UJRPhoneWidget> Phone;

	bool bPhoneOpen = false;
};
