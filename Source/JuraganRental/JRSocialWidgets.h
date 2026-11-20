// Layar kecil untuk fitur "pamer": dialog beri nama rental dan Kartu Momen (ringkasan hari yang siap di-screenshot).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRSocialWidgets.generated.h"

class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

/** "Beri nama rentalmu": kotak isian + saran nama + pratinjau papan nama. */
UCLASS()
class JURAGANRENTAL_API UJRNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Tampilkan dengan nama awal. OnDone dipanggil dengan nama yang dipilih. */
	void Open(const FString& Current);
	TFunction<void(const FString&)> OnDone;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();
	void UpdatePreview();
	UFUNCTION()
	void OnTextChanged(const FText& Text);

	UPROPERTY()
	TObjectPtr<UEditableTextBox> Input;

	UPROPERTY()
	TObjectPtr<UTextBlock> Preview;
};

/** Kartu Momen: ringkasan hari ini yang rapi untuk dibagikan (nama rental, bintang, angka penting, momen lucu). */
UCLASS()
class JURAGANRENTAL_API UJRMomentCard : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Isi ulang dari data GameMode lalu tampilkan. */
	void Refresh();
	TFunction<void()> OnReport;		// "Laporan lengkap"
	TFunction<void()> OnContinue;	// "Buka toko besok"

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();

	UPROPERTY()
	TObjectPtr<UVerticalBox> Body;
};
