// Layar komputer billing di kasir: daftar semua unit (TV, PC, VIP) dan tombol untuk menyalakannya dari jauh.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRBillingWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class JURAGANRENTAL_API UJRBillingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bangun ulang daftar unit (dipanggil saat dibuka dan berkala). */
	void Refresh(bool bForce = false);
	/** Dipanggil saat tombol tutup ditekan. */
	TFunction<void()> OnClose;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);

	UPROPERTY()
	TObjectPtr<UVerticalBox> List;

	UPROPERTY()
	TObjectPtr<UTextBlock> Summary;

	FString LastSig;
	float Timer = 0.f;
};
