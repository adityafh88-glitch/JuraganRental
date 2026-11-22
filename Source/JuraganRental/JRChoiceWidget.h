// Dialog kejadian: judul, cerita singkat, dan 2-3 pilihan yang bisa diklik atau diketuk.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRChoiceWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class JURAGANRENTAL_API UJRChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show(const FString& Title, const FString& Body, const TArray<FString>& Labels, bool bNeutral = false);

	/** Dipanggil dengan indeks pilihan yang diklik. */
	TFunction<void(int32)> OnPick;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Buttons;
};
