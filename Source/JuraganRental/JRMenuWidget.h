// Menu layar penuh: menu utama, menu jeda (Esc), halaman pengaturan, dan konfirmasi.
// Dibangun dari C++ seperti widget lain (tanpa aset UMG).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRMenuWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UJRPhoneButton;

UENUM()
enum class EJRMenuPage : uint8
{
	None,
	Main,
	Pause,
	Settings,
	Confirm,
	DemoEnd
};

UCLASS()
class JURAGANRENTAL_API UJRMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowMain();
	void ShowPause();
	void ShowSettings();
	void ShowConfirm(const FString& Title, const FString& Body, const FString& Yes, TFunction<void()> OnYes);
	void ShowDemoEnd();
	void Close();
	/** Esc saat menu terbuka: kembali satu langkah. Mengembalikan true jika menu jadi tertutup. */
	bool Back();

	EJRMenuPage GetPage() const { return Page; }
	bool IsOpen() const { return Page != EJRMenuPage::None; }

	/** Dipanggil HUD saat menu dibuka/ditutup (mengatur input dan jeda). */
	TFunction<void()> OnChanged;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();
	void Begin(EJRMenuPage NewPage, const FString& Title, const FString& Subtitle, float Width);
	UJRPhoneButton* AddButton(const FString& Label, TFunction<void()> Action, bool bPrimary = false, bool bEnabled = true);
	void AddSection(const FString& Title);
	void AddRow(const FString& Label, const FString& Value, TFunction<void()> Action);
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);

	UPROPERTY()
	TObjectPtr<class USizeBox> CardSize;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<class UImage> LogoImage;	// logo Juragan Rental di menu utama

	UPROPERTY()
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Content;

	EJRMenuPage Page = EJRMenuPage::None;
	EJRMenuPage ReturnPage = EJRMenuPage::None;	// halaman sebelum pengaturan/konfirmasi
};
