// HUD: status toko, petunjuk interaksi, notifikasi, laporan harian, dan HP jadul (Tab).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "JRHUD.generated.h"

class UJRPhoneWidget;
class UJRChoiceWidget;
class UJRTouchWidget;
class UJRMenuWidget;
class UJRBillingWidget;

UCLASS()
class JURAGANRENTAL_API AJRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;
	virtual void PostRender() override;

	void TogglePhone();
	void SetBillingOpen(bool bOpen);
	bool IsBillingOpen() const { return bBillingOpen; }
	/** Esc: tutup HP, kembali di menu, atau buka menu jeda. */
	void TogglePause();
	UJRMenuWidget* GetMenu() const { return Menu; }
	class UJRPhoneWidget* GetPhone() const { return Phone; }
	void SetPhoneOpen(bool bOpen);
	bool IsPhoneOpen() const { return bPhoneOpen; }
	/** HP atau dialog kejadian sedang terbuka: pemain tidak bisa jalan/menoleh. */
	bool IsUIOpen() const;
	bool IsTouchMode() const { return bTouchMode; }

	/** Mode foto: semua HUD dan tombol disembunyikan (tombol sentuh tetap berfungsi tapi tak terlihat). */
	void SetPhotoMode(bool bOn);
	bool IsPhotoMode() const { return bPhotoMode; }
	/** PC (F12): simpan foto layar tanpa HUD ke Dokumen/Juragan Rental/Foto. */
	void TakePhoto();
	/** Pasang ulang joystick layar sesuai pengaturan (tetap/ikut jari). */
	void ApplyTouchSettings();
	/** Ukuran tampilan layar sentuh (HP dalam game, menu, dialog, dan teks HUD). PC selalu 1. */
	void ApplyUIScale();
	/** Bahasa diganti di Pengaturan: bangun ulang widget agar teksnya langsung berganti. */
	void OnLanguageChanged();
	/** Buka dialog "Beri nama rentalmu" (dari HP > Atur). */
	void OpenNameDialog();

private:
	float UIS = 1.f;	// skala teks/panel HUD Canvas, mengikuti skala UMG di mode sentuh
	float LookYaw0 = 0.f;	// -jrtouchflow: arah pandang sebelum simulasi geser

	/** Layar pembuka KAH Gamedev + logo, sekali saat game pertama dibuka. */
	UPROPERTY()
	TObjectPtr<class UJRSplashWidget> Splash;
	bool bSplashChecked = false;

	UPROPERTY()
	TObjectPtr<class UJRNameWidget> NameWidget;

	UPROPERTY()
	TObjectPtr<class UJRMomentCard> MomentCard;
	bool bNameShown = false;
	bool bMomentShown = false;
	void DrawPanel(const FString& Title, const TArray<FString>& Lines, const FString& Footer);

	void SyncChoice();
	/** HP, tombol sentuh, billing, dialog nama, kartu momen, dan dialog pilihan. */
	void CreateGameWidgets();
	bool bStickShown = true;
	void ApplyInputMode();

	UPROPERTY()
	TObjectPtr<UJRPhoneWidget> Phone;

	UPROPERTY()
	TObjectPtr<UJRChoiceWidget> Choice;

	bool bChoiceShown = false;
	int32 ShownChoiceSerial = 0;
	bool bTouchMode = false;
	bool bPhotoMode = false;
	float PhotoHint = 0.f;		// detik petunjuk mode foto masih tampil
	int32 PhotoHideFrames = 0;	// HUD disembunyikan beberapa frame saat F12 memotret
	FString PendingPhoto;		// nama file foto yang menunggu konfirmasi
	float FpsSmooth = 60.f;

	UPROPERTY()
	TObjectPtr<UJRTouchWidget> Touch;

	UPROPERTY()
	TObjectPtr<UJRMenuWidget> Menu;

	UPROPERTY()
	TObjectPtr<UJRBillingWidget> Billing;

	bool bBillingOpen = false;

	void OnMenuChanged();

	bool bPhoneOpen = false;
};
