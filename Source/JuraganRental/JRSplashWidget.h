// Layar pembuka: logo KAH Gamedev (ubin K, A, H muncul pop-up satu per satu), lalu logo Juragan Rental
// membal masuk seperti papan nama dipasang. Ketuk/klik di mana saja untuk melewati.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRSplashWidget.generated.h"

class UImage;
class UWidget;

UCLASS()
class JURAGANRENTAL_API UJRSplashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Dipanggil saat splash selesai atau dilewati (widget sudah dilepas dari layar). */
	TFunction<void()> OnFinished;
	bool IsDone() const { return bDone; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

private:
	void BuildTree();
	UImage* MakeImage(const TCHAR* Asset, const FVector2D& Size);
	void Finish();

	UPROPERTY()
	TArray<TObjectPtr<UImage>> Tiles;	// K, A, H

	UPROPERTY()
	TObjectPtr<UImage> Gamedev;

	UPROPERTY()
	TObjectPtr<UWidget> KahGroup;

	UPROPERTY()
	TObjectPtr<UImage> Title;

	float Time = 0.f;
	int32 SoundsPlayed = 0;	// bit per bunyi yang sudah diputar
	bool bDone = false;
};
