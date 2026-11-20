// Tombol layar sentuh (Android / -jrtouch): tombol aksi (labelnya mengikuti benda yang dilihat), HP, dan Lanjut.
// Jalan memakai virtual joystick kiri bawaan Unreal; melihat sekitar dengan menggeser jari di sisi kanan layar (UJRLookPad).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JRTouchWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * Area tak terlihat di sisi kanan layar: geser jari untuk menoleh (pengganti joystick kanan).
 * Tiap jari dilacak sendiri, jadi bisa berjalan dengan jempol kiri sambil menoleh dengan jempol kanan.
 */
UCLASS()
class JURAGANRENTAL_API UJRLookPad : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

private:
	int32 Finger = -1;	// jari yang sedang menggeser (-1 = tidak ada)
};

UCLASS()
class JURAGANRENTAL_API UJRTouchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Tampil/sembunyikan tombol sesuai keadaan permainan. Dipanggil HUD tiap frame:
	 * widget yang Collapsed tidak menjalankan NativeTick, jadi tidak bisa memunculkan dirinya sendiri lagi.
	 */
	void SyncVisibility();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	UButton* MakePhoneButton();
	UButton* MakeRing(const FString& Label, int32 FontSize, const FLinearColor& Ring);
	UButton* MakeRound(const FString& Label, float Size, const FLinearColor& Color, const FLinearColor& TextColor);

	UFUNCTION()
	void OnInteractPressed();

	UFUNCTION()
	void OnInteractReleased();

	UFUNCTION()
	void OnPhone();

	UFUNCTION()
	void OnNext();

	UFUNCTION()
	void OnPause();

	UFUNCTION()
	void OnMissions();

	UPROPERTY()
	TObjectPtr<UButton> MissionToggle;	// area transparan di atas judul panel misi

	UPROPERTY()
	TObjectPtr<UButton> PauseButton;

	UPROPERTY()
	TObjectPtr<UButton> PhoneButton;

	UPROPERTY()
	TObjectPtr<UButton> NextButton;

	UPROPERTY()
	TObjectPtr<UJRLookPad> LookPad;

	UPROPERTY()
	TObjectPtr<UButton> ActionButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ActionLabel;

	FString ShownLabel;
};
