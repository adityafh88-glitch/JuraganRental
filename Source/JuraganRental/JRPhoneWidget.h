// HP jadul di dalam game: aplikasi Toko Grosir, Tarif, dan Hari Ini.
// Dibangun penuh dari C++ (tanpa aset Widget Blueprint) dan bisa diklik mouse maupun diketuk.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "JRPhoneWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class AJRGameMode;

/** Tombol yang menjalankan fungsi C++ biasa saat diklik. */
UCLASS()
class JURAGANRENTAL_API UJRPhoneButton : public UButton
{
	GENERATED_BODY()

public:
	void Bind(TFunction<void()> InAction);

private:
	UFUNCTION()
	void HandleClicked();

	TFunction<void()> Action;
};

UCLASS()
class JURAGANRENTAL_API UJRPhoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bangun ulang isi layar (dipanggil saat dibuka dan setelah membeli). */
	void Refresh(bool bForce = false);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void FillToko(AJRGameMode* GM);
	void FillTarif(AJRGameMode* GM);
	void FillHariIni(AJRGameMode* GM);
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
	UJRPhoneButton* MakeButton(const FString& Label, TFunction<void()> Action, bool bEnabled = true, bool bSelected = false);
	void AddLine(UWidget* Widget, float Top = 4.f);

	UPROPERTY()
	TObjectPtr<UVerticalBox> Content;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderMoney;

	UPROPERTY()
	TObjectPtr<UVerticalBox> TabRow;

	void BuildTabs();

	/** Baris angka di tab Hari ini; diperbarui di tempat tanpa membangun ulang. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> InfoLines;

	int32 CurrentTab = 0;
	int32 BuiltTab = -1;
	float RefreshTimer = 0.f;
	FString LastSignature;
};
