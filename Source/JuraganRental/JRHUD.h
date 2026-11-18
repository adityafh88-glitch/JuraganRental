// HUD sederhana berbasis Canvas: status toko, petunjuk interaksi, notifikasi, laporan harian, dan toko.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JRHUD.generated.h"

UCLASS()
class JURAGANRENTAL_API AJRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer);
};
