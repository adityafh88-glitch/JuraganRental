// HUD: status toko, petunjuk interaksi, notifikasi, laporan harian, dan HP jadul (Tab).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JRHUD.generated.h"

class UJRPhoneWidget;
class UJRChoiceWidget;
class UJRTouchWidget;
class UJRMenuWidget;

UCLASS()
class JURAGANRENTAL_API AJRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	void TogglePhone();
	/** Esc: tutup HP, kembali di menu, atau buka menu jeda. */
	void TogglePause();
	UJRMenuWidget* GetMenu() const { return Menu; }
	class UJRPhoneWidget* GetPhone() const { return Phone; }
	void SetPhoneOpen(bool bOpen);
	bool IsPhoneOpen() const { return bPhoneOpen; }
	/** HP atau dialog kejadian sedang terbuka: pemain tidak bisa jalan/menoleh. */
	bool IsUIOpen() const;
	bool IsTouchMode() const { return bTouchMode; }
	/** Pasang ulang joystick layar sesuai pengaturan (tetap/ikut jari). */
	void ApplyTouchSettings();

private:
	void DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer);

	void SyncChoice();
	void ApplyInputMode();

	UPROPERTY()
	TObjectPtr<UJRPhoneWidget> Phone;

	UPROPERTY()
	TObjectPtr<UJRChoiceWidget> Choice;

	bool bChoiceShown = false;
	int32 ShownChoiceSerial = 0;
	bool bTouchMode = false;

	UPROPERTY()
	TObjectPtr<UJRTouchWidget> Touch;

	UPROPERTY()
	TObjectPtr<UJRMenuWidget> Menu;

	void OnMenuChanged();

	bool bPhoneOpen = false;
};
