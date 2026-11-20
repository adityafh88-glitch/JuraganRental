// Pengaturan pemain (disimpan ke slot "JRSettings"): suara dan kontrol layar sentuh.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JRSettings.generated.h"

UCLASS()
class JURAGANRENTAL_API UJRSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bSfx = true;			// efek suara (unit, kasir, motor, dll.)

	UPROPERTY()
	bool bVoice = true;			// suara karakter

	UPROPERTY()
	bool bMusic = true;			// musik radio

	UPROPERTY()
	bool bJoystickFixed = true;	// joystick layar diam di tempat (false = pindah ke posisi jari)

	UPROPERTY()
	bool bInvertLook = false;	// geser ke atas = lihat ke bawah

	UPROPERTY()
	float LookSpeed = 1.f;		// kepekaan mouse / joystick lihat

	UPROPERTY()
	float UIScale = 1.8f;		// ukuran tampilan di layar sentuh (HP kecil butuh teks lebih besar)

	UPROPERTY()
	float JoystickSize = 0.85f;	// ukuran joystick layar (1 = bawaan Unreal; layar HP kecil cocok sedikit lebih kecil)

	UPROPERTY()
	bool bMissionsCompact = false;	// daftar misi diciutkan jadi satu baris

	UPROPERTY()
	int32 Language = 0;			// 0 otomatis (ikut Windows), 1 Indonesia, 2 English
};

namespace JRSettings
{
	/** Pengaturan aktif (dimuat dari disk saat pertama dipakai). */
	JURAGANRENTAL_API UJRSettingsSave* Get();
	JURAGANRENTAL_API void Save();

	/** Ukuran tampilan layar sentuh: Kecil 1,2 / Sedang 1,5 / Besar 1,8 / Sangat besar 2,1. */
	JURAGANRENTAL_API float NextUIScale(float Current);
	JURAGANRENTAL_API FString UIScaleName(float Scale);

	/** Ukuran joystick: Kecil 0,7 / Sedang 0,85 / Besar 1,0 / Sangat besar 1,15. */
	JURAGANRENTAL_API float NextJoystickSize(float Current);
	JURAGANRENTAL_API FString JoystickSizeName(float Size);
}
