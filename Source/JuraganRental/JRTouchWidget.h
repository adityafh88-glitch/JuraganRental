// Tombol layar sentuh (Android / -jrtouch): tombol aksi (labelnya mengikuti benda yang dilihat), HP, dan Lanjut.
// Joystick gerak dan lihat memakai virtual joystick bawaan Unreal.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRTouchWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class JURAGANRENTAL_API UJRTouchWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	UButton* MakeRound(const FString& Label, float Size, const FLinearColor& Color, const FLinearColor& TextColor);

	UFUNCTION()
	void OnInteractPressed();

	UFUNCTION()
	void OnInteractReleased();

	UFUNCTION()
	void OnPhone();

	UFUNCTION()
	void OnNext();

	UPROPERTY()
	TObjectPtr<UButton> NextButton;

	UPROPERTY()
	TObjectPtr<UButton> ActionButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ActionLabel;

	FString ShownLabel;
};
