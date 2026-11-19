// Radio Juragan FM di meja kasir: memutar lagu MUS_Radio_1..N bergiliran.
// Tekan aksi untuk menyalakan / ganti lagu / mematikan. Ikut mati saat MCB turun,
// dan mengecil sebentar saat ada karakter yang bicara.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JRInteractable.h"
#include "JRRadio.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class JURAGANRENTAL_API AJRRadio : public AActor, public IJRInteractable
{
	GENERATED_BODY()

public:
	AJRRadio();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FJRInteraction GetInteraction() const override;
	virtual void Interact() override;

	static constexpr float BaseVolume = 0.5f;

private:
	void PlayTrack(int32 Index);
	bool CanPlay() const;	// pengaturan musik nyala dan listrik tidak mati

	UFUNCTION()
	void OnTrackFinished();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Music;

	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> Tracks;

	bool bOn = true;
	int32 Track = 0;
	float Duck = 1.f;			// 1 = normal, mengecil saat karakter bicara
	bool bSwitching = false;	// sedang ganti lagu: abaikan event selesai dari lagu lama
};
