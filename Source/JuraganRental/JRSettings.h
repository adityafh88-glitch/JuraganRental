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
};

namespace JRSettings
{
	/** Pengaturan aktif (dimuat dari disk saat pertama dipakai). */
	JURAGANRENTAL_API UJRSettingsSave* Get();
	JURAGANRENTAL_API void Save();
}
