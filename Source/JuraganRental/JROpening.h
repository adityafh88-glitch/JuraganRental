// Adegan pembuka "Main baru" (Era 1): kamera dari gang kampung mendekati garasi, rolling door terangkat,
// lalu masuk melihat TV tabung yang menyala. Teks narasi digambar HUD. Bisa dilewati (LANJUT / Enter / ketuk).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JROpening.generated.h"

class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class JURAGANRENTAL_API AJROpening : public AActor
{
	GENERATED_BODY()

public:
	AJROpening();

	/** Mulai adegan: pasang rolling door tertutup dan pindahkan kamera pemain ke kamera adegan. */
	void Start(float HalfX, float DoorY);
	/** Akhiri (selesai atau dilewati): kamera kembali ke pemain, aktor dihancurkan. */
	void Finish();

	virtual void Tick(float DeltaSeconds) override;

	/** Teks narasi saat ini (kosong = tidak ada) dan kecerahannya (0..1) untuk HUD. */
	FString Caption() const;
	float CaptionAlpha() const;
	/** Layar menghitam di awal dan akhir (0 = jelas, 1 = hitam). */
	float Fade() const;

	static constexpr float Duration = 14.5f;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Cam;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shutter;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Grooves;

	float HX = 700.f;
	float Door = 420.f;
	float Time = 0.f;
	int32 SoundsPlayed = 0;
	bool bFinished = false;

	void PlaceCamera();
	void UpdateShutter();
};
